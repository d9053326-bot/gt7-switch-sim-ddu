#include "hud_renderer.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
    uint8_t r, g, b, a;
} rgba_t;

static inline rgba_t rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    rgba_t c = {r, g, b, a};
    return c;
}

static void set_color(SDL_Renderer *r, rgba_t c) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
}

static void fill_rect(SDL_Renderer *r, int x, int y, int w, int h, rgba_t c) {
    set_color(r, c);
    SDL_Rect rc = {x, y, w, h};
    SDL_RenderFillRect(r, &rc);
}

static void draw_rect(SDL_Renderer *r, int x, int y, int w, int h, int border, rgba_t c) {
    set_color(r, c);
    for (int i = 0; i < border; ++i) {
        SDL_Rect rc = {x + i, y + i, w - i * 2, h - i * 2};
        SDL_RenderDrawRect(r, &rc);
    }
}

/* Pure OLED Black background for true infinite contrast and maximum battery efficiency on Switch OLED */
static void draw_oled_black_background(SDL_Renderer *r) {
    set_color(r, rgba(0, 0, 0, 255));
    SDL_RenderClear(r);
}

/* Motorsport chamfered panel with glowing header accent strip */
static void draw_panel(SDL_Renderer *r, int x, int y, int w, int h, rgba_t bg, rgba_t border_col, int cut) {
    fill_rect(r, x + cut, y, w - cut * 2, h, bg);
    fill_rect(r, x, y + cut, cut, h - cut * 2, bg);
    fill_rect(r, x + w - cut, y + cut, cut, h - cut * 2, bg);
    for (int i = 0; i < cut; ++i) {
        set_color(r, bg);
        SDL_RenderDrawLine(r, x + cut - i, y + i, x + w - cut + i - 1, y + i);
        SDL_RenderDrawLine(r, x + cut - i, y + h - 1 - i, x + w - cut + i - 1, y + h - 1 - i);
    }

    /* Subtle top inner glow */
    set_color(r, rgba(border_col.r, border_col.g, border_col.b, 55));
    SDL_RenderDrawLine(r, x + cut + 1, y + 1, x + w - cut - 2, y + 1);
    SDL_RenderDrawLine(r, x + cut + 1, y + 2, x + w - cut - 2, y + 2);

    set_color(r, border_col);
    SDL_RenderDrawLine(r, x + cut, y, x + w - cut - 1, y);
    SDL_RenderDrawLine(r, x + w - cut - 1, y, x + w - 1, y + cut);
    SDL_RenderDrawLine(r, x + w - 1, y + cut, x + w - 1, y + h - cut - 1);
    SDL_RenderDrawLine(r, x + w - 1, y + h - cut - 1, x + w - cut - 1, y + h - 1);
    SDL_RenderDrawLine(r, x + cut, y + h - 1, x + w - cut - 1, y + h - 1);
    SDL_RenderDrawLine(r, x, y + h - cut - 1, x + cut, y + h - 1);
    SDL_RenderDrawLine(r, x, y + cut, x, y + h - cut - 1);
    SDL_RenderDrawLine(r, x, y + cut, x + cut, y);
}

static void draw_thick_line(SDL_Renderer *r, int x1, int y1, int x2, int y2, int thickness, rgba_t c) {
    set_color(r, c);
    int dx = abs(x2 - x1), dy = abs(y2 - y1);
    for (int i = -thickness / 2; i <= thickness / 2; ++i) {
        if (dx >= dy) {
            SDL_RenderDrawLine(r, x1, y1 + i, x2, y2 + i);
        } else {
            SDL_RenderDrawLine(r, x1 + i, y1, x2 + i, y2);
        }
    }
}

static void fill_circle(SDL_Renderer *r, int cx, int cy, int radius, rgba_t c) {
    set_color(r, c);
    for (int dy = -radius; dy <= radius; ++dy) {
        int dx = (int)sqrtf((float)(radius * radius - dy * dy));
        SDL_RenderDrawLine(r, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

static void draw_circle(SDL_Renderer *r, int cx, int cy, int radius, int thickness, rgba_t c) {
    set_color(r, c);
    int steps = 72;
    for (int t = 0; t < thickness; ++t) {
        int rad = radius - t;
        for (int i = 0; i < steps; ++i) {
            float a1 = (float)i * 2.0f * (float)M_PI / (float)steps;
            float a2 = (float)(i + 1) * 2.0f * (float)M_PI / (float)steps;
            int x1 = cx + (int)(cosf(a1) * rad);
            int y1 = cy + (int)(sinf(a1) * rad);
            int x2 = cx + (int)(cosf(a2) * rad);
            int y2 = cy + (int)(sinf(a2) * rad);
            SDL_RenderDrawLine(r, x1, y1, x2, y2);
        }
    }
}

/* 5x7 Pixel Bitmap Font */
static const uint8_t FONT5X7[96][5] = {
    [' ' - 32] = {0x00, 0x00, 0x00, 0x00, 0x00},
    ['!' - 32] = {0x00, 0x00, 0x5F, 0x00, 0x00},
    ['"' - 32] = {0x00, 0x07, 0x00, 0x07, 0x00},
    ['#' - 32] = {0x14, 0x7F, 0x14, 0x7F, 0x14},
    ['$' - 32] = {0x24, 0x2A, 0x7F, 0x2A, 0x12},
    ['%' - 32] = {0x23, 0x13, 0x08, 0x64, 0x62},
    ['&' - 32] = {0x36, 0x49, 0x55, 0x22, 0x50},
    ['\'' - 32]= {0x00, 0x05, 0x03, 0x00, 0x00},
    ['(' - 32] = {0x00, 0x1C, 0x22, 0x41, 0x00},
    [')' - 32] = {0x00, 0x41, 0x22, 0x1C, 0x00},
    ['*' - 32] = {0x14, 0x08, 0x3E, 0x08, 0x14},
    ['+' - 32] = {0x08, 0x08, 0x3E, 0x08, 0x08},
    [',' - 32] = {0x00, 0x50, 0x30, 0x00, 0x00},
    ['-' - 32] = {0x08, 0x08, 0x08, 0x08, 0x08},
    ['.' - 32] = {0x00, 0x60, 0x60, 0x00, 0x00},
    ['/' - 32] = {0x20, 0x10, 0x08, 0x04, 0x02},
    ['0' - 32] = {0x3E, 0x51, 0x49, 0x45, 0x3E},
    ['1' - 32] = {0x00, 0x42, 0x7F, 0x40, 0x00},
    ['2' - 32] = {0x42, 0x61, 0x51, 0x49, 0x46},
    ['3' - 32] = {0x21, 0x41, 0x45, 0x4B, 0x31},
    ['4' - 32] = {0x18, 0x14, 0x12, 0x7F, 0x10},
    ['5' - 32] = {0x27, 0x45, 0x45, 0x45, 0x39},
    ['6' - 32] = {0x3C, 0x4A, 0x49, 0x49, 0x30},
    ['7' - 32] = {0x01, 0x71, 0x09, 0x05, 0x03},
    ['8' - 32] = {0x36, 0x49, 0x49, 0x49, 0x36},
    ['9' - 32] = {0x06, 0x49, 0x49, 0x29, 0x1E},
    [':' - 32] = {0x00, 0x36, 0x36, 0x00, 0x00},
    [';' - 32] = {0x00, 0x56, 0x36, 0x00, 0x00},
    ['<' - 32] = {0x08, 0x14, 0x22, 0x41, 0x00},
    ['=' - 32] = {0x14, 0x14, 0x14, 0x14, 0x14},
    ['>' - 32] = {0x00, 0x41, 0x22, 0x14, 0x08},
    ['?' - 32] = {0x02, 0x01, 0x51, 0x09, 0x06},
    ['@' - 32] = {0x32, 0x49, 0x79, 0x41, 0x3E},
    ['A' - 32] = {0x7E, 0x11, 0x11, 0x11, 0x7E},
    ['B' - 32] = {0x7F, 0x49, 0x49, 0x49, 0x36},
    ['C' - 32] = {0x3E, 0x41, 0x41, 0x41, 0x22},
    ['D' - 32] = {0x7F, 0x41, 0x41, 0x22, 0x1C},
    ['E' - 32] = {0x7F, 0x49, 0x49, 0x49, 0x41},
    ['F' - 32] = {0x7F, 0x09, 0x09, 0x09, 0x01},
    ['G' - 32] = {0x3E, 0x41, 0x49, 0x49, 0x7A},
    ['H' - 32] = {0x7F, 0x08, 0x08, 0x08, 0x7F},
    ['I' - 32] = {0x00, 0x41, 0x7F, 0x41, 0x00},
    ['J' - 32] = {0x20, 0x40, 0x41, 0x3F, 0x01},
    ['K' - 32] = {0x7F, 0x08, 0x14, 0x22, 0x41},
    ['L' - 32] = {0x7F, 0x40, 0x40, 0x40, 0x40},
    ['M' - 32] = {0x7F, 0x02, 0x0C, 0x02, 0x7F},
    ['N' - 32] = {0x7F, 0x04, 0x08, 0x10, 0x7F},
    ['O' - 32] = {0x3E, 0x41, 0x41, 0x41, 0x3E},
    ['P' - 32] = {0x7F, 0x09, 0x09, 0x09, 0x06},
    ['Q' - 32] = {0x3E, 0x41, 0x51, 0x21, 0x5E},
    ['R' - 32] = {0x7F, 0x09, 0x19, 0x29, 0x46},
    ['S' - 32] = {0x46, 0x49, 0x49, 0x49, 0x31},
    ['T' - 32] = {0x01, 0x01, 0x7F, 0x01, 0x01},
    ['U' - 32] = {0x3F, 0x40, 0x40, 0x40, 0x3F},
    ['V' - 32] = {0x1F, 0x20, 0x40, 0x20, 0x1F},
    ['W' - 32] = {0x3F, 0x40, 0x38, 0x40, 0x3F},
    ['X' - 32] = {0x63, 0x14, 0x08, 0x14, 0x63},
    ['Y' - 32] = {0x07, 0x08, 0x70, 0x08, 0x07},
    ['Z' - 32] = {0x61, 0x51, 0x49, 0x45, 0x43},
    ['[' - 32] = {0x00, 0x7F, 0x41, 0x41, 0x00},
    ['\\' - 32]= {0x02, 0x04, 0x08, 0x10, 0x20},
    [']' - 32] = {0x00, 0x41, 0x41, 0x7F, 0x00},
    ['_' - 32] = {0x40, 0x40, 0x40, 0x40, 0x40},
    ['~' - 32] = {0x08, 0x04, 0x08, 0x10, 0x08},
};

static void draw_text(SDL_Renderer *r, const char *str, int x, int y, int scale, rgba_t c) {
    if (!str) return;
    set_color(r, c);
    int cx = x;
    for (const char *p = str; *p; ++p) {
        char ch = *p;
        if (ch >= 'a' && ch <= 'z') ch -= 32;
        if (ch < 32 || ch > 126) ch = ' ';
        const uint8_t *glyph = FONT5X7[ch - 32];
        for (int col = 0; col < 5; ++col) {
            uint8_t bits = glyph[col];
            for (int row = 0; row < 7; ++row) {
                if (bits & (1 << row)) {
                    SDL_Rect px = {cx + col * scale, y + row * scale, scale, scale};
                    SDL_RenderFillRect(r, &px);
                }
            }
        }
        cx += 6 * scale;
    }
}

static int text_width(const char *str, int scale) {
    if (!str || !*str) return 0;
    return (int)strlen(str) * 6 * scale - scale;
}

static void draw_text_centered(SDL_Renderer *r, const char *str, int cx, int y, int scale, rgba_t c) {
    int w = text_width(str, scale);
    draw_text(r, str, cx - w / 2, y, scale, c);
}

static void draw_text_right(SDL_Renderer *r, const char *str, int rx, int y, int scale, rgba_t c) {
    int w = text_width(str, scale);
    draw_text(r, str, rx - w, y, scale, c);
}

/* Vector 7-segment display with beveled gaps */
static uint8_t seg_mask_for_char(char c) {
    switch (c) {
        case '0': return 0x3F;
        case '1': return 0x06;
        case '2': return 0x5B;
        case '3': return 0x4F;
        case '4': return 0x66;
        case '5': return 0x6D;
        case '6': return 0x7D;
        case '7': return 0x07;
        case '8': return 0x7F;
        case '9': return 0x6F;
        case '-': return 0x40;
        case 'N': return 0x37;
        case 'R': return 0x77;
        default:  return 0x00;
    }
}

static void draw_7seg_char(SDL_Renderer *r, char ch, int x, int y, int w, int h, int t, rgba_t active_col, rgba_t ghost_col) {
    uint8_t mask = seg_mask_for_char(ch);
    int half_h = h / 2;
    int pad = 3;

    if (ghost_col.a > 0) {
        fill_rect(r, x + t, y, w - 2 * t, t, ghost_col);
        fill_rect(r, x + w - t, y + t, t, half_h - t - pad, ghost_col);
        fill_rect(r, x + w - t, y + half_h + pad, t, half_h - t - pad, ghost_col);
        fill_rect(r, x + t, y + h - t, w - 2 * t, t, ghost_col);
        fill_rect(r, x, y + half_h + pad, t, half_h - t - pad, ghost_col);
        fill_rect(r, x, y + t, t, half_h - t - pad, ghost_col);
        fill_rect(r, x + t, y + half_h - t / 2, w - 2 * t, t, ghost_col);
    }

    if (ch == 'R') {
        fill_rect(r, x, y, t, h, active_col);
        fill_rect(r, x + t, y, w - 2 * t, t, active_col);
        fill_rect(r, x + w - t, y + t, t, half_h - t, active_col);
        fill_rect(r, x + t, y + half_h - t / 2, w - 2 * t, t, active_col);
        draw_thick_line(r, x + w / 2, y + half_h, x + w - t / 2, y + h - t / 2, t, active_col);
        return;
    }

    if (mask & (1 << 0)) fill_rect(r, x + t, y, w - 2 * t, t, active_col);
    if (mask & (1 << 1)) fill_rect(r, x + w - t, y + t, t, half_h - t - pad, active_col);
    if (mask & (1 << 2)) fill_rect(r, x + w - t, y + half_h + pad, t, half_h - t - pad, active_col);
    if (mask & (1 << 3)) fill_rect(r, x + t, y + h - t, w - 2 * t, t, active_col);
    if (mask & (1 << 4)) fill_rect(r, x, y + half_h + pad, t, half_h - t - pad, active_col);
    if (mask & (1 << 5)) fill_rect(r, x, y + t, t, half_h - t - pad, active_col);
    if (mask & (1 << 6)) fill_rect(r, x + t, y + half_h - t / 2, w - 2 * t, t, active_col);
}

static void draw_7seg_number(SDL_Renderer *r, int val, int digits, bool zero_pad, int right_x, int y, int digit_w, int digit_h, int thick, int spacing, rgba_t active_col, rgba_t ghost_col) {
    char buf[16];
    if (zero_pad) {
        snprintf(buf, sizeof(buf), "%0*d", digits, val);
    } else {
        snprintf(buf, sizeof(buf), "%*d", digits, val);
    }
    int len = (int)strlen(buf);
    int start_x = right_x - len * digit_w - (len - 1) * spacing;
    for (int i = 0; i < len; ++i) {
        int dx = start_x + i * (digit_w + spacing);
        draw_7seg_char(r, buf[i], dx, y, digit_w, digit_h, thick, active_col, ghost_col);
    }
}

static void draw_7seg_number_centered(SDL_Renderer *r, int val, int digits, bool zero_pad, int center_x, int y, int digit_w, int digit_h, int thick, int spacing, rgba_t active_col, rgba_t ghost_col) {
    char buf[16];
    if (zero_pad) {
        snprintf(buf, sizeof(buf), "%0*d", digits, val);
    } else {
        snprintf(buf, sizeof(buf), "%*d", digits, val);
    }
    int len = (int)strlen(buf);
    int total_w = len * digit_w + (len - 1) * spacing;
    int start_x = center_x - total_w / 2;
    for (int i = 0; i < len; ++i) {
        int dx = start_x + i * (digit_w + spacing);
        draw_7seg_char(r, buf[i], dx, y, digit_w, digit_h, thick, active_col, ghost_col);
    }
}

/* Modern Luxury Automotive Vector Typography (OEM Sans-Serif Solid Glyphs) */
static void draw_modern_char(SDL_Renderer *r, char ch, int x, int y, int w, int h, int t, rgba_t col) {
    if (ch == ' ' || w <= 0 || h <= 0) return;
    int half_h = h / 2;
    int pad = t / 2;
    if (pad < 2) pad = 2;

    switch (ch) {
        case '0':
            fill_rect(r, x + pad, y, w - 2 * pad, t, col);
            fill_rect(r, x + pad, y + h - t, w - 2 * pad, t, col);
            fill_rect(r, x, y + pad, t, h - 2 * pad, col);
            fill_rect(r, x + w - t, y + pad, t, h - 2 * pad, col);
            draw_thick_line(r, x, y + pad, x + pad, y, t, col);
            draw_thick_line(r, x + w - pad, y, x + w, y + pad, t, col);
            draw_thick_line(r, x, y + h - pad, x + pad, y + h, t, col);
            draw_thick_line(r, x + w - pad, y + h, x + w, y + h - pad, t, col);
            break;

        case '1': {
            int stem_x = x + w / 2 - t / 2;
            fill_rect(r, stem_x, y, t, h, col);
            draw_thick_line(r, stem_x - w / 3, y + h / 4, stem_x, y + t / 2, t, col);
            break;
        }

        case '2':
            fill_rect(r, x + pad, y, w - 2 * pad, t, col);
            fill_rect(r, x + w - t, y + pad, t, half_h - pad, col);
            fill_rect(r, x + pad, y + half_h - t / 2, w - 2 * pad, t, col);
            fill_rect(r, x, y + half_h, t, half_h - pad, col);
            fill_rect(r, x, y + h - t, w, t, col);
            draw_thick_line(r, x, y + pad, x + pad, y, t, col);
            break;

        case '3':
            fill_rect(r, x + pad, y, w - 2 * pad, t, col);
            fill_rect(r, x + w - t, y + pad, t, half_h - pad, col);
            fill_rect(r, x + w / 3, y + half_h - t / 2, w * 2 / 3, t, col);
            fill_rect(r, x + w - t, y + half_h, t, half_h - pad, col);
            fill_rect(r, x + pad, y + h - t, w - 2 * pad, t, col);
            draw_thick_line(r, x, y + pad, x + pad, y, t, col);
            draw_thick_line(r, x, y + h - pad, x + pad, y + h, t, col);
            break;

        case '4':
            fill_rect(r, x, y, t, half_h, col);
            fill_rect(r, x, y + half_h - t / 2, w, t, col);
            fill_rect(r, x + w - t, y, t, h, col);
            break;

        case '5':
            fill_rect(r, x, y, w, t, col);
            fill_rect(r, x, y, t, half_h, col);
            fill_rect(r, x, y + half_h - t / 2, w - pad, t, col);
            fill_rect(r, x + w - t, y + half_h, t, half_h - pad, col);
            fill_rect(r, x + pad, y + h - t, w - 2 * pad, t, col);
            draw_thick_line(r, x, y + h - pad, x + pad, y + h, t, col);
            break;

        case '6':
            fill_rect(r, x + pad, y, w - 2 * pad, t, col);
            fill_rect(r, x, y + pad, t, h - 2 * pad, col);
            fill_rect(r, x, y + half_h - t / 2, w - pad, t, col);
            fill_rect(r, x + w - t, y + half_h, t, half_h - pad, col);
            fill_rect(r, x + pad, y + h - t, w - 2 * pad, t, col);
            draw_thick_line(r, x, y + pad, x + pad, y, t, col);
            draw_thick_line(r, x, y + h - pad, x + pad, y + h, t, col);
            break;

        case '7':
            fill_rect(r, x, y, w, t, col);
            draw_thick_line(r, x + w - t / 2, y + t / 2, x + w / 3, y + h - t / 2, t, col);
            break;

        case '8':
            fill_rect(r, x + pad, y, w - 2 * pad, t, col);
            fill_rect(r, x + pad, y + half_h - t / 2, w - 2 * pad, t, col);
            fill_rect(r, x + pad, y + h - t, w - 2 * pad, t, col);
            fill_rect(r, x, y + pad, t, half_h - 2 * pad, col);
            fill_rect(r, x + w - t, y + pad, t, half_h - 2 * pad, col);
            fill_rect(r, x, y + half_h + pad, t, half_h - 2 * pad, col);
            fill_rect(r, x + w - t, y + half_h + pad, t, half_h - 2 * pad, col);
            draw_thick_line(r, x, y + pad, x + pad, y, t, col);
            draw_thick_line(r, x + w - pad, y, x + w, y + pad, t, col);
            draw_thick_line(r, x, y + h - pad, x + pad, y + h, t, col);
            draw_thick_line(r, x + w - pad, y + h, x + w, y + h - pad, t, col);
            break;

        case '9':
            fill_rect(r, x + pad, y, w - 2 * pad, t, col);
            fill_rect(r, x, y + pad, t, half_h - pad, col);
            fill_rect(r, x, y + half_h - t / 2, w - pad, t, col);
            fill_rect(r, x + w - t, y + pad, t, h - 2 * pad, col);
            fill_rect(r, x + pad, y + h - t, w - 2 * pad, t, col);
            draw_thick_line(r, x, y + pad, x + pad, y, t, col);
            draw_thick_line(r, x + w - pad, y, x + w, y + pad, t, col);
            draw_thick_line(r, x, y + h - pad, x + pad, y + h, t, col);
            break;

        case 'P':
            fill_rect(r, x, y, t, h, col);
            fill_rect(r, x, y, w - pad, t, col);
            fill_rect(r, x + w - t, y + pad, t, half_h - pad, col);
            fill_rect(r, x, y + half_h - t / 2, w - pad, t, col);
            draw_thick_line(r, x + w - pad, y, x + w, y + pad, t, col);
            draw_thick_line(r, x + w - pad, y + half_h, x + w, y + half_h - pad, t, col);
            break;

        case 'R':
            fill_rect(r, x, y, t, h, col);
            fill_rect(r, x, y, w - pad, t, col);
            fill_rect(r, x + w - t, y + pad, t, half_h - pad, col);
            fill_rect(r, x, y + half_h - t / 2, w - pad, t, col);
            draw_thick_line(r, x + w - pad, y, x + w, y + pad, t, col);
            draw_thick_line(r, x + w / 2, y + half_h, x + w - t / 2, y + h - t / 2, t, col);
            break;

        case 'N':
            fill_rect(r, x, y, t, h, col);
            fill_rect(r, x + w - t, y, t, h, col);
            draw_thick_line(r, x + t / 2, y + t / 2, x + w - t / 2, y + h - t / 2, t, col);
            break;

        case 'D':
            fill_rect(r, x, y, t, h, col);
            fill_rect(r, x, y, w - t - pad, t, col);
            fill_rect(r, x, y + h - t, w - t - pad, t, col);
            fill_rect(r, x + w - t, y + pad * 2, t, h - 4 * pad, col);
            draw_thick_line(r, x + w - t - pad, y + t / 2, x + w - t / 2, y + pad * 2, t, col);
            draw_thick_line(r, x + w - t - pad, y + h - t / 2, x + w - t / 2, y + h - pad * 2, t, col);
            break;

        case 'S':
            fill_rect(r, x + pad, y, w - 2 * pad, t, col);
            fill_rect(r, x, y + pad, t, half_h - pad, col);
            fill_rect(r, x + pad, y + half_h - t / 2, w - 2 * pad, t, col);
            fill_rect(r, x + w - t, y + half_h, t, half_h - pad, col);
            fill_rect(r, x + pad, y + h - t, w - 2 * pad, t, col);
            draw_thick_line(r, x, y + pad, x + pad, y, t, col);
            draw_thick_line(r, x, y + h - pad, x + pad, y + h, t, col);
            break;

        case '-':
            fill_rect(r, x, y + half_h - t / 2, w, t, col);
            break;

        case '+':
            fill_rect(r, x, y + half_h - t / 2, w, t, col);
            fill_rect(r, x + w / 2 - t / 2, y, t, h, col);
            break;

        default:
            fill_rect(r, x, y, w, h, col);
            break;
    }
}

static void __attribute__((unused)) draw_modern_number(SDL_Renderer *r, int val, int digits, bool zero_pad, int right_x, int y, int digit_w, int digit_h, int thick, int spacing, rgba_t col) {
    char buf[16];
    if (zero_pad) {
        snprintf(buf, sizeof(buf), "%0*d", digits, val);
    } else {
        snprintf(buf, sizeof(buf), "%d", val);
    }
    int len = (int)strlen(buf);
    int start_x = right_x - len * digit_w - (len - 1) * spacing;
    for (int i = 0; i < len; ++i) {
        int dx = start_x + i * (digit_w + spacing);
        draw_modern_char(r, buf[i], dx, y, digit_w, digit_h, thick, col);
    }
}

static void draw_modern_number_centered(SDL_Renderer *r, int val, int digits, bool zero_pad, int center_x, int y, int digit_w, int digit_h, int thick, int spacing, rgba_t col) {
    char buf[16];
    if (zero_pad) {
        snprintf(buf, sizeof(buf), "%0*d", digits, val);
    } else {
        snprintf(buf, sizeof(buf), "%d", val);
    }
    int len = (int)strlen(buf);
    int total_w = len * digit_w + (len - 1) * spacing;
    int start_x = center_x - total_w / 2;
    for (int i = 0; i < len; ++i) {
        int dx = start_x + i * (digit_w + spacing);
        draw_modern_char(r, buf[i], dx, y, digit_w, digit_h, thick, col);
    }
}

static void format_lap_time(int32_t ms, char *out, size_t out_len) {
    if (ms <= 0 || ms > 3600000) {
        snprintf(out, out_len, "--:--.---");
        return;
    }
    int mins = ms / 60000;
    int secs = (ms % 60000) / 1000;
    int millis = ms % 1000;
    snprintf(out, out_len, "%d:%02d.%03d", mins, secs, millis);
}

static void format_sector_time(int32_t ms, char *out, size_t out_len) {
    if (ms <= 0 || ms > 600000) {
        snprintf(out, out_len, "--.---");
        return;
    }
    int secs = ms / 1000;
    int millis = ms % 1000;
    snprintf(out, out_len, "%02d.%03d", secs, millis);
}

static rgba_t tire_temp_color(float temp_c) {
    if (temp_c < 60.0f) return rgba(35, 135, 255, 255);
    if (temp_c < 75.0f) {
        float t = (temp_c - 60.0f) / 15.0f;
        return rgba((uint8_t)(35 * (1.0f - t) + 20 * t),
                    (uint8_t)(135 * (1.0f - t) + 230 * t),
                    (uint8_t)(255 * (1.0f - t) + 110 * t), 255);
    }
    if (temp_c < 95.0f) return rgba(20, 230, 110, 255);
    if (temp_c < 108.0f) {
        float t = (temp_c - 95.0f) / 13.0f;
        return rgba((uint8_t)(20 + 235 * t),
                    (uint8_t)(230 - 40 * t),
                    (uint8_t)(110 * (1.0f - t)), 255);
    }
    return rgba(255, 45, 45, 255);
}

static char gear_to_char(int gear) {
    if (gear == 0) return 'R';
    if (gear < 0 || gear > 9) return 'N';
    return (char)('0' + gear);
}

/* Helper: Draw an arc gauge composed of segmented blocks */
static void draw_segmented_arc(SDL_Renderer *r, int cx, int cy, int radius, int thickness, float start_deg, float end_deg, int num_segs, float fill_pct, rgba_t col_on, rgba_t col_off) {
    if (num_segs <= 0) return;
    float deg_step = (end_deg - start_deg) / (float)num_segs;
    int lit_count = (int)roundf(fill_pct * (float)num_segs);
    if (lit_count > num_segs) lit_count = num_segs;
    if (lit_count < 0) lit_count = 0;

    for (int i = 0; i < num_segs; ++i) {
        float deg = start_deg + (i + 0.5f) * deg_step;
        float rad_ang = deg * 3.14159265f / 180.0f;
        float cos_a = cosf(rad_ang);
        float sin_a = sinf(rad_ang);

        int x0 = cx + (int)((radius - thickness) * cos_a);
        int y0 = cy + (int)((radius - thickness) * sin_a);
        int x1 = cx + (int)(radius * cos_a);
        int y1 = cy + (int)(radius * sin_a);

        rgba_t c = (i < lit_count) ? col_on : col_off;
        draw_thick_line(r, x0, y0, x1, y1, 4, c);
    }
}

void hud_init(hud_ui_state_t *ui, const char *initial_ip) {
    memset(ui, 0, sizeof(*ui));
    ui->theme = HUD_THEME_GT7;
    ui->ev_style = EV_STYLE_BENZ_EQ;
    ui->fullscreen_canvas = false;
    ui->dock_visible = true;
    ui->use_mph = false;
    ui->show_ip_modal = false;
    ui->request_quit = false;
    strncpy(ui->edit_ip, initial_ip ? initial_ip : "AUTO", sizeof(ui->edit_ip) - 1);
    ui->ip_cursor = (int)strlen(ui->edit_ip);
}

void hud_cycle_theme(hud_ui_state_t *ui, int dir, uint32_t now_ms) {
    int next = ((int)ui->theme + dir) % (int)HUD_THEME_COUNT;
    if (next < 0) next += (int)HUD_THEME_COUNT;
    ui->theme = (hud_theme_t)next;
    ui->theme_banner_until_ms = now_ms + 2000;
}

void hud_cycle_ev_style(hud_ui_state_t *ui, int dir, uint32_t now_ms) {
    int next = ((int)ui->ev_style + dir) % (int)EV_STYLE_COUNT;
    if (next < 0) next += (int)EV_STYLE_COUNT;
    ui->ev_style = (ev_style_t)next;
    ui->theme_banner_until_ms = now_ms + 2000;
}

/* Shared helper: Top F1/GT3 15-LED RPM Shift Light Array */
static void draw_rpm_led_strip(SDL_Renderer *r, const gt7_state_t *state, uint32_t now_ms, int y, int h) {
    int num_leds = 15;
    int total_w = 1248;
    int start_x = (SCREEN_W - total_w) / 2;
    int gap = 8;
    int led_w = (total_w - gap * (num_leds - 1)) / num_leds;

    float max_rpm = (state->rpm_limiter > 1000) ? (float)state->rpm_limiter : 8500.0f;
    float start_rpm = max_rpm * 0.55f;
    float frac = (state->engine_rpm - start_rpm) / (max_rpm * 0.96f - start_rpm);
    if (frac < 0.0f) frac = 0.0f;
    if (frac > 1.0f) frac = 1.0f;
    int active_leds = (int)roundf(frac * num_leds);

    bool shift_blink = (state->flags & GT7_FLAG_REV_BLINK) || (state->engine_rpm >= max_rpm * 0.95f);
    bool blink_on = ((now_ms / 70) % 2) == 0;

    for (int i = 0; i < num_leds; ++i) {
        int lx = start_x + i * (led_w + gap);
        rgba_t col_on;
        if (i < 5)       col_on = rgba(0, 245, 95, 255);
        else if (i < 10) col_on = rgba(255, 55, 45, 255);
        else             col_on = rgba(45, 165, 255, 255);

        if (shift_blink) {
            col_on = blink_on ? rgba(0, 235, 255, 255) : rgba(255, 25, 65, 255);
        }

        bool lit = shift_blink ? true : (i < active_leds);
        rgba_t c = lit ? col_on : rgba(20, 25, 34, 255);
        fill_rect(r, lx, y, led_w, h, c);
        if (lit) {
            fill_rect(r, lx + 4, y + 3, led_w - 8, h / 3, rgba(255, 255, 255, 90));
        }
        draw_rect(r, lx, y, led_w, h, 1, lit ? rgba(255, 255, 255, 140) : rgba(45, 52, 66, 255));
    }
}

/* Shared helper: 4-Wheel Tire Temp + Slip + Suspension Widget */
static void draw_tires_widget(SDL_Renderer *r, const gt7_state_t *state, int x, int y, int w, int h, rgba_t accent) {
    draw_panel(r, x, y, w, h, rgba(0, 0, 0, 255), accent, 10);
    draw_text(r, "TIRE SURFACE & TRACTION", x + 16, y + 12, 2, accent);

    static const char *labels[4] = {"FL", "FR", "RL", "RR"};
    int box_w = 72, box_h = (h > 300) ? 102 : 82;
    int cx = x + w / 2;
    int cy = y + h / 2 + 12;

    draw_rect(r, cx - 32, cy - 58, 64, 116, 2, rgba(55, 65, 88, 255));
    SDL_RenderDrawLine(r, cx - 52, cy - 48, cx + 52, cy - 48);
    SDL_RenderDrawLine(r, cx - 52, cy + 48, cx + 52, cy + 48);

    int pos[4][2] = {
        {x + 24, cy - box_h - 8},
        {x + w - 24 - box_w, cy - box_h - 8},
        {x + 24, cy + 8},
        {x + w - 24 - box_w, cy + 8}
    };

    for (int i = 0; i < 4; ++i) {
        int tx = pos[i][0], ty = pos[i][1];
        rgba_t tcol = tire_temp_color(state->tire_temp[i]);
        fill_rect(r, tx, ty, box_w, box_h, rgba(tcol.r / 3, tcol.g / 3, tcol.b / 3, 255));
        fill_rect(r, tx + 4, ty + 4, box_w - 8, box_h - 8, tcol);

        if (state->tire_slip_ratio[i] > 0.12f) {
            draw_rect(r, tx - 2, ty - 2, box_w + 4, box_h + 4, 3, rgba(255, 240, 0, 255));
        } else {
            draw_rect(r, tx, ty, box_w, box_h, 2, rgba(220, 230, 245, 180));
        }

        draw_text_centered(r, labels[i], tx + box_w / 2, ty + 10, 2, rgba(10, 12, 16, 255));
        char tbuf[16];
        snprintf(tbuf, sizeof(tbuf), "%.0fC", state->tire_temp[i]);
        draw_text_centered(r, tbuf, tx + box_w / 2, ty + box_h / 2 - 8, 3, rgba(10, 12, 16, 255));

        /* Packet C Surface Contact Patch Badge: T=Tarmac, C=Curb, D=Dirt, G=Grass, S=Snow */
        char surf_char = (state->surface_type[i] != 0) ? (char)state->surface_type[i] : 'T';
        rgba_t surf_bg = rgba(14, 22, 34, 255);
        rgba_t surf_fg = rgba(0, 225, 255, 255);
        if (surf_char == 'C' || surf_char == 'c') {
            surf_bg = rgba(245, 175, 20, 255);
            surf_fg = rgba(0, 0, 0, 255);
        } else if (surf_char == 'D' || surf_char == 'd' || surf_char == 'G' || surf_char == 'g') {
            surf_bg = rgba(225, 45, 45, 255);
            surf_fg = rgba(255, 255, 255, 255);
        }
        int s_badge_y = ty + box_h - 22;
        fill_rect(r, tx + 6, s_badge_y, box_w - 12, 17, surf_bg);
        char s_str[8] = { surf_char, '\0' };
        draw_text_centered(r, s_str, tx + box_w / 2, s_badge_y + 2, 2, surf_fg);

        int susp_h = (int)(state->susp_height[i] * 0.75f);
        if (susp_h < 4) susp_h = 4;
        if (susp_h > box_h - 8) susp_h = box_h - 8;
        int sx = (i % 2 == 0) ? (tx + box_w + 6) : (tx - 13);
        fill_rect(r, sx, ty, 7, box_h, rgba(28, 34, 46, 255));
        fill_rect(r, sx, ty + box_h - susp_h, 7, susp_h, rgba(0, 220, 255, 255));
    }
}

/* Shared helper: G-Force Meter + Live Pedal Telemetry Trace Graph */
static void draw_gforce_and_trace(SDL_Renderer *r, const gt7_state_t *state, int x, int y, int w, int h, rgba_t accent) {
    draw_panel(r, x, y, w, h, rgba(0, 0, 0, 255), accent, 10);
    draw_text(r, "G-LOAD & TELEMETRY TRACE", x + 16, y + 10, 2, accent);

    int g_cx = x + 78;
    int g_cy = y + h / 2 + 10;
    int g_rad = (h > 240) ? 58 : 48;
    fill_circle(r, g_cx, g_cy, g_rad, rgba(0, 0, 0, 255));
    draw_circle(r, g_cx, g_cy, g_rad, 2, rgba(70, 85, 110, 255));
    draw_circle(r, g_cx, g_cy, g_rad / 2, 1, rgba(45, 55, 75, 255));
    set_color(r, rgba(55, 65, 85, 255));
    SDL_RenderDrawLine(r, g_cx - g_rad, g_cy, g_cx + g_rad, g_cy);
    SDL_RenderDrawLine(r, g_cx, g_cy - g_rad, g_cx, g_cy + g_rad);

    int dot_x = g_cx + (int)((state->g_lat / 2.2f) * g_rad);
    int dot_y = g_cy - (int)((state->g_lon / 2.2f) * g_rad);
    fill_circle(r, dot_x, dot_y, 7, rgba(255, 55, 75, 255));
    draw_circle(r, dot_x, dot_y, 7, 2, rgba(255, 255, 255, 255));

    char gbuf[32];
    snprintf(gbuf, sizeof(gbuf), "%.1fG", sqrtf(state->g_lat * state->g_lat + state->g_lon * state->g_lon));
    draw_text_centered(r, gbuf, g_cx, g_cy + g_rad + 6, 2, rgba(210, 225, 245, 255));

    int gx = x + 150;
    int gy = y + 34;
    int gw = w - 166;
    int gh = h - 46;
    fill_rect(r, gx, gy, gw, gh, rgba(8, 10, 16, 255));
    draw_rect(r, gx, gy, gw, gh, 1, rgba(45, 55, 75, 255));

    for (int i = 1; i < GT7_TRACE_LEN; ++i) {
        int idx0 = (state->trace_head + i - 1) % GT7_TRACE_LEN;
        int idx1 = (state->trace_head + i) % GT7_TRACE_LEN;
        int x0 = gx + ((i - 1) * gw) / (GT7_TRACE_LEN - 1);
        int x1 = gx + (i * gw) / (GT7_TRACE_LEN - 1);

        int ty0 = gy + gh - 2 - (int)((state->trace_throttle[idx0] / 100.0f) * (gh - 4));
        int ty1 = gy + gh - 2 - (int)((state->trace_throttle[idx1] / 100.0f) * (gh - 4));
        draw_thick_line(r, x0, ty0, x1, ty1, 2, rgba(0, 240, 100, 255));

        int by0 = gy + gh - 2 - (int)((state->trace_brake[idx0] / 100.0f) * (gh - 4));
        int by1 = gy + gh - 2 - (int)((state->trace_brake[idx1] / 100.0f) * (gh - 4));
        draw_thick_line(r, x0, by0, x1, by1, 2, rgba(255, 45, 55, 255));
    }
}

/* Core Helper: High-Precision 2D Circuit Map Geometry Renderer (Glitch-Free & Aspect-Ratio Correct) */
static void draw_track_map_geometry(SDL_Renderer *r, const gt7_state_t *state, int mx, int my, int map_w, int map_h, int outer_th, int inner_th, bool show_sf) {
    float span_x = state->track_max_x - state->track_min_x;
    float span_z = state->track_max_z - state->track_min_z;
    if (span_x < 20.0f) span_x = 20.0f;
    if (span_z < 20.0f) span_z = 20.0f;

    /* Maintain true real-world geographic aspect ratio */
    float scale = fminf((float)map_w / span_x, (float)map_h / span_z);
    int render_w = (int)(span_x * scale);
    int render_h = (int)(span_z * scale);
    int off_x = mx + (map_w - render_w) / 2;
    int off_y = my + (map_h - render_h) / 2;

    int count = state->track_pt_count;
    if (count >= 2) {
        int head = state->track_pt_head;
        int num_lines = (count < GT7_TRACK_PTS) ? (count - 1) : count;
        if (!state->track_is_closed && num_lines == count) {
            num_lines = count - 1;
        }

        for (int k = 0; k < num_lines; ++k) {
            int idx0, idx1;
            if (count < GT7_TRACK_PTS) {
                idx0 = k;
                idx1 = (k + 1) % count;
            } else {
                idx0 = (head + k) % GT7_TRACK_PTS;
                idx1 = (head + k + 1) % GT7_TRACK_PTS;
            }

            /* Skip line if teleport/respawn segment break */
            if (state->track_pts[idx1].segment_break) continue;

            float dx = state->track_pts[idx1].x - state->track_pts[idx0].x;
            float dz = state->track_pts[idx1].z - state->track_pts[idx0].z;
            if (dx * dx + dz * dz > 40000.0f) continue; /* Skip > 200m glitch jump */

            int px0 = off_x + (int)((state->track_pts[idx0].x - state->track_min_x) * scale);
            int py0 = off_y + (int)((state->track_pts[idx0].z - state->track_min_z) * scale);
            int px1 = off_x + (int)((state->track_pts[idx1].x - state->track_min_x) * scale);
            int py1 = off_y + (int)((state->track_pts[idx1].z - state->track_min_z) * scale);

            /* Outer dark track tarmac border */
            draw_thick_line(r, px0, py0, px1, py1, outer_th, rgba(40, 48, 65, 255));

            /* Inner racing line colored by braking/throttle zone */
            rgba_t line_col = rgba(0, 235, 115, 255);
            if (state->track_pts[idx0].pedal_state == 2)      line_col = rgba(255, 50, 60, 255);
            else if (state->track_pts[idx0].pedal_state == 0) line_col = rgba(255, 210, 0, 255);
            draw_thick_line(r, px0, py0, px1, py1, inner_th, line_col);
        }

        if (show_sf && state->track_is_closed) {
            int sfx = off_x + (int)((state->track_pts[0].x - state->track_min_x) * scale);
            int sfy = off_y + (int)((state->track_pts[0].z - state->track_min_z) * scale);
            fill_rect(r, sfx - 4, sfy - 7, 8, 14, rgba(255, 255, 255, 255));
            draw_text(r, "S/F", sfx + 6, sfy - 4, 1, rgba(255, 255, 255, 255));
        }
    }

    /* Live Car GPS Beacon */
    int car_px = off_x + (int)((state->pos_x - state->track_min_x) * scale);
    int car_py = off_y + (int)((state->pos_z - state->track_min_z) * scale);
    if (car_px < mx) car_px = mx;
    if (car_px > mx + map_w) car_px = mx + map_w;
    if (car_py < my) car_py = my;
    if (car_py > my + map_h) car_py = my + map_h;

    fill_circle(r, car_px, car_py, 10, rgba(0, 225, 255, 95));
    fill_circle(r, car_px, car_py, 5, rgba(0, 240, 255, 255));
    draw_circle(r, car_px, car_py, 6, 2, rgba(255, 255, 255, 255));
}

/* Helper: Render 2D GPS Circuit Map with Braking/Throttle Heatmap & Live Car Beacon */
static void draw_circuit_map_widget(SDL_Renderer *r, const gt7_state_t *state, int x, int y, int w, int h, rgba_t accent, bool detailed) {
    draw_panel(r, x, y, w, h, rgba(0, 0, 0, 255), accent, 14);
    char hdr[80];
    char short_circuit[64];
    snprintf(short_circuit, sizeof(short_circuit), "%s", state->circuit_name[0] ? state->circuit_name : "CIRCUIT");
    if (strlen(short_circuit) > 16) {
        short_circuit[14] = '.';
        short_circuit[15] = '.';
        short_circuit[16] = '\0';
    }
    snprintf(hdr, sizeof(hdr), "MAP: %s", short_circuit);
    draw_text(r, hdr, x + 18, y + 14, 2, accent);
    draw_text_right(r, "[TAP: SWITCH]", x + w - 18, y + 14, 2, rgba(255, 205, 0, 255));

    int pad_x = 36, pad_y = 48;
    int map_w = w - pad_x * 2;
    int map_h = h - pad_y - 36;
    int mx = x + pad_x;
    int my = y + pad_y;

    /* Tactical radar grid background */
    set_color(r, rgba(20, 26, 38, 255));
    for (int gx = mx; gx <= mx + map_w; gx += 40) SDL_RenderDrawLine(r, gx, my, gx, my + map_h);
    for (int gy = my; gy <= my + map_h; gy += 40) SDL_RenderDrawLine(r, mx, gy, mx + map_w, gy);

    draw_track_map_geometry(r, state, mx, my, map_w, map_h, detailed ? 8 : 5, detailed ? 4 : 2, true);

    char sec_str[64];
    snprintf(sec_str, sizeof(sec_str), "ACTIVE SECTOR: S%d   |   LAP PROGRESS: %02.0f%%",
             state->current_sector + 1, state->lap_progress * 100.0f);
    draw_text(r, sec_str, x + 18, y + h - 24, 2, rgba(215, 230, 250, 255));
}

/* =========================================================================
 * THEME 0: OFFICIAL GRAN TURISMO 7 NATIVE HUD & MFD STYLE (Polyphony Digital)
 * ========================================================================= */
static void render_theme_gt7_official(SDL_Renderer *r, const gt7_state_t *state, const hud_ui_state_t *ui, uint32_t now_ms, int top_y, int bot_y) {
    rgba_t gt_white  = rgba(248, 250, 255, 255);
    rgba_t gt_silver = rgba(165, 175, 192, 255);
    rgba_t gt_red    = rgba(235, 28, 46, 255);
    rgba_t gt_cyan   = rgba(0, 210, 255, 255);
    rgba_t gt_purple = rgba(198, 78, 255, 255);
    rgba_t gt_green  = rgba(30, 235, 120, 255);
    rgba_t gt_panel  = rgba(0, 0, 0, 255);

    int h_avail = bot_y - top_y;

    /* 1. Top-Left Official GT7 POSITION & LAP Counter Card */
    draw_panel(r, 18, top_y + 4, 345, 108, gt_panel, rgba(70, 82, 105, 255), 10);
    fill_rect(r, 18, top_y + 4, 8, 108, gt_red); /* GT7 signature red vertical ribbon */
    draw_text(r, "POS", 40, top_y + 18, 2, gt_silver);
    char pbuf[64];
    snprintf(pbuf, sizeof(pbuf), "%d", state->race_position > 0 ? state->race_position : 2);
    draw_text(r, pbuf, 40, top_y + 42, 6, gt_white);
    snprintf(pbuf, sizeof(pbuf), "/ %d", state->total_cars > 0 ? state->total_cars : 20);
    draw_text(r, pbuf, 118, top_y + 66, 3, gt_silver);

    SDL_RenderDrawLine(r, 198, top_y + 18, 198, top_y + 98);
    draw_text(r, "LAP", 216, top_y + 18, 2, gt_silver);
    snprintf(pbuf, sizeof(pbuf), "%d", state->current_lap > 0 ? state->current_lap : 5);
    draw_text(r, pbuf, 216, top_y + 42, 6, gt_white);
    snprintf(pbuf, sizeof(pbuf), "/ %d", state->total_laps > 0 ? state->total_laps : 15);
    draw_text(r, pbuf, 286, top_y + 66, 3, gt_silver);

    /* Official GT7 Car Category Tag (from Packet C) */
    if (state->car_category[0]) {
        fill_rect(r, 260, top_y + 14, 90, 20, rgba(20, 32, 50, 255));
        draw_rect(r, 260, top_y + 14, 90, 20, 1, gt_cyan);
        draw_text_centered(r, state->car_category, 305, top_y + 17, 1, gt_cyan);
    }

    /* 2. Top-Center GT7 Live Time Diff Pill & Sweeping Horizon RPM Arc */
    int d_w = 260, d_x = SCREEN_W / 2 - d_w / 2;
    draw_panel(r, d_x, top_y + 4, d_w, 46, rgba(0, 0, 0, 255),
               (state->delta_ms <= 0) ? gt_purple : gt_red, 8);
    snprintf(pbuf, sizeof(pbuf), "%s%.3f", (state->delta_ms <= 0) ? "-" : "+", fabsf((float)state->delta_ms / 1000.0f));
    draw_text_centered(r, pbuf, SCREEN_W / 2, top_y + 15, 3, (state->delta_ms <= 0) ? gt_purple : gt_red);

    /* 3. Top-Right Official GT7 Lap Timing Stack (TOTAL / CURRENT / BEST / LAST) */
    draw_panel(r, 912, top_y + 4, 350, 146, gt_panel, rgba(70, 82, 105, 255), 10);
    fill_rect(r, 1254, top_y + 4, 8, 146, gt_cyan);
    draw_text(r, "CURRENT LAP", 930, top_y + 18, 2, gt_cyan);
    format_lap_time(state->current_lap_ms, pbuf, sizeof(pbuf));
    draw_text_right(r, pbuf, 1242, top_y + 14, 3, gt_white);

    SDL_RenderDrawLine(r, 930, top_y + 56, 1242, top_y + 56);
    draw_text(r, "BEST LAP", 930, top_y + 68, 2, gt_silver);
    format_lap_time(state->best_lap_ms, pbuf, sizeof(pbuf));
    draw_text_right(r, pbuf, 1242, top_y + 64, 3, gt_purple);

    draw_text(r, "LAST LAP", 930, top_y + 110, 2, gt_silver);
    format_lap_time(state->last_lap_ms, pbuf, sizeof(pbuf));
    draw_text_right(r, pbuf, 1242, top_y + 106, 3, gt_white);

    /* 4. Bottom-Left Official GT7 Tire Wear/Temp + Fuel Gauge + Water/Oil Vitals */
    int bl_y = top_y + 126;
    int bl_h = h_avail - 130;
    draw_panel(r, 18, bl_y, 345, bl_h, gt_panel, rgba(70, 82, 105, 255), 12);
    draw_text(r, "GT7 TIRE & FUEL TELEMETRY", 36, bl_y + 14, 2, gt_silver);

    /* 4-Wheel GT7 style rounded tire icons with inner temperature and outer wear/slip ring */
    int t_cx = 122, t_cy = bl_y + 145;
    draw_rect(r, t_cx - 24, t_cy - 48, 48, 96, 2, rgba(65, 78, 100, 255));
    static const char *t_lbl[4] = {"FL", "FR", "RL", "RR"};
    int t_pos[4][2] = {
        {38, t_cy - 84}, {154, t_cy - 84},
        {38, t_cy + 10}, {154, t_cy + 10}
    };
    for (int i = 0; i < 4; ++i) {
        int tx = t_pos[i][0], ty = t_pos[i][1];
        rgba_t tc = tire_temp_color(state->tire_temp[i]);
        fill_rect(r, tx, ty, 52, 74, rgba(tc.r / 3, tc.g / 3, tc.b / 3, 255));
        fill_rect(r, tx + 4, ty + 4, 44, 66, tc);
        draw_rect(r, tx, ty, 52, 74, (state->tire_slip_ratio[i] > 0.12f) ? 3 : 1,
                  (state->tire_slip_ratio[i] > 0.12f) ? gt_red : gt_white);
        draw_text_centered(r, t_lbl[i], tx + 26, ty + 8, 2, rgba(10, 12, 18, 255));
        snprintf(pbuf, sizeof(pbuf), "%.0fC", state->tire_temp[i]);
        draw_text_centered(r, pbuf, tx + 26, ty + 32, 2, rgba(10, 12, 18, 255));

        /* Packet C Tire Surface Badge (Tarmac, Curb, Dirt) */
        char sc = (state->surface_type[i] != 0) ? (char)state->surface_type[i] : 'T';
        rgba_t sc_bg = (sc == 'C' || sc == 'c') ? rgba(245, 175, 20, 255) : ((sc == 'D' || sc == 'd' || sc == 'G' || sc == 'g') ? rgba(225, 45, 45, 255) : rgba(18, 26, 40, 255));
        rgba_t sc_fg = (sc == 'C' || sc == 'c') ? rgba(0, 0, 0, 255) : gt_white;
        fill_rect(r, tx + 12, ty + 50, 28, 16, sc_bg);
        char sc_str[4] = { sc, '\0' };
        draw_text_centered(r, sc_str, tx + 26, ty + 52, 1, sc_fg);
    }

    /* Right side of Bottom-Left card: Vertical GT7 Fuel Bar & Remaining Laps */
    int fx = 242, fy = bl_y + 58, fw = 30, fh = 166;
    fill_rect(r, fx, fy, fw, fh, rgba(28, 34, 46, 255));
    int f_fill = (int)((state->fuel_pct / 100.0f) * fh);
    fill_rect(r, fx, fy + fh - f_fill, fw, f_fill, (state->fuel_pct < 18.0f) ? gt_red : gt_cyan);
    draw_rect(r, fx, fy, fw, fh, 2, gt_silver);
    draw_text_centered(r, "FUEL", fx + 15, fy - 18, 2, gt_silver);
    snprintf(pbuf, sizeof(pbuf), "%.0f%%", state->fuel_pct);
    draw_text(r, pbuf, fx + 38, fy + fh - 22, 3, gt_white);

    float laps_rem = (state->fuel_burn_per_lap > 0.1f) ? (state->fuel_pct / state->fuel_burn_per_lap) : (state->fuel_pct / 3.2f);
    snprintf(pbuf, sizeof(pbuf), "%.1f", laps_rem);
    draw_text(r, "LAPS", fx + 38, fy + 10, 2, gt_silver);
    draw_text(r, pbuf, fx + 38, fy + 32, 3, (laps_rem < 2.0f) ? gt_red : gt_green);

    /* Bottom of Left Panel: G-Force & Water/Oil Temps */
    int gy_sep = bl_y + bl_h - 145;
    SDL_RenderDrawLine(r, 34, gy_sep, 345, gy_sep);
    snprintf(pbuf, sizeof(pbuf), "WATER: %.0f C     OIL: %.0f C", state->water_temp, state->oil_temp);
    draw_text(r, pbuf, 36, gy_sep + 14, 2, gt_silver);
    snprintf(pbuf, sizeof(pbuf), "OIL PRESS: %.1f BAR   RIDE: %.0fMM", state->oil_pressure, state->body_height);
    draw_text(r, pbuf, 36, gy_sep + 42, 2, gt_silver);

    /* GT7 Lateral/Longitudinal G-Force Ball Bar */
    draw_text(r, "G-LOAD", 36, gy_sep + 76, 2, gt_cyan);
    fill_rect(r, 36, gy_sep + 98, 308, 16, rgba(24, 30, 42, 255));
    SDL_RenderDrawLine(r, 190, gy_sep + 94, 190, gy_sep + 118);
    int g_bx = 190 + (int)((state->g_lat / 2.2f) * 140);
    fill_circle(r, g_bx, gy_sep + 106, 8, gt_cyan);
    snprintf(pbuf, sizeof(pbuf), "%+.1fG / %+.1fG", state->g_lat, state->g_lon);
    draw_text_right(r, pbuf, 344, gy_sep + 76, 2, gt_white);

    /* 5. Center Masterpiece: Official GT7 Curved Horizon RPM Bar + Speed + Gear + Suggested Gear + Turbo Gauge */
    int c_y = top_y + 62;
    int c_h = h_avail - 66;
    draw_panel(r, 378, c_y, 518, c_h, rgba(0, 0, 0, 255), rgba(85, 100, 128, 255), 18);

    /* GT7 Signature Curved RPM Bar across the upper arch of Center Pod */
    float max_rpm = (state->rpm_limiter > 1000) ? (float)state->rpm_limiter : 8800.0f;
    float warn_rpm = (state->rpm_warning > 1000) ? (float)state->rpm_warning : (max_rpm * 0.86f);
    float rpm_frac = state->engine_rpm / max_rpm;
    if (rpm_frac < 0.0f) rpm_frac = 0.0f;
    if (rpm_frac > 1.0f) rpm_frac = 1.0f;

    bool shift_flash = (state->flags & GT7_FLAG_REV_BLINK) || (state->engine_rpm >= warn_rpm);
    bool flash_phase = ((now_ms / 65) % 2) == 0;

    /* Redline Shift Flash Banner (Just like GT7's pulsing red/cyan shift bar!) */
    if (shift_flash && flash_phase) {
        fill_rect(r, 396, c_y + 12, 482, 26, gt_red);
        draw_text_centered(r, ">> SHIFT UP <<", 637, c_y + 18, 2, gt_white);
    } else {
        snprintf(pbuf, sizeof(pbuf), "%.0f RPM", state->engine_rpm);
        draw_text_centered(r, pbuf, 637, c_y + 16, 3, gt_silver);
    }

    int arc_segs = 46;
    int arc_lit = (int)(rpm_frac * arc_segs);
    for (int i = 0; i < arc_segs; ++i) {
        float norm = (float)i / (float)(arc_segs - 1);
        int sx = 404 + i * 10;
        /* Gentle upward arch curve like GT7 bumper HUD */
        int arch_offset = (int)(sinf(norm * (float)M_PI) * 24.0f);
        int sy = c_y + 72 - arch_offset;
        bool red_zone = (norm * max_rpm) >= warn_rpm;
        rgba_t seg_col = red_zone ? gt_red : gt_white;
        if (shift_flash && flash_phase) seg_col = gt_cyan;

        fill_rect(r, sx, sy, 8, 32, (i < arc_lit) ? seg_col : rgba(28, 34, 48, 255));
    }

    /* Scale numbers under the GT7 curved RPM bar */
    int max_k = (int)ceilf(max_rpm / 1000.0f);
    for (int k = 0; k <= max_k; ++k) {
        float knorm = (float)(k * 1000) / max_rpm;
        if (knorm > 1.0f) break;
        int kx = 408 + (int)(knorm * (arc_segs * 10));
        int arch_offset = (int)(sinf(knorm * (float)M_PI) * 24.0f);
        snprintf(pbuf, sizeof(pbuf), "%d", k);
        draw_text_centered(r, pbuf, kx, c_y + 110 - arch_offset, 2,
                           (k * 1000 >= (int)warn_rpm) ? gt_red : gt_silver);
    }

    /* GT7 Vertical Curved Brake (Left, Red) & Throttle (Right, White/Cyan) Arcs flanking Center Pod */
    int ped_y = c_y + 135;
    int ped_h = c_h - 210;
    /* Left Brake Pillar */
    fill_rect(r, 400, ped_y, 26, ped_h, rgba(32, 20, 24, 255));
    int b_fill = (int)((state->brake_pct / 100.0f) * ped_h);
    fill_rect(r, 400, ped_y + ped_h - b_fill, 26, b_fill, gt_red);
    int pk_y = ped_y + ped_h - (int)((state->peak_brake_pct / 100.0f) * ped_h);
    fill_rect(r, 398, pk_y, 30, 3, rgba(255, 210, 0, 255));
    draw_rect(r, 400, ped_y, 26, ped_h, 1, gt_silver);
    draw_text_centered(r, "BRK", 413, ped_y + ped_h + 8, 2, gt_red);

    /* Right Throttle Pillar */
    fill_rect(r, 848, ped_y, 26, ped_h, rgba(20, 28, 38, 255));
    int t_fill = (int)((state->throttle_pct / 100.0f) * ped_h);
    fill_rect(r, 848, ped_y + ped_h - t_fill, 26, t_fill, gt_cyan);
    draw_rect(r, 848, ped_y, 26, ped_h, 1, gt_silver);
    draw_text_centered(r, "THR", 861, ped_y + ped_h + 8, 2, gt_cyan);

    /* GT7 Center Turbo Boost Half-Moon Dial (Above Speed/Gear) */
    int t_dial_cx = 637, t_dial_cy = c_y + 185, t_dial_r = 46;
    draw_circle(r, t_dial_cx, t_dial_cy, t_dial_r, 3, rgba(60, 75, 98, 255));
    float b_clamped = state->turbo_bar;
    if (b_clamped < -1.0f) b_clamped = -1.0f;
    if (b_clamped > 2.0f)  b_clamped = 2.0f;
    float b_ang = (180.0f + ((b_clamped + 1.0f) / 3.0f) * 180.0f) * (float)M_PI / 180.0f;
    draw_thick_line(r, t_dial_cx, t_dial_cy,
                    t_dial_cx + (int)(cosf(b_ang) * (t_dial_r - 6)),
                    t_dial_cy + (int)(sinf(b_ang) * (t_dial_r - 6)), 4, gt_red);
    fill_circle(r, t_dial_cx, t_dial_cy, 6, gt_white);
    snprintf(pbuf, sizeof(pbuf), "BOOST %+.2f BAR", state->turbo_bar);
    draw_text_centered(r, pbuf, t_dial_cx, t_dial_cy + 14, 2, gt_silver);

    /* GT7 Center Digital Speed (Left-Center) & Gear + Blinking Red Suggested Gear Box (Right-Center) */
    int main_row_y = c_y + c_h - 245;
    int spd = (int)roundf(ui->use_mph ? state->speed_mph : state->speed_kmh);
    draw_7seg_number(r, spd, 3, false, 625, main_row_y + 18, 56, 98, 12, 10, gt_white, rgba(22, 28, 40, 255));
    draw_text_right(r, ui->use_mph ? "MPH" : "KM/H", 625, main_row_y + 128, 3, gt_silver);

    /* Current Gear */
    draw_text_centered(r, "GEAR", 698, main_row_y - 6, 2, gt_silver);
    draw_7seg_char(r, gear_to_char(state->current_gear), 660, main_row_y + 16, 76, 125, 15,
                   shift_flash ? gt_red : gt_white, rgba(25, 30, 42, 255));

    /* Official GT7 Red Pulsing Suggested Gear Box! */
    int sug = state->suggested_gear;
    bool has_sug = (sug >= 1 && sug <= 8 && sug != state->current_gear);
    int sbox_x = 752, sbox_y = main_row_y + 32;
    fill_rect(r, sbox_x, sbox_y, 78, 94, has_sug ? gt_red : rgba(25, 30, 42, 255));
    draw_rect(r, sbox_x, sbox_y, 78, 94, 2, has_sug ? gt_white : rgba(60, 70, 90, 255));
    draw_text_centered(r, "SUGG", sbox_x + 39, sbox_y + 8, 2, has_sug ? gt_white : gt_silver);
    if (has_sug) {
        draw_7seg_char(r, (char)('0' + sug), sbox_x + 19, sbox_y + 30, 40, 54, 7, gt_white, rgba(0, 0, 0, 0));
    } else {
        draw_text_centered(r, "-", sbox_x + 39, sbox_y + 46, 4, rgba(90, 100, 120, 255));
    }

    /* Bottom of Center Pod: Official GT7 Assist Indicators (TCS / ABS / ASM / HANDBRAKE / LIGHTS) */
    int ind_y = c_y + c_h - 56;
    bool tcs_act = (state->flags & GT7_FLAG_TCS_ACTIVE) != 0;
    bool asm_act = (state->flags & GT7_FLAG_ASM_ACTIVE) != 0;
    bool hbrk    = (state->flags & GT7_FLAG_HANDBRAKE) != 0;
    bool lights  = (state->flags & GT7_FLAG_LIGHTS_ACTIVE) != 0;

    fill_rect(r, 400, ind_y, 110, 38, tcs_act ? rgba(255, 185, 0, 255) : rgba(24, 30, 42, 255));
    draw_text_centered(r, "TCS", 455, ind_y + 12, 2, tcs_act ? rgba(10, 12, 16, 255) : gt_silver);

    fill_rect(r, 522, ind_y, 110, 38, asm_act ? rgba(255, 185, 0, 255) : rgba(24, 30, 42, 255));
    draw_text_centered(r, "ASM", 577, ind_y + 12, 2, asm_act ? rgba(10, 12, 16, 255) : gt_silver);

    fill_rect(r, 644, ind_y, 110, 38, hbrk ? gt_red : rgba(24, 30, 42, 255));
    draw_text_centered(r, "BRAKE", 699, ind_y + 12, 2, hbrk ? gt_white : gt_silver);

    fill_rect(r, 766, ind_y, 108, 38, lights ? gt_cyan : rgba(24, 30, 42, 255));
    draw_text_centered(r, "BEAM", 820, ind_y + 12, 2, lights ? rgba(10, 12, 16, 255) : gt_silver);

    /* 6. Bottom-Right Official GT7 Interactive MFD (Multi-Function Display)! */
    int mfd_y = top_y + 162;
    int mfd_h = h_avail - 166;
    draw_panel(r, 912, mfd_y, 350, mfd_h, gt_panel, gt_cyan, 14);
    fill_rect(r, 912, mfd_y, 350, 34, rgba(0, 185, 235, 45));
    draw_text(r, "GT7 MFD", 928, mfd_y + 10, 2, gt_cyan);
    draw_text_right(r, "[TAP MFD TO CYCLE]", 1246, mfd_y + 10, 2, rgba(255, 210, 0, 255));

    int page = state->mfd_page % 4;
    if (page == 0) {
        /* MFD Page 1: Circuit Radar & Sector Splits */
        draw_text(r, "PAGE 1/4: CIRCUIT RADAR & SECTORS", 928, mfd_y + 44, 2, gt_white);
        int map_x = 936, map_y = mfd_y + 72, map_w = 302, map_h = mfd_h - 175;
        fill_rect(r, map_x, map_y, map_w, map_h, rgba(0, 0, 0, 255));
        draw_rect(r, map_x, map_y, map_w, map_h, 1, rgba(45, 60, 85, 255));

        draw_track_map_geometry(r, state, map_x + 8, map_y + 8, map_w - 16, map_h - 16, 4, 2, false);

        /* Sector S1 / S2 / S3 mini row at bottom of MFD */
        int sec_y = mfd_y + mfd_h - 90;
        for (int s = 0; s < 3; ++s) {
            int bx = 930 + s * 106;
            fill_rect(r, bx, sec_y, 98, 72, rgba(20, 26, 38, 255));
            draw_rect(r, bx, sec_y, 98, 72, 1, (state->current_sector == s) ? gt_cyan : rgba(60, 72, 92, 255));
            snprintf(pbuf, sizeof(pbuf), "SECTOR %d", s + 1);
            draw_text_centered(r, pbuf, bx + 49, sec_y + 10, 2, gt_silver);
            format_sector_time(state->cur_sector_ms[s], pbuf, sizeof(pbuf));
            draw_text_centered(r, pbuf, bx + 49, sec_y + 40, 2, gt_white);
        }
    } else if (page == 1) {
        /* MFD Page 2: Official GT7 Fuel Map (1 Power .. 6 Lean) & Stint Computer */
        draw_text(r, "PAGE 2/4: FUEL MAP & ENDURANCE", 928, mfd_y + 44, 2, gt_cyan);
        draw_text_centered(r, "FUEL MAP MODE", 1087, mfd_y + 82, 2, gt_silver);
        draw_text(r, "POWER", 936, mfd_y + 112, 2, gt_green);
        draw_text_right(r, "LEAN", 1238, mfd_y + 112, 2, gt_cyan);

        for (int m = 1; m <= 6; ++m) {
            int mx = 936 + (m - 1) * 51;
            bool sel = (state->gt7_fuel_map == m);
            fill_rect(r, mx, mfd_y + 136, 45, 54, sel ? gt_cyan : rgba(26, 32, 46, 255));
            snprintf(pbuf, sizeof(pbuf), "%d", m);
            draw_text_centered(r, pbuf, mx + 22, mfd_y + 152, 3, sel ? rgba(10, 14, 20, 255) : gt_white);
        }

        snprintf(pbuf, sizeof(pbuf), "AVG CONSUMPTION: %.1f%% / LAP", state->fuel_burn_per_lap);
        draw_text(r, pbuf, 936, mfd_y + 216, 2, gt_white);
        snprintf(pbuf, sizeof(pbuf), "ESTIMATED RANGE: %.1f LAPS", laps_rem);
        draw_text(r, pbuf, 936, mfd_y + 248, 2, gt_green);
        snprintf(pbuf, sizeof(pbuf), "SESSION TOP SPEED: %.1f KM/H", state->session_top_speed_kmh);
        draw_text(r, pbuf, 936, mfd_y + 280, 2, gt_silver);
    } else if (page == 2) {
        /* MFD Page 3: Brake Balance & Live Pedal Telemetry Trace */
        draw_text(r, "PAGE 3/4: BRAKE BALANCE & TRACE", 928, mfd_y + 44, 2, gt_red);
        snprintf(pbuf, sizeof(pbuf), "BRAKE BIAS: %+d (%s)",
                 state->gt7_brake_bal, (state->gt7_brake_bal < 0) ? "FRONT" : "REAR");
        draw_text_centered(r, pbuf, 1087, mfd_y + 76, 2, gt_white);

        for (int b = -5; b <= 5; ++b) {
            int bx = 934 + (b + 5) * 28;
            bool sel = (state->gt7_brake_bal == b);
            fill_rect(r, bx, mfd_y + 102, 24, 28, sel ? gt_red : rgba(28, 34, 48, 255));
        }

        /* Live scrolling Throttle/Brake graph inside MFD */
        int gx = 932, gy = mfd_y + 148, gw = 310, gh = mfd_h - 170;
        fill_rect(r, gx, gy, gw, gh, rgba(8, 10, 16, 255));
        draw_rect(r, gx, gy, gw, gh, 1, rgba(50, 62, 84, 255));
        for (int i = 1; i < GT7_TRACE_LEN; ++i) {
            int idx0 = (state->trace_head + i - 1) % GT7_TRACE_LEN;
            int idx1 = (state->trace_head + i) % GT7_TRACE_LEN;
            int x0 = gx + ((i - 1) * gw) / (GT7_TRACE_LEN - 1);
            int x1 = gx + (i * gw) / (GT7_TRACE_LEN - 1);
            int ty0 = gy + gh - 2 - (int)((state->trace_throttle[idx0] / 100.0f) * (gh - 4));
            int ty1 = gy + gh - 2 - (int)((state->trace_throttle[idx1] / 100.0f) * (gh - 4));
            draw_thick_line(r, x0, ty0, x1, ty1, 2, gt_cyan);
            int by0 = gy + gh - 2 - (int)((state->trace_brake[idx0] / 100.0f) * (gh - 4));
            int by1 = gy + gh - 2 - (int)((state->trace_brake[idx1] / 100.0f) * (gh - 4));
            draw_thick_line(r, x0, by0, x1, by1, 2, gt_red);
        }
    } else {
        /* MFD Page 4: Traction Control (TCS 0..5) & Vehicle Dynamics */
        draw_text(r, "PAGE 4/4: TRACTION CONTROL (TCS)", 928, mfd_y + 44, 2, gt_purple);
        for (int t = 0; t <= 5; ++t) {
            int tx = 936 + t * 51;
            bool sel = (state->gt7_tcs_level == t);
            fill_rect(r, tx, mfd_y + 86, 45, 58, sel ? gt_purple : rgba(26, 32, 46, 255));
            snprintf(pbuf, sizeof(pbuf), "%d", t);
            draw_text_centered(r, pbuf, tx + 22, mfd_y + 104, 3, gt_white);
        }
        snprintf(pbuf, sizeof(pbuf), "PITCH: %+.1f DEG   ROLL: %+.1f DEG", state->pitch, state->roll);
        draw_text(r, pbuf, 936, mfd_y + 174, 2, gt_white);
        snprintf(pbuf, sizeof(pbuf), "FL SLIP: %.0f%%       FR SLIP: %.0f%%",
                 state->tire_slip_ratio[0] * 100.0f, state->tire_slip_ratio[1] * 100.0f);
        draw_text(r, pbuf, 936, mfd_y + 210, 2, gt_silver);
        snprintf(pbuf, sizeof(pbuf), "RL SLIP: %.0f%%       RR SLIP: %.0f%%",
                 state->tire_slip_ratio[2] * 100.0f, state->tire_slip_ratio[3] * 100.0f);
        draw_text(r, pbuf, 936, mfd_y + 240, 2, gt_silver);
    }
}

/* =========================================================================
 * THEME 1: HIGH-RES DIGITAL TACHOMETER & MOTORSPORT DDU (DIGI-TACH)
 * ========================================================================= */
static void render_theme_digitach(SDL_Renderer *r, const gt7_state_t *state, const hud_ui_state_t *ui, uint32_t now_ms, int top_y, int bot_y) {
    rgba_t neon_cyan   = rgba(0, 225, 255, 255);
    rgba_t neon_green  = rgba(0, 255, 120, 255);
    rgba_t neon_yellow = rgba(255, 220, 0, 255);
    rgba_t neon_orange = rgba(255, 130, 0, 255);
    rgba_t neon_red    = rgba(255, 35, 55, 255);
    rgba_t neon_purple = rgba(195, 75, 255, 255);
    rgba_t ddu_dark    = rgba(0, 0, 0, 255);
    rgba_t ddu_slot    = rgba(0, 0, 0, 255);
    rgba_t text_dim    = rgba(150, 165, 185, 255);
    rgba_t text_bright = rgba(245, 250, 255, 255);

    int content_y = top_y + 2;
    int content_h = bot_y - content_y - 2;

    /* 1. Staged Shift LED Array (20 LEDs across 1248px) */
    int num_leds = 20;
    int total_w = 1248;
    int start_x = (SCREEN_W - total_w) / 2;
    int gap = 6;
    int led_w = (total_w - gap * (num_leds - 1)) / num_leds;
    int led_h = (content_h > 620) ? 22 : 18;

    float max_rpm = (state->rpm_limiter > 1000) ? (float)state->rpm_limiter : 8500.0f;
    float start_rpm = max_rpm * 0.50f;
    float frac = (state->engine_rpm - start_rpm) / (max_rpm * 0.96f - start_rpm);
    if (frac < 0.0f) frac = 0.0f;
    if (frac > 1.0f) frac = 1.0f;
    int active_leds = (int)roundf(frac * num_leds);

    bool shift_blink = (state->flags & GT7_FLAG_REV_BLINK) || (state->engine_rpm >= max_rpm * 0.95f);
    bool blink_on = ((now_ms / 65) % 2) == 0;

    for (int i = 0; i < num_leds; ++i) {
        int lx = start_x + i * (led_w + gap);
        rgba_t col_on;
        if (i < 6)       col_on = neon_green;
        else if (i < 12) col_on = neon_yellow;
        else if (i < 16) col_on = neon_orange;
        else if (i < 18) col_on = neon_red;
        else             col_on = neon_cyan;

        if (shift_blink) {
            col_on = blink_on ? neon_cyan : neon_red;
        }

        bool lit = shift_blink ? true : (i < active_leds);
        fill_rect(r, lx, content_y, led_w, led_h, lit ? col_on : rgba(18, 22, 32, 255));
        if (lit) {
            fill_rect(r, lx + 2, content_y + 2, led_w - 4, led_h / 3, rgba(255, 255, 255, 110));
        }
        draw_rect(r, lx, content_y, led_w, led_h, 1, lit ? rgba(255, 255, 255, 160) : rgba(45, 54, 72, 255));
    }

    /* 2. Main High-Resolution Digital Tachometer Panel */
    int tach_panel_y = content_y + led_h + 8;
    int tach_panel_h = (content_h > 620) ? 68 : 58;
    draw_panel(r, 16, tach_panel_y, 1248, tach_panel_h, ddu_dark, neon_cyan, 10);

    /* Tachometer Label and Real-time RPM Numerical Display */
    draw_text(r, "TACHOMETER", 32, tach_panel_y + 8, 2, neon_cyan);
    char rpm_str[48];
    snprintf(rpm_str, sizeof(rpm_str), "%.0f RPM", state->engine_rpm);
    draw_text_right(r, rpm_str, 1240, tach_panel_y + 6, 3, shift_blink ? neon_red : text_bright);

    /* 50-Segment High-Resolution Digital Tachometer Bar */
    int tbar_x = 32;
    int tbar_y = tach_panel_y + 32;
    int tbar_w = 1216;
    int tbar_h = tach_panel_h - 40;
    int seg_count = 50;
    int seg_gap = 3;
    int seg_w = (tbar_w - (seg_count - 1) * seg_gap) / seg_count;

    float rpm_ratio = state->engine_rpm / max_rpm;
    if (rpm_ratio > 1.0f) rpm_ratio = 1.0f;
    int filled_segs = (int)roundf(rpm_ratio * seg_count);

    for (int s = 0; s < seg_count; ++s) {
        int sx = tbar_x + s * (seg_w + seg_gap);
        float seg_pct = (float)s / (float)seg_count;
        rgba_t scol;
        if (seg_pct < 0.60f)       scol = neon_green;
        else if (seg_pct < 0.80f)  scol = neon_yellow;
        else if (seg_pct < 0.92f)  scol = neon_orange;
        else                       scol = neon_red;

        if (shift_blink && s < filled_segs) {
            scol = blink_on ? neon_cyan : neon_red;
        }

        bool s_lit = (s < filled_segs);
        fill_rect(r, sx, tbar_y, seg_w, tbar_h, s_lit ? scol : rgba(22, 28, 40, 255));
        if (s_lit) {
            fill_rect(r, sx + 1, tbar_y + 1, seg_w - 2, tbar_h / 2, rgba(255, 255, 255, 80));
        }
        draw_rect(r, sx, tbar_y, seg_w, tbar_h, 1, s_lit ? rgba(255, 255, 255, 120) : rgba(36, 46, 62, 255));
    }

    /* RPM scale markings (1k to 10k) */
    for (int k = 1; k <= 10; ++k) {
        float r_val = (float)(k * 1000);
        if (r_val > max_rpm * 1.05f) break;
        int mk_x = tbar_x + (int)((r_val / max_rpm) * tbar_w);
        if (mk_x >= tbar_x && mk_x <= tbar_x + tbar_w) {
            set_color(r, rgba(70, 85, 115, 255));
            SDL_RenderDrawLine(r, mk_x, tbar_y - 4, mk_x, tbar_y - 1);
            char kstr[16];
            snprintf(kstr, sizeof(kstr), "%d", k);
            draw_text(r, kstr, mk_x - 3, tach_panel_y + 18, 1, text_dim);
        }
    }

    /* 3. Main Body: Left Box (Lap Timing & Sectors), Center Box (Cockpit Gear & Speed), Right Box (Tires & Vitals) */
    int body_y = tach_panel_y + tach_panel_h + 8;
    int body_h = content_h - (body_y - content_y);

    int col_w = 348;
    int center_w = 1248 - col_w * 2 - 16;
    int left_x = 16;
    int center_x = left_x + col_w + 8;
    int right_x = center_x + center_w + 8;

    /* ---------------- LEFT COLUMN: TIMING, DELTA & SECTORS ---------------- */
    draw_panel(r, left_x, body_y, col_w, body_h, ddu_dark, neon_cyan, 12);
    char buf[64];
    snprintf(buf, sizeof(buf), "LAP %02d",
             state->current_lap > 0 ? state->current_lap : 1);
    draw_text(r, buf, left_x + 16, body_y + 14, 3, neon_cyan);
    snprintf(buf, sizeof(buf), "P%d / %d",
             state->race_position > 0 ? state->race_position : 1,
             state->total_cars > 0 ? state->total_cars : 20);
    draw_text_right(r, buf, left_x + col_w - 16, body_y + 14, 3, neon_yellow);

    /* Delta Box */
    int delta_y = body_y + 48;
    fill_rect(r, left_x + 14, delta_y, col_w - 28, 52, ddu_slot);
    bool faster = (state->delta_ms <= 0);
    draw_rect(r, left_x + 14, delta_y, col_w - 28, 52, 2, faster ? neon_green : neon_red);
    draw_text(r, "DELTA", left_x + 24, delta_y + 17, 2, text_dim);
    snprintf(buf, sizeof(buf), "%s%.3f", (state->delta_ms <= 0) ? "-" : "+", fabsf((float)state->delta_ms / 1000.0f));
    draw_text_right(r, buf, left_x + col_w - 24, delta_y + 12, 4, faster ? neon_green : neon_red);

    /* Lap Times Matrix */
    int lap_box_y = delta_y + 60;
    draw_text(r, "CURRENT LAP", left_x + 18, lap_box_y + 4, 2, text_dim);
    format_lap_time(state->current_lap_ms, buf, sizeof(buf));
    draw_text_right(r, buf, left_x + col_w - 18, lap_box_y + 2, 3, text_bright);

    draw_text(r, "BEST LAP", left_x + 18, lap_box_y + 36, 2, text_dim);
    format_lap_time(state->best_lap_ms, buf, sizeof(buf));
    draw_text_right(r, buf, left_x + col_w - 18, lap_box_y + 34, 3, neon_purple);

    draw_text(r, "LAST LAP", left_x + 18, lap_box_y + 68, 2, text_dim);
    format_lap_time(state->last_lap_ms, buf, sizeof(buf));
    draw_text_right(r, buf, left_x + col_w - 18, lap_box_y + 66, 3, text_bright);

    /* Sector Splits (S1 / S2 / S3) */
    int sec_y = lap_box_y + 104;
    int sec_card_w = (col_w - 36) / 3;
    for (int s = 0; s < 3; ++s) {
        int scx = left_x + 14 + s * (sec_card_w + 4);
        bool cur = (state->current_sector == s);
        fill_rect(r, scx, sec_y, sec_card_w, 62, ddu_slot);
        draw_rect(r, scx, sec_y, sec_card_w, 62, 1, cur ? neon_cyan : rgba(50, 62, 85, 255));
        snprintf(buf, sizeof(buf), "S%d", s + 1);
        draw_text_centered(r, buf, scx + sec_card_w / 2, sec_y + 8, 2, cur ? neon_cyan : text_dim);
        format_sector_time(state->cur_sector_ms[s], buf, sizeof(buf));
        draw_text_centered(r, buf, scx + sec_card_w / 2, sec_y + 32, 2,
                           (state->cur_sector_ms[s] <= state->best_sector_ms[s]) ? neon_purple : text_bright);
    }

    /* Engine Vitals Footer in Left Box */
    int eng_y = sec_y + 72;
    fill_rect(r, left_x + 14, eng_y, col_w - 28, body_h - (eng_y - body_y) - 12, ddu_slot);
    draw_text(r, "WATER TEMP", left_x + 22, eng_y + 12, 2, text_dim);
    snprintf(buf, sizeof(buf), "%.0f C", state->water_temp);
    draw_text_right(r, buf, left_x + col_w - 22, eng_y + 12, 2, (state->water_temp > 105.0f) ? neon_red : neon_green);

    draw_text(r, "OIL TEMP", left_x + 22, eng_y + 38, 2, text_dim);
    snprintf(buf, sizeof(buf), "%.0f C", state->oil_temp);
    draw_text_right(r, buf, left_x + col_w - 22, eng_y + 38, 2, (state->oil_temp > 120.0f) ? neon_red : text_bright);

    draw_text(r, "OIL PRESS", left_x + 22, eng_y + 64, 2, text_dim);
    snprintf(buf, sizeof(buf), "%.1f BAR", state->oil_pressure);
    draw_text_right(r, buf, left_x + col_w - 22, eng_y + 64, 2, text_bright);

    if (state->flags & GT7_FLAG_HAS_TURBO) {
        draw_text(r, "BOOST / TURBO", left_x + 22, eng_y + 90, 2, text_dim);
        snprintf(buf, sizeof(buf), "%+.2f BAR", state->turbo_bar);
        draw_text_right(r, buf, left_x + col_w - 22, eng_y + 90, 2, neon_cyan);
    }

    /* ---------------- CENTER COLUMN: GIANT GEAR, SPEED & PEDALS ---------------- */
    draw_panel(r, center_x, body_y, center_w, body_h, ddu_dark, neon_cyan, 14);

    /* Suggested Gear Box */
    int sugg_w = 90, sugg_h = 104;
    int sugg_x = center_x + 24;
    int sugg_y = body_y + 16;
    fill_rect(r, sugg_x, sugg_y, sugg_w, sugg_h, rgba(0, 0, 0, 255));
    draw_rect(r, sugg_x, sugg_y, sugg_w, sugg_h, 2, neon_yellow);
    draw_text_centered(r, "SUGG", sugg_x + sugg_w / 2, sugg_y + 8, 2, neon_yellow);
    char s_char = (state->suggested_gear > 0 && state->suggested_gear <= 9) ? (char)('0' + state->suggested_gear) : '-';
    draw_7seg_char(r, s_char, sugg_x + (sugg_w - 42) / 2, sugg_y + 30, 42, 64, 7, neon_yellow, rgba(0, 0, 0, 0));

    /* Giant Center Current Gear */
    int gear_x = sugg_x + sugg_w + 18;
    int gear_y = body_y + 16;
    int gear_w = 175;
    int gear_h = (body_h > 460) ? 195 : 175;
    fill_rect(r, gear_x, gear_y, gear_w, gear_h, rgba(0, 0, 0, 255));
    rgba_t gear_border = shift_blink ? (blink_on ? neon_cyan : neon_red) : neon_cyan;
    draw_rect(r, gear_x, gear_y, gear_w, gear_h, 3, gear_border);
    draw_text_centered(r, "GEAR", gear_x + gear_w / 2, gear_y + 10, 2, neon_cyan);
    draw_7seg_char(r, gear_to_char(state->current_gear),
                   gear_x + (gear_w - 88) / 2, gear_y + 36, 88, gear_h - 50, 15,
                   text_bright, rgba(0, 0, 0, 0));

    /* Giant Digital Speedometer */
    int spd_x = gear_x + gear_w + 18;
    int spd_y = body_y + 16;
    int spd_w = center_x + center_w - 24 - spd_x;
    int spd_h = gear_h;
    fill_rect(r, spd_x, spd_y, spd_w, spd_h, rgba(0, 0, 0, 255));
    draw_rect(r, spd_x, spd_y, spd_w, spd_h, 2, rgba(60, 78, 105, 255));
    draw_text_centered(r, ui->use_mph ? "SPEED (MPH)" : "SPEED (KM/H)", spd_x + spd_w / 2, spd_y + 10, 2, neon_cyan);
    int cur_spd = (int)roundf(ui->use_mph ? state->speed_mph : state->speed_kmh);
    draw_7seg_number_centered(r, cur_spd, 3, false,
                              spd_x + spd_w / 2, spd_y + 36, 46, spd_h - 52, 9, 8,
                              text_bright, rgba(0, 0, 0, 0));

    /* Throttle & Brake Digital Bar Displays with Exact Real-time Percentages */
    int pedal_y = gear_y + gear_h + 16;
    int pedal_w = center_w - 48;
    int pedal_h = 24;

    /* Throttle Bar */
    draw_text(r, "THROTTLE", center_x + 24, pedal_y - 2, 2, neon_green);
    snprintf(buf, sizeof(buf), "%03.0f%%", state->throttle_pct);
    draw_text_right(r, buf, center_x + center_w - 24, pedal_y - 2, 2, neon_green);
    fill_rect(r, center_x + 24, pedal_y + 18, pedal_w, pedal_h, ddu_slot);
    int t_fill = (int)((state->throttle_pct / 100.0f) * pedal_w);
    fill_rect(r, center_x + 24, pedal_y + 18, t_fill, pedal_h, neon_green);
    draw_rect(r, center_x + 24, pedal_y + 18, pedal_w, pedal_h, 1, rgba(60, 95, 75, 255));

    /* Brake Bar with Peak Brake Hold Marker */
    int brk_y = pedal_y + 50;
    draw_text(r, "BRAKE", center_x + 24, brk_y - 2, 2, neon_red);
    snprintf(buf, sizeof(buf), "%03.0f%%  (PEAK: %03.0f%%)", state->brake_pct, state->peak_brake_pct);
    draw_text_right(r, buf, center_x + center_w - 24, brk_y - 2, 2, neon_red);
    fill_rect(r, center_x + 24, brk_y + 18, pedal_w, pedal_h, ddu_slot);
    int b_fill = (int)((state->brake_pct / 100.0f) * pedal_w);
    fill_rect(r, center_x + 24, brk_y + 18, b_fill, pedal_h, neon_red);
    int peak_bx = center_x + 24 + (int)((state->peak_brake_pct / 100.0f) * pedal_w);
    if (peak_bx > center_x + 24 + pedal_w - 3) peak_bx = center_x + 24 + pedal_w - 3;
    fill_rect(r, peak_bx, brk_y + 16, 4, pedal_h + 4, neon_yellow);
    draw_rect(r, center_x + 24, brk_y + 18, pedal_w, pedal_h, 1, rgba(110, 50, 60, 255));

    /* Fuel, Dynamics & Vehicle Telemetry Summary Strip */
    int fuel_strip_y = brk_y + 52;
    fill_rect(r, center_x + 24, fuel_strip_y, pedal_w, body_h - (fuel_strip_y - body_y) - 12, ddu_slot);
    draw_rect(r, center_x + 24, fuel_strip_y, pedal_w, body_h - (fuel_strip_y - body_y) - 12, 1, rgba(35, 50, 75, 255));

    /* Row 1: Fuel & Estimated Laps on left; Car Category Badge on right */
    draw_text(r, "FUEL", center_x + 36, fuel_strip_y + 10, 2, text_dim);
    snprintf(buf, sizeof(buf), "%.1f%%  (%.1f LAPS)", state->fuel_pct, (state->fuel_burn_per_lap > 0.1f) ? (state->fuel_pct / state->fuel_burn_per_lap) : (state->fuel_pct / 3.0f));
    draw_text(r, buf, center_x + 95, fuel_strip_y + 10, 2, (state->fuel_pct < 15.0f) ? neon_red : neon_green);

    if (state->car_category[0]) {
        fill_rect(r, center_x + pedal_w - 75, fuel_strip_y + 6, 85, 22, rgba(16, 28, 45, 255));
        draw_rect(r, center_x + pedal_w - 75, fuel_strip_y + 6, 85, 22, 1, neon_cyan);
        draw_text_centered(r, state->car_category, center_x + pedal_w - 32, fuel_strip_y + 9, 2, neon_cyan);
    }

    /* Row 2: 3-Axis Accelerations (G-Load, Sway, Heave, Surge) */
    snprintf(buf, sizeof(buf), "G: %.2fG  (LAT %+.2f  LON %+.2f)",
             sqrtf(state->g_lat * state->g_lat + state->g_lon * state->g_lon),
             state->g_lat, state->g_lon);
    draw_text(r, buf, center_x + 36, fuel_strip_y + 34, 2, text_bright);

    /* Row 3: Front Steering Angle, Wheel Rotational Velocity & Ride Height */
    snprintf(buf, sizeof(buf), "STEER: %+.1f DEG   RIDE: %.0fMM",
             state->steer_angle_fl_deg, state->body_height);
    draw_text(r, buf, center_x + 36, fuel_strip_y + 56, 2, neon_yellow);

    /* Row 4: Live EV/Hybrid Recovery or Downforce & Session Top Speed */
    if (state->has_packet_tilde || fabsf(state->energy_recovery_kw) > 1.0f) {
        snprintf(buf, sizeof(buf), "HYBRID: %+.0f KW   TOP: %.1f KM/H",
                 state->energy_recovery_kw, state->session_top_speed_kmh);
        draw_text(r, buf, center_x + 36, fuel_strip_y + 78, 2, neon_cyan);
    } else {
        snprintf(buf, sizeof(buf), "AERO: F %.0fK / R %.0fK   TOP: %.1fK",
                 state->downforce_front_kg, state->downforce_rear_kg, state->session_top_speed_kmh);
        draw_text(r, buf, center_x + 36, fuel_strip_y + 78, 2, rgba(160, 185, 215, 255));
    }

    /* ---------------- RIGHT COLUMN: 4 TIRES & GPS CIRCUIT RADAR ---------------- */
    draw_panel(r, right_x, body_y, col_w, body_h, ddu_dark, neon_cyan, 12);
    draw_text(r, "TIRE SURFACE TEMPERATURE", right_x + 16, body_y + 14, 2, neon_cyan);

    /* 4-Wheel Tire Digital Matrix */
    static const char *tlabels[4] = {"FRONT LEFT", "FRONT RIGHT", "REAR LEFT", "REAR RIGHT"};
    int tcard_w = (col_w - 36) / 2;
    int tcard_h = 72;
    int t_pos[4][2] = {
        {right_x + 14, body_y + 38},
        {right_x + 22 + tcard_w, body_y + 38},
        {right_x + 14, body_y + 116},
        {right_x + 22 + tcard_w, body_y + 116}
    };

    for (int t = 0; t < 4; ++t) {
        int tx = t_pos[t][0], ty = t_pos[t][1];
        rgba_t tcol = tire_temp_color(state->tire_temp[t]);
        fill_rect(r, tx, ty, tcard_w, tcard_h, ddu_slot);
        fill_rect(r, tx + 4, ty + 24, tcard_w - 8, tcard_h - 28, rgba(tcol.r / 3, tcol.g / 3, tcol.b / 3, 255));
        draw_rect(r, tx, ty, tcard_w, tcard_h, 1, (state->tire_slip_ratio[t] > 0.12f) ? neon_yellow : rgba(50, 65, 88, 255));

        draw_text(r, tlabels[t], tx + 8, ty + 6, 1, text_dim);

        /* Packet C Surface Type Badge */
        char sc = (state->surface_type[t] != 0) ? (char)state->surface_type[t] : 'T';
        rgba_t sc_bg = (sc == 'C' || sc == 'c') ? rgba(245, 175, 20, 255) : ((sc == 'D' || sc == 'd' || sc == 'G' || sc == 'g') ? rgba(225, 45, 45, 255) : rgba(20, 32, 50, 255));
        rgba_t sc_fg = (sc == 'C' || sc == 'c') ? rgba(0, 0, 0, 255) : text_bright;
        fill_rect(r, tx + tcard_w - 28, ty + 4, 22, 16, sc_bg);
        char sc_str[4] = { sc, '\0' };
        draw_text_centered(r, sc_str, tx + tcard_w - 17, ty + 6, 1, sc_fg);

        snprintf(buf, sizeof(buf), "%.0f C", state->tire_temp[t]);
        draw_text_centered(r, buf, tx + tcard_w / 2, ty + 30, 2, tcol);

        /* Slip ratio percentage */
        snprintf(buf, sizeof(buf), "SLIP: %.0f%%", state->tire_slip_ratio[t] * 100.0f);
        draw_text_centered(r, buf, tx + tcard_w / 2, ty + 50, 1, (state->tire_slip_ratio[t] > 0.10f) ? neon_yellow : text_dim);
    }

    /* Driver Assist Flags: ABS, TCS, ASM, FUEL MAP, BRAKE BALANCE */
    int ast_y = body_y + 196;
    int ast_w = (col_w - 36) / 3;
    bool abs_on = (state->flags & GT7_FLAG_ASM_ACTIVE) || (state->brake_pct > 80.0f);
    fill_rect(r, right_x + 14, ast_y, ast_w, 32, abs_on ? neon_yellow : ddu_slot);
    draw_rect(r, right_x + 14, ast_y, ast_w, 32, 1, abs_on ? text_bright : rgba(50, 65, 88, 255));
    draw_text_centered(r, "ABS", right_x + 14 + ast_w / 2, ast_y + 8, 2, abs_on ? rgba(10, 14, 20, 255) : text_dim);

    bool tcs_on = (state->flags & GT7_FLAG_TCS_ACTIVE) || (state->tire_slip_ratio[2] > 0.10f || state->tire_slip_ratio[3] > 0.10f);
    fill_rect(r, right_x + 18 + ast_w, ast_y, ast_w, 32, tcs_on ? neon_cyan : ddu_slot);
    draw_rect(r, right_x + 18 + ast_w, ast_y, ast_w, 32, 1, tcs_on ? text_bright : rgba(50, 65, 88, 255));
    snprintf(buf, sizeof(buf), "TCS %d", state->gt7_tcs_level);
    draw_text_centered(r, buf, right_x + 18 + ast_w + ast_w / 2, ast_y + 8, 2, tcs_on ? rgba(10, 14, 20, 255) : text_dim);

    fill_rect(r, right_x + 22 + ast_w * 2, ast_y, ast_w, 32, ddu_slot);
    draw_rect(r, right_x + 22 + ast_w * 2, ast_y, ast_w, 32, 1, rgba(50, 65, 88, 255));
    snprintf(buf, sizeof(buf), "BB %s%d", state->gt7_brake_bal >= 0 ? "F+" : "R", abs(state->gt7_brake_bal));
    draw_text_centered(r, buf, right_x + 22 + ast_w * 2 + ast_w / 2, ast_y + 8, 2, text_bright);

    /* Mini GPS Circuit Radar */
    int mrad_y = ast_y + 40;
    int mrad_h = body_h - (mrad_y - body_y) - 12;
    fill_rect(r, right_x + 14, mrad_y, col_w - 28, mrad_h, rgba(8, 12, 18, 255));
    draw_rect(r, right_x + 14, mrad_y, col_w - 28, mrad_h, 1, rgba(50, 65, 88, 255));
    draw_text(r, "CIRCUIT RADAR", right_x + 22, mrad_y + 8, 2, neon_cyan);
    draw_track_map_geometry(r, state, right_x + 20, mrad_y + 26, col_w - 40, mrad_h - 32, 4, 2, false);
}

/* =========================================================================
 * THEME 2: LUXURY EV DIGITAL COCKPIT (Mercedes EQ / BMW i / Audi / Volvo)
 * ========================================================================= */
static void draw_ev_brand_selector(SDL_Renderer *r, const hud_ui_state_t *ui, int top_y) {
    static const char *brands[EV_STYLE_COUNT] = {
        "[+] BENZ MBUX EQ",
        "[#] BMW i-CURVED M",
        "[O] AUDI e-tron GT",
        "[*] VOLVO EX90 / POLESTAR"
    };

    static const rgba_t brand_colors[EV_STYLE_COUNT] = {
        {0, 225, 255, 255},   /* Benz EQ Cyan */
        {0, 160, 255, 255},   /* BMW Electric Blue */
        {235, 45, 55, 255},   /* Audi Sport Red */
        {180, 225, 255, 255}  /* Volvo Scandinavian Ice Blue */
    };

    int bar_y = top_y + 4;
    int bar_h = 32;
    int tab_w = 302;
    int gap = 8;
    int start_x = 24;

    for (int i = 0; i < EV_STYLE_COUNT; ++i) {
        int tx = start_x + i * (tab_w + gap);
        bool active = ((int)ui->ev_style == i);
        rgba_t col = brand_colors[i];

        if (active) {
            fill_rect(r, tx, bar_y, tab_w, bar_h, rgba(col.r / 6, col.g / 6, col.b / 6, 255));
            draw_rect(r, tx, bar_y, tab_w, bar_h, 2, col);
            draw_text_centered(r, brands[i], tx + tab_w / 2, bar_y + 9, 2, col);
            fill_rect(r, tx + 8, bar_y + bar_h - 3, tab_w - 16, 3, col);
        } else {
            fill_rect(r, tx, bar_y, tab_w, bar_h, rgba(0, 0, 0, 255));
            draw_rect(r, tx, bar_y, tab_w, bar_h, 1, rgba(40, 52, 70, 255));
            draw_text_centered(r, brands[i], tx + tab_w / 2, bar_y + 9, 2, rgba(140, 155, 175, 255));
        }
    }
}

/* Helper: Draw simple 1px line */
static void draw_line(SDL_Renderer *r, int x1, int y1, int x2, int y2, rgba_t c) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_RenderDrawLine(r, x1, y1, x2, y2);
}

/* Helper: Draw high-contrast Speed Limit sign (European red circle or US rectangle) */
static void draw_speed_limit_sign(SDL_Renderer *r, int cx, int cy, int limit, bool us_style) {
    if (us_style) {
        fill_rect(r, cx - 18, cy - 22, 36, 44, rgba(240, 245, 250, 255));
        draw_rect(r, cx - 18, cy - 22, 36, 44, 2, rgba(20, 20, 20, 255));
        draw_text_centered(r, "LIMIT", cx, cy - 17, 1, rgba(20, 20, 20, 255));
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", limit);
        draw_text_centered(r, buf, cx, cy - 2, 2, rgba(20, 20, 20, 255));
    } else {
        fill_circle(r, cx, cy, 18, rgba(240, 245, 250, 255));
        draw_circle(r, cx, cy, 18, 4, rgba(225, 30, 35, 255));
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", limit);
        draw_text_centered(r, buf, cx, cy - 8, 2, rgba(20, 20, 20, 255));
    }
}

/* Helper: Draw Mercedes MBUX / Classic Dial Radial Sunburst Rays */
static void draw_radial_sunburst_rays(SDL_Renderer *r, int cx, int cy, int r_inner, int r_outer, int count, rgba_t color) {
    for (int i = 0; i < count; ++i) {
        float deg = (float)i * (360.0f / (float)count);
        float rad = deg * 3.14159265f / 180.0f;
        int x0 = cx + (int)(r_inner * cosf(rad));
        int y0 = cy + (int)(r_inner * sinf(rad));
        int x1 = cx + (int)(r_outer * cosf(rad));
        int y1 = cy + (int)(r_outer * sinf(rad));
        draw_line(r, x0, y0, x1, y1, color);
    }
}

/* Helper: Draw glowing needle with dynamic vibrant trailing arc */
static void draw_needle_with_trail(SDL_Renderer *r, int cx, int cy, float deg_start, float deg_curr, int length, int trail_rad, rgba_t needle_col, rgba_t trail_col) {
    if (deg_curr > deg_start + 1.0f) {
        draw_segmented_arc(r, cx, cy, trail_rad, 4, deg_start, deg_curr, 32, 1.0f, trail_col, rgba(0, 0, 0, 0));
    }
    float rad = deg_curr * 3.14159265f / 180.0f;
    float c = cosf(rad);
    float s = sinf(rad);
    int r_hub = 18;
    int x0 = cx + (int)(r_hub * c);
    int y0 = cy + (int)(r_hub * s);
    int tip_x = cx + (int)(length * c);
    int tip_y = cy + (int)(length * s);

    /* Slender tapered glowing laser needle with soft bloom */
    draw_thick_line(r, x0, y0, tip_x, tip_y, 4, rgba(needle_col.r, needle_col.g, needle_col.b, 90));
    draw_thick_line(r, x0, y0, tip_x, tip_y, 2, needle_col);

    /* Center Hub Cap */
    fill_circle(r, cx, cy, 18, rgba(8, 12, 18, 255));
    draw_circle(r, cx, cy, 18, 2, rgba(220, 235, 255, 240));
    draw_circle(r, cx, cy, 8, 1, needle_col);
    fill_circle(r, cx, cy, 4, needle_col);
}

/* Helper: Draw radial tick marks with numerical graduation labels */
static void draw_radial_dial_ticks(SDL_Renderer *r, int cx, int cy, int r_in, int r_out, float start_deg, float end_deg, int num_ticks, int major_step, rgba_t col_major, rgba_t col_minor) {
    if (num_ticks <= 1) return;
    float step = (end_deg - start_deg) / (float)(num_ticks - 1);
    for (int i = 0; i < num_ticks; ++i) {
        float deg = start_deg + i * step;
        float rad = deg * 3.14159265f / 180.0f;
        float c = cosf(rad);
        float s = sinf(rad);
        bool is_major = (major_step > 0) && (i % major_step == 0);
        int cur_in = is_major ? (r_in - 6) : r_in;
        int x0 = cx + (int)(cur_in * c);
        int y0 = cy + (int)(cur_in * s);
        int x1 = cx + (int)(r_out * c);
        int y1 = cy + (int)(r_out * s);
        draw_thick_line(r, x0, y0, x1, y1, is_major ? 3 : 1, is_major ? col_major : col_minor);
    }
}

/* =========================================================================
 * VECTOR ICON DRAWING PRIMITIVES (Zero UTF-8 font corruption, 100% crisp)
 * ========================================================================= */

/* Top-down Car Silhouette with seats / seatbelt indicators */
static void draw_icon_car_topdown(SDL_Renderer *r, int cx, int cy, int w, int h, rgba_t body_col, rgba_t seat_alert_col, bool alert_front_right, bool alert_rear_mid) {
    int x0 = cx - w / 2;
    int y0 = cy - h / 2;
    /* Outer chassis body */
    draw_panel(r, x0, y0, w, h, rgba(10, 18, 30, 255), body_col, 5);
    /* Windshield */
    int ws_y = y0 + (int)(h * 0.22f);
    draw_line(r, x0 + 4, ws_y, x0 + w - 5, ws_y, body_col);
    /* Rear window */
    int rw_y = y0 + (int)(h * 0.72f);
    draw_line(r, x0 + 4, rw_y, x0 + w - 5, rw_y, body_col);
    /* Wheels (4 dark nubs) */
    fill_rect(r, x0 - 2, y0 + 3, 2, 6, rgba(50, 70, 95, 255));
    fill_rect(r, x0 + w, y0 + 3, 2, 6, rgba(50, 70, 95, 255));
    fill_rect(r, x0 - 2, y0 + h - 9, 2, 6, rgba(50, 70, 95, 255));
    fill_rect(r, x0 + w, y0 + h - 9, 2, 6, rgba(50, 70, 95, 255));
    /* Passenger Seats */
    /* Front Driver (left) */
    fill_rect(r, cx - 6, cy - 3, 4, 5, rgba(60, 220, 120, 255));
    /* Front Passenger (right) */
    fill_rect(r, cx + 2, cy - 3, 4, 5, alert_front_right ? seat_alert_col : rgba(60, 220, 120, 255));
    /* Rear Seats (3 seats) */
    fill_rect(r, cx - 7, cy + 5, 3, 4, rgba(60, 220, 120, 255));
    fill_rect(r, cx - 1, cy + 5, 3, 4, alert_rear_mid ? seat_alert_col : rgba(60, 220, 120, 255));
    fill_rect(r, cx + 5, cy + 5, 3, 4, rgba(60, 220, 120, 255));
}

/* Seatbelt Warning Icon */
static void draw_icon_seatbelt(SDL_Renderer *r, int cx, int cy, rgba_t col) {
    /* Head */
    fill_circle(r, cx, cy - 6, 3, col);
    /* Body */
    draw_line(r, cx - 4, cy + 6, cx + 4, cy + 6, col);
    draw_line(r, cx - 4, cy + 6, cx - 2, cy - 1, col);
    draw_line(r, cx + 4, cy + 6, cx + 2, cy - 1, col);
    /* Diagonal Seatbelt sash */
    draw_thick_line(r, cx - 5, cy - 2, cx + 4, cy + 6, 2, col);
}

/* Steering Wheel Assist Icon (MBUX / BMW ADAS) */
static void draw_icon_steering_wheel(SDL_Renderer *r, int cx, int cy, int rad, rgba_t col) {
    draw_circle(r, cx, cy, rad, 2, col);
    fill_circle(r, cx, cy, 3, col);
    /* Left, Right, Bottom spokes */
    draw_line(r, cx - rad + 2, cy, cx - 3, cy, col);
    draw_line(r, cx + 3, cy, cx + rad - 2, cy, col);
    draw_line(r, cx, cy + 3, cx, cy + rad - 2, col);
}

/* Mercedes MBUX Dual-Leaf Eco Icon */
static void draw_icon_leaf(SDL_Renderer *r, int cx, int cy, rgba_t col) {
    /* Left leaf curve */
    draw_thick_line(r, cx - 8, cy - 4, cx, cy + 5, 2, col);
    draw_thick_line(r, cx - 8, cy - 4, cx - 2, cy - 1, 2, col);
    draw_thick_line(r, cx - 2, cy - 1, cx, cy + 5, 2, col);
    /* Right leaf curve */
    draw_thick_line(r, cx + 8, cy - 6, cx, cy + 5, 2, col);
    draw_thick_line(r, cx + 8, cy - 6, cx + 3, cy - 2, 2, col);
    draw_thick_line(r, cx + 3, cy - 2, cx, cy + 5, 2, col);
    /* Stem */
    draw_line(r, cx, cy + 5, cx, cy + 9, col);
}

/* Fuel Pump Icon */
static void draw_icon_fuel_pump(SDL_Renderer *r, int x, int y, rgba_t col) {
    /* Dispenser box */
    draw_rect(r, x, y, 10, 13, 2, col);
    /* Window in pump */
    draw_line(r, x + 2, y + 3, x + 8, y + 3, col);
    /* Hose */
    draw_line(r, x + 10, y + 3, x + 13, y + 5, col);
    draw_line(r, x + 13, y + 5, x + 13, y + 10, col);
    /* Nozzle */
    draw_line(r, x + 13, y + 10, x + 11, y + 12, col);
}

/* High-Voltage EV Battery Icon */
static void draw_icon_battery(SDL_Renderer *r, int x, int y, int w, int h, float pct, rgba_t col) {
    draw_rect(r, x, y, w, h, 2, col);
    /* Terminal nub */
    fill_rect(r, x + w, y + h / 4, 2, h / 2, col);
    /* Fill segments */
    int fill_w = (int)((w - 4) * pct);
    if (fill_w > 0) {
        fill_rect(r, x + 2, y + 2, fill_w, h - 4, col);
    }
}

/* Headlight Beam Icon */
static void draw_icon_headlight(SDL_Renderer *r, int cx, int cy, rgba_t col) {
    /* Lens dome (left side) */
    draw_circle(r, cx - 2, cy, 6, 2, col);
    fill_rect(r, cx - 2, cy - 7, 7, 14, rgba(0, 0, 0, 255));
    draw_line(r, cx - 2, cy - 6, cx - 2, cy + 6, col);
    /* 3 Rays */
    draw_line(r, cx + 2, cy - 4, cx + 10, cy - 4, col);
    draw_line(r, cx + 2, cy,     cx + 10, cy,     col);
    draw_line(r, cx + 2, cy + 4, cx + 10, cy + 4, col);
}

/* Snowflake Temperature Icon */
static void draw_icon_snowflake(SDL_Renderer *r, int cx, int cy, rgba_t col) {
    /* Cross */
    draw_line(r, cx, cy - 6, cx, cy + 6, col);
    draw_line(r, cx - 6, cy, cx + 6, cy, col);
    /* Diagonals */
    draw_line(r, cx - 4, cy - 4, cx + 4, cy + 4, col);
    draw_line(r, cx - 4, cy + 4, cx + 4, cy - 4, col);
    /* Center point */
    fill_circle(r, cx, cy, 1, col);
}

/* Lane Assist Active Icon (ADAS) */
static void draw_icon_lane_assist(SDL_Renderer *r, int cx, int cy, rgba_t col) {
    /* Left lane line */
    draw_thick_line(r, cx - 14, cy + 8, cx - 7, cy - 8, 2, col);
    /* Right lane line */
    draw_thick_line(r, cx + 14, cy + 8, cx + 7, cy - 8, 2, col);
    /* Small car centered in lane */
    draw_panel(r, cx - 4, cy - 4, 8, 8, rgba(0, 0, 0, 0), col, 2);
}

/* Radar Cruise Distance Icon */
static void draw_icon_radar_distance(SDL_Renderer *r, int cx, int cy, rgba_t col) {
    /* Lead car */
    draw_panel(r, cx - 14, cy - 5, 8, 10, rgba(0, 0, 0, 0), col, 2);
    /* Radar waves */
    draw_line(r, cx - 2, cy - 4, cx, cy, col);
    draw_line(r, cx, cy, cx - 2, cy + 4, col);
    draw_line(r, cx + 2, cy - 6, cx + 5, cy, col);
    draw_line(r, cx + 5, cy, cx + 2, cy + 6, col);
    /* Trailing car */
    draw_panel(r, cx + 8, cy - 5, 8, 10, rgba(0, 0, 0, 0), col, 2);
}

/* 1. MERCEDES-BENZ MBUX CLASSIC COCKPIT (Dual 492px Dials, Horizon Wave, "From Reset" Card) */
static void render_ev_benz_eq(SDL_Renderer *r, const gt7_state_t *state, const hud_ui_state_t *ui, uint32_t now_ms, int content_y, int content_h) {
    (void)now_ms;
    rgba_t text_wh   = rgba(245, 250, 255, 255);
    rgba_t text_dim  = rgba(140, 160, 185, 255);
    rgba_t mb_blue   = rgba(0, 140, 255, 255);
    rgba_t mb_glow   = rgba(0, 110, 240, 85);
    rgba_t mb_red    = rgba(245, 45, 65, 255);
    rgba_t mb_cyan   = rgba(0, 225, 255, 255);
    rgba_t mb_green  = rgba(60, 220, 120, 255);

    /* Ambient Electric Horizon Wave in center */
    for (int y = content_y + 80; y <= content_y + 360; y += 2) {
        float dist = fabsf((float)(y - (content_y + 220))) / 140.0f;
        if (dist < 1.0f) {
            float intensity = 1.0f - dist * dist;
            uint8_t a = (uint8_t)(intensity * 85.0f);
            draw_thick_line(r, 220, y, 1060, y, 2, rgba(0, 130, 255, a));
            if (dist < 0.35f) {
                uint8_t a2 = (uint8_t)((1.0f - dist / 0.35f) * 60.0f);
                draw_thick_line(r, 340, y, 940, y, 2, rgba(0, 225, 255, a2));
            }
        }
    }

    float speed = ui->use_mph ? state->speed_mph : state->speed_kmh;
    float max_spd = ui->use_mph ? 160.0f : 260.0f;
    float spd_frac = speed / max_spd;
    if (spd_frac > 1.0f) spd_frac = 1.0f;

    float rpm = state->engine_rpm;
    float max_rpm = 8000.0f;
    float rpm_frac = rpm / max_rpm;
    if (rpm_frac > 1.0f) rpm_frac = 1.0f;

    int cy = content_y + content_h / 2;
    int dial_rad = 246; /* 492px diameter, filling screen height */

    /* ================= LEFT DIAL: Mercedes Speedometer ================= */
    int left_cx = 246;
    fill_circle(r, left_cx, cy, dial_rad, rgba(0, 0, 0, 255));
    draw_circle(r, left_cx, cy, dial_rad, 2, rgba(235, 245, 255, 255));
    draw_circle(r, left_cx, cy, dial_rad - 2, 1, mb_glow);

    /* Radial Sunburst Rays */
    draw_radial_sunburst_rays(r, left_cx, cy, 32, dial_rad - 26, 72, rgba(30, 75, 125, 80));

    /* Graduation Ticks: outer ticks at perimeter */
    draw_radial_dial_ticks(r, left_cx, cy, dial_rad - 22, dial_rad - 6, 135.0f, 405.0f, 33, 4, rgba(240, 245, 255, 255), rgba(70, 95, 130, 255));

    /* Dynamic Glowing Red Needle & Arc Trail (rendered FIRST so numbers sit on top) */
    float spd_deg = 135.0f + spd_frac * 270.0f;
    draw_needle_with_trail(r, left_cx, cy, 135.0f, spd_deg, dial_rad - 22, dial_rad - 6, mb_red, rgba(255, 45, 75, 180));

    /* Numbers: 0, 20, 40, 60, 80, 100, 120, 140, 160 (staggered inside ticks/trail) */
    static const int mb_spds[9] = {0, 20, 40, 60, 80, 100, 120, 140, 160};
    for (int i = 0; i < 9; ++i) {
        float deg = 135.0f + (float)i * 33.75f;
        float rad = deg * 3.14159265f / 180.0f;
        int tx = left_cx + (int)((dial_rad - 54) * cosf(rad));
        int ty = cy + (int)((dial_rad - 54) * sinf(rad));
        char lbuf[8];
        snprintf(lbuf, sizeof(lbuf), "%d", mb_spds[i]);
        draw_text_centered(r, lbuf, tx, ty - 6, 2, text_wh);
    }
    draw_text_centered(r, ui->use_mph ? "mph" : "km/h", left_cx, cy - 80, 2, text_dim);

    /* Dynamic Fuel Gauge at bottom of left dial */
    int fuel_y = cy + dial_rad - 56;
    int cur_range = (int)(state->fuel_pct * (ui->use_mph ? 3.8f : 6.1f));
    char f_lbl[24];
    snprintf(f_lbl, sizeof(f_lbl), "%d %s", cur_range, ui->use_mph ? "mi" : "km");
    draw_icon_fuel_pump(r, left_cx - 42, fuel_y - 12, text_wh);
    draw_text(r, f_lbl, left_cx - 28, fuel_y - 10, 1, text_wh);
    draw_text(r, "0", left_cx - 48, fuel_y + 8, 1, text_dim);
    draw_text(r, "1/2", left_cx - 8, fuel_y + 14, 1, text_dim);
    draw_text(r, "1", left_cx + 36, fuel_y + 8, 1, text_dim);
    draw_segmented_arc(r, left_cx, cy, dial_rad - 16, 4, 250.0f, 290.0f, 10, state->fuel_pct / 100.0f, text_wh, rgba(40, 60, 85, 255));

    /* ================= RIGHT DIAL: Mercedes Tachometer & Power ================= */
    int right_cx = SCREEN_W - 246;
    fill_circle(r, right_cx, cy, dial_rad, rgba(0, 0, 0, 255));
    draw_circle(r, right_cx, cy, dial_rad, 2, rgba(235, 245, 255, 255));
    draw_circle(r, right_cx, cy, dial_rad - 2, 1, mb_glow);

    /* Radial Sunburst Rays */
    draw_radial_sunburst_rays(r, right_cx, cy, 32, dial_rad - 26, 72, rgba(30, 75, 125, 80));

    /* Graduation Ticks */
    draw_radial_dial_ticks(r, right_cx, cy, dial_rad - 22, dial_rad - 6, 135.0f, 405.0f, 33, 4, rgba(240, 245, 255, 255), rgba(70, 95, 130, 255));

    /* Dynamic Glowing Red Needle & Arc Trail (rendered FIRST so numbers sit on top) */
    float rpm_deg = 135.0f + rpm_frac * 270.0f;
    draw_needle_with_trail(r, right_cx, cy, 135.0f, rpm_deg, dial_rad - 22, dial_rad - 6, mb_red, rgba(255, 45, 75, 180));

    /* Numbers: 0, 1, 2, 3, 4, 5, 6, 7, 8 (staggered inside ticks/trail) */
    for (int i = 0; i <= 8; ++i) {
        float deg = 135.0f + (float)i * 33.75f;
        float rad = deg * 3.14159265f / 180.0f;
        int tx = right_cx + (int)((dial_rad - 54) * cosf(rad));
        int ty = cy + (int)((dial_rad - 54) * sinf(rad));
        char lbuf[8];
        snprintf(lbuf, sizeof(lbuf), "%d", i);
        draw_text_centered(r, lbuf, tx, ty - 6, 2, (i >= 7) ? mb_red : text_wh);
    }
    draw_text_centered(r, "x1000/min", right_cx, cy - 80, 1, text_dim);

    /* POWER & CHARGE readouts at bottom of right dial */
    int pwr_lbl_y = cy + dial_rad - 48;
    draw_text(r, "POWER", right_cx - 48, pwr_lbl_y, 1, text_wh);
    draw_text(r, "CHARGE", right_cx + 12, pwr_lbl_y, 1, mb_cyan);
    if (state->energy_recovery_kw < -1.0f) {
        char kw_buf[16];
        snprintf(kw_buf, sizeof(kw_buf), "%d kW", (int)fabsf(state->energy_recovery_kw));
        draw_text_centered(r, kw_buf, right_cx + 34, pwr_lbl_y + 14, 1, mb_cyan);
    } else if (state->energy_recovery_kw > 1.0f) {
        char kw_buf[16];
        snprintf(kw_buf, sizeof(kw_buf), "%d kW", (int)state->energy_recovery_kw);
        draw_text_centered(r, kw_buf, right_cx - 28, pwr_lbl_y + 14, 1, text_wh);
    }

    /* Dynamic Vertical Gear Selector Column & Driving Mode Pill (outside right dial) */
    int gr_x = 1208;
    int gr_y = content_y + content_h - 48;
    bool is_p = (state->speed_kmh < 1.0f && state->current_gear <= 0);
    bool is_r = (state->current_gear == 0);
    bool is_n = (state->current_gear == -1);
    bool is_d = (state->current_gear >= 1);

    draw_text(r, "R", gr_x, gr_y - 24, 1, is_r ? rgba(255, 195, 45, 255) : text_dim);
    draw_text(r, "N", gr_x, gr_y - 12, 1, is_n ? text_wh : text_dim);
    draw_text(r, "D", gr_x, gr_y,      1, is_d ? text_wh : text_dim);

    if (is_p) {
        draw_modern_char(r, 'P', gr_x + 14, gr_y - 22, 16, 24, 4, text_wh);
    } else if (is_r) {
        draw_modern_char(r, 'R', gr_x + 14, gr_y - 22, 16, 24, 4, rgba(255, 195, 45, 255));
    } else if (is_n) {
        draw_modern_char(r, 'N', gr_x + 14, gr_y - 22, 16, 24, 4, text_wh);
    } else {
        draw_modern_char(r, 'D', gr_x + 12, gr_y - 22, 14, 24, 4, text_wh);
        draw_modern_char(r, '0' + state->current_gear, gr_x + 28, gr_y - 20, 12, 20, 3, mb_cyan);
    }

    /* Dynamic Drive Mode Pill: [ S ] when throttle > 75%, otherwise [ C ] */
    bool is_sport = (state->throttle_pct > 75.0f);
    fill_rect(r, gr_x + 10, gr_y + 10, 32, 18, is_sport ? rgba(90, 15, 25, 255) : rgba(0, 45, 80, 255));
    draw_rect(r, gr_x + 10, gr_y + 10, 32, 18, 1, is_sport ? mb_red : mb_cyan);
    draw_text_centered(r, is_sport ? "S" : "C", gr_x + 26, gr_y + 12, 1, is_sport ? mb_red : mb_cyan);

    /* ================= CENTER TOP: Speed Limit, ADAS, Digital Speed ================= */
    int mid_cx = SCREEN_W / 2;
    int top_row_y = content_y + 4;

    /* Speed Limit Sign: LIMIT 50 */
    draw_speed_limit_sign(r, mid_cx - 150, top_row_y + 20, 50, true);

    /* Radar Cruise Distance Icon */
    draw_icon_radar_distance(r, mid_cx - 95, top_row_y + 20, text_dim);

    /* Center Modern Digital Speed */
    int ispd = (int)roundf(speed);
    draw_modern_number_centered(r, ispd, 1, false, mid_cx, top_row_y + 6, 26, 42, 6, 6, text_wh);
    draw_text_centered(r, ui->use_mph ? "mph" : "km/h", mid_cx, top_row_y + 52, 1, text_dim);

    /* Steering & Lane Assist Icons */
    draw_icon_steering_wheel(r, mid_cx + 80, top_row_y + 20, 10, mb_green);
    draw_icon_lane_assist(r, mid_cx + 130, top_row_y + 20, text_dim);

    /* Red Seatbelt Warning at top center */
    draw_icon_seatbelt(r, mid_cx, top_row_y - 2, mb_red);

    /* ================= CENTER FLOATING CARD: "From Reset" / "ENTIRE" ================= */
    int card_w = 268;
    int card_h = 142;
    int card_x = mid_cx - card_w / 2;
    int card_y = content_y + 86;

    draw_text_centered(r, "From Reset", mid_cx, card_y - 18, 2, text_dim);
    draw_panel(r, card_x, card_y, card_w, card_h, rgba(8, 20, 42, 235), rgba(0, 195, 255, 220), 12);
    draw_text_centered(r, "ENTIRE", mid_cx, card_y + 8, 1, mb_cyan);

    char c_buf[32];
    /* Row 1: Real-time Fuel Level & Current/Top Speed */
    snprintf(c_buf, sizeof(c_buf), "%.1f L", (state->fuel_level > 0.0f) ? state->fuel_level : (state->fuel_pct * 0.65f));
    draw_text(r, c_buf, card_x + 18, card_y + 28, 2, text_wh);
    snprintf(c_buf, sizeof(c_buf), "%.0f %s", speed, ui->use_mph ? "mph" : "km/h");
    draw_text(r, c_buf, card_x + 154, card_y + 28, 2, text_wh);

    /* Row 2: Live Trip Distance & Lap Timer */
    float trip_dist = (float)state->current_lap * (ui->use_mph ? 2.6f : 4.2f) + ((float)state->current_lap_ms / 60000.0f) * (speed * 0.012f);
    snprintf(c_buf, sizeof(c_buf), "%.1f %s", trip_dist, ui->use_mph ? "mi" : "km");
    draw_text(r, c_buf, card_x + 18, card_y + 64, 2, text_wh);
    int lap_sec = (state->current_lap_ms > 0) ? (int)(state->current_lap_ms / 1000) : (int)((now_ms / 1000) % 3600);
    snprintf(c_buf, sizeof(c_buf), "%02d:%02d", lap_sec / 60, lap_sec % 60);
    draw_text(r, c_buf, card_x + 154, card_y + 64, 2, text_wh);

    /* Row 3: Remaining Dynamic Range & Race Position */
    int rem_range = (int)(state->fuel_pct * (ui->use_mph ? 3.8f : 6.1f));
    snprintf(c_buf, sizeof(c_buf), "%d %s", rem_range, ui->use_mph ? "mi" : "km");
    draw_text(r, c_buf, card_x + 18, card_y + 100, 2, mb_cyan);
    draw_icon_leaf(r, mid_cx, card_y + 108, mb_green);
    snprintf(c_buf, sizeof(c_buf), "POS %d/%d", (state->race_position > 0) ? state->race_position : 1, (state->total_cars > 0) ? state->total_cars : 16);
    draw_text(r, c_buf, card_x + 154, card_y + 100, 2, text_wh);

    /* ================= CENTER BELOW CARD: Seat Occupancy Graphic & AWD Split ================= */
    int occ_w = 236, occ_h = 24, occ_x = mid_cx - occ_w / 2, occ_y = card_y + card_h + 10;
    draw_panel(r, occ_x, occ_y, occ_w, occ_h, rgba(8, 16, 32, 230), rgba(25, 50, 85, 255), 8);
    draw_icon_car_topdown(r, occ_x + 55, occ_y + 12, 18, 20, rgba(40, 75, 120, 255), mb_red, false, false);
    draw_thick_line(r, occ_x + 105, occ_y + 12, occ_x + 130, occ_y + 12, 3, mb_red);
    fill_rect(r, occ_x + 130, occ_y + 7, 8, 10, mb_red);

    int bot_card_y = occ_y + occ_h + 8;
    draw_panel(r, mid_cx - 130, bot_card_y, 260, 48, rgba(6, 14, 26, 230), rgba(25, 50, 85, 255), 8);
    char awd_buf[64];
    snprintf(awd_buf, sizeof(awd_buf), "4MATIC AWD: F %d%% | R %d%%",
             (state->torque_vector_nm[0] > 0.1f || state->torque_vector_nm[2] > 0.1f) ?
             (int)(100.0f * (state->torque_vector_nm[0] + state->torque_vector_nm[1]) / (state->torque_vector_nm[0] + state->torque_vector_nm[1] + state->torque_vector_nm[2] + state->torque_vector_nm[3] + 0.1f)) : 46,
             (state->torque_vector_nm[0] > 0.1f || state->torque_vector_nm[2] > 0.1f) ?
             (int)(100.0f * (state->torque_vector_nm[2] + state->torque_vector_nm[3]) / (state->torque_vector_nm[0] + state->torque_vector_nm[1] + state->torque_vector_nm[2] + state->torque_vector_nm[3] + 0.1f)) : 54);
    draw_text_centered(r, awd_buf, mid_cx, bot_card_y + 6, 2, mb_cyan);

    char sc_fl = (state->surface_type[0] != 0) ? (char)state->surface_type[0] : 'T';
    char sc_fr = (state->surface_type[1] != 0) ? (char)state->surface_type[1] : 'T';
    char sc_rl = (state->surface_type[2] != 0) ? (char)state->surface_type[2] : 'T';
    char sc_rr = (state->surface_type[3] != 0) ? (char)state->surface_type[3] : 'T';
    snprintf(awd_buf, sizeof(awd_buf), "FL %.0f°[%c] FR %.0f°[%c] RL %.0f°[%c] RR %.0f°[%c]",
             state->tire_temp[0], sc_fl, state->tire_temp[1], sc_fr,
             state->tire_temp[2], sc_rl, state->tire_temp[3], sc_rr);
    draw_text_centered(r, awd_buf, mid_cx, bot_card_y + 28, 1, text_wh);

    int status_y = content_y + content_h - 22;
    draw_text(r, "01:58 pm", 355, status_y, 2, text_dim);
    if (is_p) {
        draw_text(r, "PARK", 525, status_y, 2, mb_red);
        fill_rect(r, 625, status_y - 2, 22, 20, mb_blue);
        draw_text_centered(r, "P", 636, status_y, 2, text_wh);
    } else {
        draw_text(r, "READY", 520, status_y, 2, mb_green);
        char lap_buf[16];
        snprintf(lap_buf, sizeof(lap_buf), "LAP %d", (state->current_lap > 0) ? state->current_lap : 1);
        draw_text(r, lap_buf, 620, status_y, 2, text_wh);
    }
    draw_icon_snowflake(r, 706, status_y + 7, text_dim);
    draw_text(r, "24°F", 718, status_y, 2, text_dim);
}

/* 2. BMW CURVED DISPLAY OS 8.5/9 (Symmetrical Dual Chevrons, Full-Width 3D Nav Map) */
static void render_ev_bmw_i(SDL_Renderer *r, const gt7_state_t *state, const hud_ui_state_t *ui, uint32_t now_ms, int content_y, int content_h) {
    (void)now_ms;
    rgba_t bmw_cyan  = rgba(0, 225, 255, 255);
    rgba_t text_dim  = rgba(140, 160, 185, 255);
    rgba_t text_wh   = rgba(245, 250, 255, 255);
    rgba_t bmw_green = rgba(40, 225, 120, 255);

    float speed = ui->use_mph ? state->speed_mph : state->speed_kmh;
    float max_spd = ui->use_mph ? 160.0f : 260.0f;
    float spd_frac = speed / max_spd;
    if (spd_frac > 1.0f) spd_frac = 1.0f;

    float power_pct = state->throttle_pct;
    float charge_pct = state->brake_pct;
    float pwr_frac = (charge_pct > 2.0f) ? (charge_pct / 100.0f) : (power_pct / 100.0f);
    if (pwr_frac > 1.0f) pwr_frac = 1.0f;

    int cy = content_y + content_h / 2;

    /* ================= 100% FULL-WIDTH 3D NAVIGATION STREET GRID BACKGROUND ================= */
    int map_x = 0;
    int map_w = SCREEN_W;
    int map_y = content_y;
    int map_h = content_h - 38;

    /* Dark 3D Perspective Road Grid / Circuit Map */
    fill_rect(r, map_x, map_y, map_w, map_h, rgba(5, 14, 28, 255));

    /* Perspective diagonal road lines radiating across full width */
    for (int gx = -120; gx <= SCREEN_W + 120; gx += 55) {
        int target_x = SCREEN_W / 2 + (gx - SCREEN_W / 2) / 4;
        draw_line(r, gx, map_y + map_h, target_x, map_y + 15, rgba(16, 40, 78, 255));
    }
    /* Perspective horizontal cross roads with quadratic compression */
    for (int gy = 0; gy < 8; ++gy) {
        float f = (float)gy / 7.0f;
        int py = map_y + map_h - (int)(powf(f, 1.4f) * (map_h - 18));
        draw_line(r, 0, py, SCREEN_W, py, rgba(20, 48, 92, 255));
    }

    /* Angled city street network */
    draw_thick_line(r, 60, map_y + 140, 420, map_y + 35, 2, rgba(25, 58, 115, 255));
    draw_thick_line(r, 120, map_y + map_h - 30, 520, map_y + 80, 2, rgba(25, 58, 115, 255));
    draw_thick_line(r, SCREEN_W - 60, map_y + 130, SCREEN_W - 420, map_y + 45, 2, rgba(25, 58, 115, 255));
    draw_thick_line(r, SCREEN_W - 120, map_y + map_h - 40, SCREEN_W - 510, map_y + 90, 2, rgba(25, 58, 115, 255));

    /* White/cyan navigation route line */
    draw_thick_line(r, SCREEN_W / 2 + 10, map_y + map_h, SCREEN_W / 2 - 30, map_y + map_h / 2, 4, rgba(230, 245, 255, 240));
    draw_thick_line(r, SCREEN_W / 2 - 30, map_y + map_h / 2, SCREEN_W / 2 + 120, map_y + 35, 4, rgba(230, 245, 255, 240));

    /* Destination waypoint pin */
    draw_rect(r, SCREEN_W / 2 + 112, map_y + 20, 16, 16, 2, text_wh);
    fill_rect(r, SCREEN_W / 2 + 116, map_y + 24, 8, 8, text_wh);

    /* BMW Driver Attention Camera Trapezoid Notch at top center */
    fill_rect(r, SCREEN_W / 2 - 50, map_y, 100, 16, rgba(0, 0, 0, 255));
    draw_thick_line(r, SCREEN_W / 2 - 60, map_y, SCREEN_W / 2 - 45, map_y + 16, 2, rgba(20, 30, 45, 255));
    draw_thick_line(r, SCREEN_W / 2 + 60, map_y, SCREEN_W / 2 + 45, map_y + 16, 2, rgba(20, 30, 45, 255));
    draw_line(r, SCREEN_W / 2 - 45, map_y + 16, SCREEN_W / 2 + 45, map_y + 16, rgba(20, 30, 45, 255));

    /* Actual GT7 Real-time Track Map Overlay (cleanly contained in central corridor) */
    draw_track_map_geometry(r, state, 470, map_y + 35, SCREEN_W - 940, map_h - 70, 3, 2, false);

    /* Center Navigation Position Triangle Cursor */
    int nav_cx = SCREEN_W / 2;
    int nav_cy = map_y + map_h - 40;
    draw_thick_line(r, nav_cx, nav_cy - 12, nav_cx - 9, nav_cy + 9, 3, bmw_cyan);
    draw_thick_line(r, nav_cx, nav_cy - 12, nav_cx + 9, nav_cy + 9, 3, bmw_cyan);
    draw_thick_line(r, nav_cx - 9, nav_cy + 9, nav_cx, nav_cy + 4, 2, bmw_cyan);
    draw_thick_line(r, nav_cx + 9, nav_cy + 9, nav_cx, nav_cy + 4, 2, bmw_cyan);

    /* ================= LEFT CHEVRON: SPEEDOMETER BRACKET (<) ================= */
    /* Outer 3-Segment Coordinates (top slant, vertical flank, bottom slant) */
    int l_top_x = 390, l_top_y = content_y + 35;
    int l_v_top_x = 220, l_v_top_y = cy - 70;
    int l_v_bot_x = 220, l_v_bot_y = cy + 70;
    int l_bot_x = 420, l_bot_y = content_y + content_h - 60;

    /* Inner 3-Segment Coordinates */
    int l_in_top_x = 355, l_in_top_y = content_y + 55;
    int l_in_v_top_x = 265, l_in_v_top_y = cy - 60;
    int l_in_v_bot_x = 265, l_in_v_bot_y = cy + 60;
    int l_in_bot_x = 390, l_in_bot_y = content_y + content_h - 80;

    /* Frosted glass backing inside left chevron */
    for (int step = 0; step < 8; ++step) {
        float t = (float)step / 7.0f;
        int sx1 = l_top_x - (int)(t * 35), sy1 = l_top_y + (int)(t * 20);
        int sx2 = l_v_top_x + (int)(t * 45), sy2 = l_v_top_y + (int)(t * 10);
        int sx3 = l_v_bot_x + (int)(t * 45), sy3 = l_v_bot_y - (int)(t * 10);
        int sx4 = l_bot_x - (int)(t * 30), sy4 = l_bot_y - (int)(t * 20);
        rgba_t band_col = rgba(0, (uint8_t)(60 + t * 110), (uint8_t)(140 + t * 90), (uint8_t)(30 + t * 160));
        draw_thick_line(r, sx1, sy1, sx2, sy2, 3, band_col);
        draw_thick_line(r, sx2, sy2, sx3, sy3, 3, band_col);
        draw_thick_line(r, sx3, sy3, sx4, sy4, 3, band_col);
    }

    /* Outer crisp border of left chevron */
    draw_thick_line(r, l_top_x, l_top_y, l_v_top_x, l_v_top_y, 4, bmw_cyan);
    draw_thick_line(r, l_v_top_x, l_v_top_y, l_v_bot_x, l_v_bot_y, 4, bmw_cyan);
    draw_thick_line(r, l_v_bot_x, l_v_bot_y, l_bot_x, l_bot_y, 4, bmw_cyan);

    /* Inner border */
    draw_thick_line(r, l_in_top_x, l_in_top_y, l_in_v_top_x, l_in_v_top_y, 2, rgba(0, 140, 255, 140));
    draw_thick_line(r, l_in_v_top_x, l_in_v_top_y, l_in_v_bot_x, l_in_v_bot_y, 2, rgba(0, 140, 255, 140));
    draw_thick_line(r, l_in_v_bot_x, l_in_v_bot_y, l_in_bot_x, l_in_bot_y, 2, rgba(0, 140, 255, 140));

    /* Top chevron cap bar & "km/h" label */
    draw_thick_line(r, l_in_top_x, l_in_top_y, l_top_x, l_top_y, 3, text_wh);
    draw_text_centered(r, ui->use_mph ? "mph" : "km/h", (l_in_top_x + l_top_x) / 2, l_top_y - 26, 2, text_wh);

    /* Graduation marks: 0, 30, 60, 90, 120 */
    draw_thick_line(r, l_bot_x - 12, l_bot_y, l_bot_x + 4, l_bot_y, 3, bmw_cyan);
    draw_text_right(r, "0", l_bot_x - 16, l_bot_y - 6, 2, text_wh);

    draw_thick_line(r, l_v_bot_x - 12, l_v_bot_y, l_v_bot_x + 4, l_v_bot_y, 3, bmw_cyan);
    draw_text_right(r, "30", l_v_bot_x - 16, l_v_bot_y - 6, 2, text_wh);

    draw_thick_line(r, l_v_top_x - 12, cy, l_v_top_x + 4, cy, 3, bmw_cyan);
    draw_text_right(r, "60", l_v_top_x - 16, cy - 6, 2, text_wh);

    draw_thick_line(r, l_v_top_x - 12, l_v_top_y, l_v_top_x + 4, l_v_top_y, 3, bmw_cyan);
    draw_text_right(r, "90", l_v_top_x - 16, l_v_top_y - 6, 2, text_wh);

    draw_thick_line(r, l_top_x - 12, l_top_y, l_top_x + 4, l_top_y, 3, bmw_cyan);
    draw_text_right(r, "120", l_top_x - 16, l_top_y + 16, 2, text_wh);

    /* Dynamic illuminated speed ribbon along left chevron */
    if (spd_frac > 0.02f) {
        if (spd_frac <= 0.35f) {
            float sub = spd_frac / 0.35f;
            int ex = l_bot_x - (int)(sub * (l_bot_x - l_v_bot_x));
            int ey = l_bot_y - (int)(sub * (l_bot_y - l_v_bot_y));
            draw_thick_line(r, l_bot_x, l_bot_y, ex, ey, 7, text_wh);
        } else if (spd_frac <= 0.70f) {
            draw_thick_line(r, l_bot_x, l_bot_y, l_v_bot_x, l_v_bot_y, 7, text_wh);
            float sub = (spd_frac - 0.35f) / 0.35f;
            int ey = l_v_bot_y - (int)(sub * (l_v_bot_y - l_v_top_y));
            draw_thick_line(r, l_v_bot_x, l_v_bot_y, l_v_top_x, ey, 7, text_wh);
        } else {
            draw_thick_line(r, l_bot_x, l_bot_y, l_v_bot_x, l_v_bot_y, 7, text_wh);
            draw_thick_line(r, l_v_bot_x, l_v_bot_y, l_v_top_x, l_v_top_y, 7, text_wh);
            float sub = (spd_frac - 0.70f) / 0.30f;
            int ex = l_v_top_x + (int)(sub * (l_top_x - l_v_top_x));
            int ey = l_v_top_y - (int)(sub * (l_v_top_y - l_top_y));
            draw_thick_line(r, l_v_top_x, l_v_top_y, ex, ey, 7, text_wh);
        }
    }

    /* Huge Modern Vector Digital Speed in crook of left chevron */
    int ispd = (int)roundf(speed);
    draw_modern_number_centered(r, ispd, 1, false, 370, cy - 36, 36, 58, 7, 7, text_wh);
    draw_text_centered(r, ui->use_mph ? "mph" : "km/h", 370, cy + 34, 2, text_wh);

    /* Left Green Headlight Indicator Icon */
    draw_icon_headlight(r, 65, cy + 20, bmw_green);

    /* Translucent BMW M Left Tyres & Steering Widget */
    fill_rect(r, 24, cy - 100, 140, 68, rgba(6, 14, 26, 185));
    draw_rect(r, 24, cy - 100, 140, 68, 1, rgba(25, 50, 85, 200));
    draw_text(r, "TYRES (LEFT)", 32, cy - 94, 1, text_dim);
    char bmw_tbuf[32];
    char sc_fl = (state->surface_type[0] != 0) ? (char)state->surface_type[0] : 'T';
    snprintf(bmw_tbuf, sizeof(bmw_tbuf), "FL %.0f°[%c] RL %.0f°", state->tire_temp[0], sc_fl, state->tire_temp[2]);
    draw_text(r, bmw_tbuf, 32, cy - 76, 1, tire_temp_color(state->tire_temp[0]));
    snprintf(bmw_tbuf, sizeof(bmw_tbuf), "STEER: %+.1f°", state->steer_angle_fl_deg);
    draw_text(r, bmw_tbuf, 32, cy - 56, 1, bmw_cyan);

    /* ================= RIGHT CHEVRON: POWER & ADAPTIVE BRACKET (>) ================= */
    /* Outer Symmetrical Chevron Coordinates */
    int r_top_x = SCREEN_W - l_top_x;
    int r_v_top_x = SCREEN_W - l_v_top_x;
    int r_v_bot_x = SCREEN_W - l_v_bot_x;
    int r_bot_x = SCREEN_W - l_bot_x;

    /* Inner Symmetrical Chevron Coordinates */
    int r_in_top_x = SCREEN_W - l_in_top_x;
    int r_in_v_top_x = SCREEN_W - l_in_v_top_x;
    int r_in_v_bot_x = SCREEN_W - l_in_v_bot_x;
    int r_in_bot_x = SCREEN_W - l_in_bot_x;

    /* Frosted glass backing inside right chevron */
    for (int step = 0; step < 8; ++step) {
        float t = (float)step / 7.0f;
        int sx1 = r_top_x + (int)(t * 35), sy1 = l_top_y + (int)(t * 20);
        int sx2 = r_v_top_x - (int)(t * 45), sy2 = l_v_top_y + (int)(t * 10);
        int sx3 = r_v_bot_x - (int)(t * 45), sy3 = l_v_bot_y - (int)(t * 10);
        int sx4 = r_bot_x + (int)(t * 30), sy4 = l_bot_y - (int)(t * 20);
        rgba_t band_col = rgba(0, (uint8_t)(60 + t * 110), (uint8_t)(140 + t * 90), (uint8_t)(30 + t * 160));
        draw_thick_line(r, sx1, sy1, sx2, sy2, 3, band_col);
        draw_thick_line(r, sx2, sy2, sx3, sy3, 3, band_col);
        draw_thick_line(r, sx3, sy3, sx4, sy4, 3, band_col);
    }

    /* Outer crisp border of right chevron */
    draw_thick_line(r, r_top_x, l_top_y, r_v_top_x, l_v_top_y, 4, bmw_cyan);
    draw_thick_line(r, r_v_top_x, l_v_top_y, r_v_bot_x, l_v_bot_y, 4, bmw_cyan);
    draw_thick_line(r, r_v_bot_x, l_v_bot_y, r_bot_x, l_bot_y, 4, bmw_cyan);

    /* Inner border */
    draw_thick_line(r, r_in_top_x, l_in_top_y, r_in_v_top_x, l_in_v_top_y, 2, rgba(0, 140, 255, 140));
    draw_thick_line(r, r_in_v_top_x, l_in_v_top_y, r_in_v_bot_x, l_in_v_bot_y, 2, rgba(0, 140, 255, 140));
    draw_thick_line(r, r_in_v_bot_x, l_in_v_bot_y, r_in_bot_x, l_in_bot_y, 2, rgba(0, 140, 255, 140));

    /* Top chevron cap bar & "Power %" label */
    draw_thick_line(r, r_top_x, l_top_y, r_in_top_x, l_in_top_y, 3, text_wh);
    draw_text_centered(r, "Power %", (r_top_x + r_in_top_x) / 2, l_top_y - 26, 2, text_wh);

    /* Graduation marks: READY 0, 25, 50, 100 */
    draw_thick_line(r, r_bot_x - 4, l_bot_y, r_bot_x + 12, l_bot_y, 3, bmw_cyan);
    draw_text(r, "READY 0", r_bot_x + 16, l_bot_y - 6, 2, text_wh);

    draw_thick_line(r, r_v_bot_x - 4, l_v_bot_y, r_v_bot_x + 12, l_v_bot_y, 3, bmw_cyan);
    draw_text(r, "25", r_v_bot_x + 16, l_v_bot_y - 6, 2, text_wh);

    draw_thick_line(r, r_v_top_x - 4, l_v_top_y, r_v_top_x + 12, l_v_top_y, 3, bmw_cyan);
    draw_text(r, "50", r_v_top_x + 16, l_v_top_y - 6, 2, text_wh);

    draw_thick_line(r, r_top_x - 4, l_top_y, r_top_x + 12, l_top_y, 3, bmw_cyan);
    draw_text(r, "100", r_top_x + 16, l_top_y + 16, 2, text_wh);

    /* Dynamic illuminated power ribbon along right chevron */
    if (pwr_frac > 0.02f) {
        if (pwr_frac <= 0.35f) {
            float sub = pwr_frac / 0.35f;
            int ex = r_bot_x + (int)(sub * (r_v_bot_x - r_bot_x));
            int ey = l_bot_y - (int)(sub * (l_bot_y - l_v_bot_y));
            draw_thick_line(r, r_bot_x, l_bot_y, ex, ey, 7, (charge_pct > 2.0f) ? bmw_green : text_wh);
        } else if (pwr_frac <= 0.70f) {
            draw_thick_line(r, r_bot_x, l_bot_y, r_v_bot_x, l_v_bot_y, 7, text_wh);
            float sub = (pwr_frac - 0.35f) / 0.35f;
            int ey = l_v_bot_y - (int)(sub * (l_v_bot_y - l_v_top_y));
            draw_thick_line(r, r_v_bot_x, l_v_bot_y, r_v_top_x, ey, 7, text_wh);
        } else {
            draw_thick_line(r, r_bot_x, l_bot_y, r_v_bot_x, l_v_bot_y, 7, text_wh);
            draw_thick_line(r, r_v_bot_x, l_v_bot_y, r_v_top_x, l_v_top_y, 7, text_wh);
            float sub = (pwr_frac - 0.70f) / 0.30f;
            int ex = r_v_top_x - (int)(sub * (r_v_top_x - r_top_x));
            int ey = l_v_top_y - (int)(sub * (l_v_top_y - l_top_y));
            draw_thick_line(r, r_v_top_x, l_v_top_y, ex, ey, 7, text_wh);
        }
    }

    /* Dynamic Modern Vector Gear Indicator in crook of right chevron */
    bool is_p = (state->speed_kmh < 1.0f && state->current_gear <= 0);
    bool is_r = (state->current_gear == 0);
    bool is_n = (state->current_gear == -1);
    bool is_sport = (state->throttle_pct > 75.0f);

    if (is_p) {
        draw_modern_char(r, 'P', SCREEN_W - 390, cy - 35, 40, 58, 8, text_wh);
    } else if (is_r) {
        draw_modern_char(r, 'R', SCREEN_W - 390, cy - 35, 40, 58, 8, rgba(255, 195, 45, 255));
    } else if (is_n) {
        draw_modern_char(r, 'N', SCREEN_W - 390, cy - 35, 40, 58, 8, text_wh);
    } else {
        draw_modern_char(r, is_sport ? 'S' : 'D', SCREEN_W - 405, cy - 35, 36, 58, 7, is_sport ? rgba(255, 60, 60, 255) : text_wh);
        draw_modern_char(r, '0' + state->current_gear, SCREEN_W - 362, cy - 35, 26, 58, 6, is_sport ? rgba(255, 60, 60, 255) : bmw_cyan);
    }

    /* Live Energy Recovery / Drive Output in kW */
    char bmw_kw[24];
    if (state->energy_recovery_kw < -1.0f) {
        snprintf(bmw_kw, sizeof(bmw_kw), "-%d kW", (int)fabsf(state->energy_recovery_kw));
        draw_text_centered(r, bmw_kw, SCREEN_W - 370, cy + 34, 2, bmw_green);
    } else if (state->energy_recovery_kw > 1.0f) {
        snprintf(bmw_kw, sizeof(bmw_kw), "+%d kW", (int)state->energy_recovery_kw);
        draw_text_centered(r, bmw_kw, SCREEN_W - 370, cy + 34, 2, bmw_cyan);
    }

    /* Translucent BMW M Right Tyres & M xDrive AWD Widget */
    fill_rect(r, SCREEN_W - 164, cy - 100, 140, 68, rgba(6, 14, 26, 185));
    draw_rect(r, SCREEN_W - 164, cy - 100, 140, 68, 1, rgba(25, 50, 85, 200));
    draw_text(r, "TYRES (RIGHT)", SCREEN_W - 156, cy - 94, 1, text_dim);
    char bmw_rtbuf[32];
    char sc_fr = (state->surface_type[1] != 0) ? (char)state->surface_type[1] : 'T';
    snprintf(bmw_rtbuf, sizeof(bmw_rtbuf), "FR %.0f°[%c] RR %.0f°", state->tire_temp[1], sc_fr, state->tire_temp[3]);
    draw_text(r, bmw_rtbuf, SCREEN_W - 156, cy - 76, 1, tire_temp_color(state->tire_temp[1]));
    snprintf(bmw_rtbuf, sizeof(bmw_rtbuf), "M xDRIVE F46:R54");
    draw_text(r, bmw_rtbuf, SCREEN_W - 156, cy - 56, 1, bmw_cyan);

    /* Dynamic Lower inner tab: ADAPTIVE / SPORT / REGEN / PARK mode pill */
    const char *mode_name = "ADAPTIVE";
    rgba_t mode_color = bmw_cyan;
    rgba_t mode_bg = rgba(0, 48, 90, 240);

    if (charge_pct > 5.0f) {
        mode_name = "REGEN";
        mode_color = bmw_green;
        mode_bg = rgba(0, 60, 40, 240);
    } else if (is_sport) {
        mode_name = "SPORT";
        mode_color = rgba(255, 60, 60, 255);
        mode_bg = rgba(90, 15, 25, 240);
    } else if (is_p) {
        mode_name = "PARK";
        mode_color = text_dim;
        mode_bg = rgba(25, 35, 50, 240);
    }

    fill_rect(r, SCREEN_W - 390, l_bot_y - 48, 86, 24, mode_bg);
    draw_rect(r, SCREEN_W - 390, l_bot_y - 48, 86, 24, 1, mode_color);
    draw_text_centered(r, mode_name, SCREEN_W - 347, l_bot_y - 43, 1, mode_color);

    /* ================= BOTTOM STATUS ROW ================= */
    int b_row_y = content_y + content_h - 32;

    /* Left: Dynamic Fuel / Energy range and bar */
    int fuel_range = (int)(state->fuel_pct * (ui->use_mph ? 4.3f : 7.0f));
    char bmw_f_buf[24];
    snprintf(bmw_f_buf, sizeof(bmw_f_buf), "%d %s", fuel_range, ui->use_mph ? "mi" : "km");
    draw_icon_fuel_pump(r, 115, b_row_y - 12, text_wh);
    draw_text(r, bmw_f_buf, 132, b_row_y - 12, 2, text_wh);
    fill_rect(r, 130, b_row_y + 8, 120, 6, rgba(30, 45, 65, 255));
    int bmw_f_bar = (int)((state->fuel_pct / 100.0f) * 120.0f);
    if (bmw_f_bar > 120) bmw_f_bar = 120;
    if (bmw_f_bar < 0) bmw_f_bar = 0;
    fill_rect(r, 130, b_row_y + 8, bmw_f_bar, 6, (state->fuel_pct < 15.0f) ? rgba(255, 180, 0, 255) : text_wh);
    draw_text(r, "0", 118, b_row_y + 5, 1, text_dim);
    draw_text(r, "1", 255, b_row_y + 5, 1, text_dim);

    /* Clock */
    draw_text_centered(r, "10:49", 320, b_row_y - 2, 2, text_wh);

    /* Speed Limit Sign: Red Circle [ 50 ] */
    draw_speed_limit_sign(r, 495, b_row_y + 4, 50, false);

    /* Overhead car chassis icon + Seatbelt */
    draw_icon_seatbelt(r, 605, b_row_y + 4, rgba(235, 45, 55, 255));
    draw_icon_car_topdown(r, 645, b_row_y + 4, 26, 32, rgba(40, 75, 120, 255), rgba(255, 160, 0, 255), true, true);

    /* Outside Temperature */
    char temp_buf[24];
    snprintf(temp_buf, sizeof(temp_buf), "%s", ui->use_mph ? "+ 66 °F" : "+ 19.0 °C");
    draw_text(r, temp_buf, 930, b_row_y - 2, 2, text_wh);

    /* Right: Dynamic High-Voltage Electric battery range and bar */
    int batt_range = (int)((state->fuel_pct > 0.0f ? state->fuel_pct : 80.0f) * (ui->use_mph ? 0.35f : 0.55f));
    char bmw_b_buf[24];
    snprintf(bmw_b_buf, sizeof(bmw_b_buf), "%d %s", batt_range, ui->use_mph ? "mi" : "km");
    draw_icon_battery(r, 1058, b_row_y - 12, 16, 11, state->fuel_pct / 100.0f, bmw_cyan);
    draw_text(r, bmw_b_buf, 1080, b_row_y - 12, 2, text_wh);
    fill_rect(r, 1080, b_row_y + 8, 120, 6, rgba(30, 45, 65, 255));
    int bmw_b_bar = (int)((state->fuel_pct / 100.0f) * 120.0f);
    if (bmw_b_bar > 120) bmw_b_bar = 120;
    if (bmw_b_bar < 0) bmw_b_bar = 0;
    fill_rect(r, 1080, b_row_y + 8, bmw_b_bar, 6, bmw_cyan);
    draw_text(r, "0", 1068, b_row_y + 5, 1, text_dim);
    draw_text(r, "1", 1205, b_row_y + 5, 1, text_dim);
}

/* 3. AUDI SPORT RS VIRTUAL COCKPIT (RS Runway Tachometer, Dual Power/Torque Pods, Boost Bar) */
static void render_ev_audi_etron(SDL_Renderer *r, const gt7_state_t *state, const hud_ui_state_t *ui, uint32_t now_ms, int content_y, int content_h) {
    (void)now_ms;
    rgba_t audi_red   = rgba(235, 30, 45, 255);
    rgba_t audi_cyan  = rgba(0, 220, 245, 255);
    rgba_t audi_green = rgba(40, 225, 120, 255);
    rgba_t text_dim   = rgba(140, 160, 185, 255);
    rgba_t text_wh    = rgba(245, 250, 255, 255);
    rgba_t runway_bg  = rgba(30, 42, 60, 255);

    float speed = ui->use_mph ? state->speed_mph : state->speed_kmh;
    float rpm = state->engine_rpm;
    float max_rpm = 8000.0f;
    float rpm_frac = rpm / max_rpm;
    if (rpm_frac > 1.0f) rpm_frac = 1.0f;

    float power_pct = state->throttle_pct;
    float torque_pct = (state->throttle_pct > 5.0f) ? (state->throttle_pct * 0.95f) : 2.0f;
    float boost_pct = (state->turbo_bar > 0.0f) ? (state->turbo_bar / 2.0f) : 0.0f;
    if (boost_pct > 1.0f) boost_pct = 1.0f;

    int mid_y = content_y + content_h / 2;

    /* ================= TOP STATUS BAR ================= */
    int top_y = content_y + 12;

    /* Headlight & Parking Brake */
    draw_icon_headlight(r, 410, top_y + 10, audi_green);
    draw_text(r, "(P)", 510, top_y, 2, audi_red);

    /* Dynamic Fuel Range Red Badge */
    int audi_fuel_range = (int)(state->fuel_pct * (ui->use_mph ? 3.9f : 6.3f));
    char audi_f_buf[24];
    snprintf(audi_f_buf, sizeof(audi_f_buf), "%d %s", audi_fuel_range, ui->use_mph ? "mi" : "km");
    fill_rect(r, 400, top_y + 24, 110, 24, (state->fuel_pct < 15.0f) ? rgba(200, 30, 40, 255) : audi_red);
    draw_icon_fuel_pump(r, 410, top_y + 29, text_wh);
    draw_text_centered(r, audi_f_buf, 465, top_y + 28, 2, text_wh);

    /* Radio Station */
    draw_text(r, "((o)) 2GB 873", 550, top_y + 28, 2, text_wh);

    /* Phone & Compass Icons */
    draw_text(r, "[T]  ^", 725, top_y + 28, 2, text_dim);
    draw_line(r, 390, top_y + 54, 930, top_y + 54, rgba(40, 55, 80, 255));

    /* ================= LEFT SIDE: Date & Time, Oil Temp ================= */
    int left_x = 60;

    draw_text(r, "Date & time", left_x + 90, mid_y - 120, 2, text_dim);
    draw_text(r, "10:43 AM", left_x + 90, mid_y - 85, 4, text_wh);
    draw_text(r, "Wednesday", left_x + 90, mid_y - 48, 2, text_wh);
    draw_text(r, "11.3.2026", left_x + 90, mid_y - 26, 2, text_wh);

    /* Dynamic Oil Temperature Bar */
    int oil_y = mid_y + 90;
    float o_temp = (state->oil_temp > 20.0f) ? state->oil_temp : 102.0f;
    float o_frac = (o_temp - 60.0f) / (140.0f - 60.0f);
    if (o_frac < 0.0f) o_frac = 0.0f;
    if (o_frac > 1.0f) o_frac = 1.0f;
    int oil_fill_x = (left_x + 60) + (int)(o_frac * 170.0f);
    draw_thick_line(r, left_x + 60, oil_y, left_x + 230, oil_y, 3, rgba(30, 45, 65, 255));
    draw_thick_line(r, left_x + 60, oil_y, oil_fill_x, oil_y, 3, (o_temp > 125.0f) ? audi_red : text_wh);
    char o_buf[24];
    snprintf(o_buf, sizeof(o_buf), "OIL %.0f°C", o_temp);
    draw_text(r, o_buf, left_x + 100, oil_y + 10, 2, text_wh);

    /* Audi Sport RS TPMS Tyre Card (4-Wheel Temperatures & Contact Patches) */
    draw_panel(r, left_x + 50, oil_y + 34, 220, 78, rgba(8, 14, 24, 230), rgba(35, 55, 80, 255), 8);
    draw_text(r, "SPORT RS TYRES", left_x + 62, oil_y + 42, 1, audi_red);
    char rs_tbuf[32];
    char sc_fl = (state->surface_type[0] != 0) ? (char)state->surface_type[0] : 'T';
    char sc_fr = (state->surface_type[1] != 0) ? (char)state->surface_type[1] : 'T';
    snprintf(rs_tbuf, sizeof(rs_tbuf), "FL %.0f°C [%c]  FR %.0f°C [%c]", state->tire_temp[0], sc_fl, state->tire_temp[1], sc_fr);
    draw_text(r, rs_tbuf, left_x + 62, oil_y + 60, 1, tire_temp_color(state->tire_temp[0]));
    char sc_rl = (state->surface_type[2] != 0) ? (char)state->surface_type[2] : 'T';
    char sc_rr = (state->surface_type[3] != 0) ? (char)state->surface_type[3] : 'T';
    snprintf(rs_tbuf, sizeof(rs_tbuf), "RL %.0f°C [%c]  RR %.0f°C [%c]", state->tire_temp[2], sc_rl, state->tire_temp[3], sc_rr);
    draw_text(r, rs_tbuf, left_x + 62, oil_y + 80, 1, tire_temp_color(state->tire_temp[2]));

    /* ================= CENTER: AUDI RS RUNWAY TACHOMETER ================= */
    /* Runway Hockey-stick geometry */
    int rx_start = 380, ry_start = mid_y + 115; /* READY */
    int rx_bend  = 530, ry_bend  = mid_y - 95;  /* Turn at 3.5k RPM */
    int rx_end   = 890, ry_end   = mid_y - 95;  /* 8k RPM Redline */

    /* Inactive Track */
    draw_thick_line(r, rx_start, ry_start, rx_bend, ry_bend, 14, runway_bg);
    draw_thick_line(r, rx_bend, ry_bend, rx_end, ry_end, 14, runway_bg);

    /* Redline Sector (7 to 8) on horizontal track */
    int rx_redline = rx_bend + (int)((rx_end - rx_bend) * 0.72f);
    draw_thick_line(r, rx_redline, ry_end, rx_end, ry_end, 14, rgba(140, 20, 30, 255));

    /* Graduation Labels along Runway: 1, 2, 3 along diagonal; 4, 5, 6, 7, 8 along top */
    draw_text(r, "READY", rx_start - 6, ry_start + 16, 1, text_dim);

    /* Diagonal ticks 1, 2, 3 */
    for (int i = 1; i <= 3; ++i) {
        float f = (float)i / 3.5f;
        int tx = rx_start + (int)(f * (rx_bend - rx_start));
        int ty = ry_start - (int)(f * (ry_start - ry_bend));
        draw_thick_line(r, tx - 8, ty - 6, tx + 6, ty + 8, 2, text_wh);
        char nbuf[4];
        snprintf(nbuf, sizeof(nbuf), "%d", i);
        draw_text(r, nbuf, tx - 28, ty - 20, 2, text_wh);
    }

    /* Horizontal ticks 4, 5, 6, 7, 8 */
    for (int i = 4; i <= 8; ++i) {
        float f = (float)(i - 3.5f) / 4.5f;
        int tx = rx_bend + (int)(f * (rx_end - rx_bend));
        draw_thick_line(r, tx, ry_bend - 12, tx, ry_bend + 4, 2, (i >= 7) ? audi_red : text_wh);
        char nbuf[4];
        snprintf(nbuf, sizeof(nbuf), "%d", i);
        draw_text_centered(r, nbuf, tx, ry_bend - 28, 2, (i >= 7) ? audi_red : text_wh);
    }
    draw_text_right(r, "1 / min x 1000", rx_end, ry_bend + 22, 1, text_dim);

    /* Dynamic Illuminated Runway RPM Fill */
    float diag_len = sqrtf((rx_bend - rx_start) * (rx_bend - rx_start) + (ry_start - ry_bend) * (ry_start - ry_bend));
    float horiz_len = (float)(rx_end - rx_bend);
    float total_runway = diag_len + horiz_len;
    float lit_len = rpm_frac * total_runway;

    if (lit_len > 0.0f) {
        if (lit_len <= diag_len) {
            float sub = lit_len / diag_len;
            int ex = rx_start + (int)(sub * (rx_bend - rx_start));
            int ey = ry_start - (int)(sub * (ry_start - ry_bend));
            draw_thick_line(r, rx_start, ry_start, ex, ey, 12, text_wh);
        } else {
            draw_thick_line(r, rx_start, ry_start, rx_bend, ry_bend, 12, text_wh);
            float sub = (lit_len - diag_len) / horiz_len;
            int ex = rx_bend + (int)(sub * (rx_end - rx_bend));
            draw_thick_line(r, rx_bend, ry_bend, ex, ry_end, 12, (rpm_frac > 0.85f) ? audi_red : text_wh);
        }
    }

    /* Under Runway: Large Modern Digital Speed */
    int ispd = (int)roundf(speed);
    int center_disp_x = 640;
    draw_modern_number_centered(r, ispd, 1, false, center_disp_x, mid_y - 28, 38, 62, 8, 7, text_wh);
    draw_text_centered(r, ui->use_mph ? "mph" : "km/h", center_disp_x, mid_y + 44, 1, text_dim);

    /* Dynamic Gear indicator under speed: P / R / N / D <gear> / S <gear> and DYNAMIC / AUTO / PARK */
    bool is_p = (state->speed_kmh < 1.0f && state->current_gear <= 0);
    bool is_r = (state->current_gear == 0);
    bool is_n = (state->current_gear == -1);
    bool is_sport = (state->throttle_pct > 75.0f);

    if (is_p) {
        draw_modern_char(r, 'P', center_disp_x - 12, mid_y + 56, 24, 36, 5, text_wh);
    } else if (is_r) {
        draw_modern_char(r, 'R', center_disp_x - 12, mid_y + 56, 24, 36, 5, rgba(255, 195, 45, 255));
    } else if (is_n) {
        draw_modern_char(r, 'N', center_disp_x - 12, mid_y + 56, 24, 36, 5, text_wh);
    } else {
        draw_modern_char(r, is_sport ? 'S' : 'D', center_disp_x - 22, mid_y + 56, 20, 36, 5, is_sport ? audi_red : text_wh);
        draw_modern_char(r, '0' + state->current_gear, center_disp_x + 4, mid_y + 56, 18, 36, 5, is_sport ? audi_red : text_wh);
    }
    draw_text_centered(r, is_sport ? "DYNAMIC" : (is_p ? "PARK" : "AUTO"), center_disp_x, mid_y + 98, 1, is_sport ? audi_red : text_dim);

    /* Lane Assist Icon */
    draw_icon_lane_assist(r, center_disp_x + 130, mid_y + 72, audi_green);

    /* ================= RIGHT SIDE: Dual Circular Pods (Power & Torque) ================= */
    int pod1_cx = 970;
    int pod2_cx = 1110;
    int pod_cy = mid_y + 15;
    int pod_rad = 54;

    /* Pod 1: Power % */
    draw_text_centered(r, "Power", pod1_cx, pod_cy - pod_rad - 22, 2, text_wh);
    fill_circle(r, pod1_cx, pod_cy, pod_rad, rgba(6, 14, 24, 255));
    draw_circle(r, pod1_cx, pod_cy, pod_rad, 2, audi_cyan);
    draw_circle(r, pod1_cx, pod_cy, pod_rad - 4, 1, rgba(20, 45, 75, 255));
    draw_text_centered(r, "%", pod1_cx, pod_cy - 24, 2, text_dim);
    draw_modern_number_centered(r, (int)power_pct, 1, false, pod1_cx, pod_cy - 4, 22, 36, 5, 5, text_wh);
    draw_segmented_arc(r, pod1_cx, pod_cy, pod_rad - 6, 4, 120.0f, 420.0f, 16, power_pct / 100.0f, audi_red, rgba(0, 0, 0, 0));
    fill_rect(r, pod1_cx - 2, pod_cy + pod_rad - 8, 4, 6, audi_red);

    /* Pod 2: Torque % */
    draw_text_centered(r, "Torque", pod2_cx, pod_cy - pod_rad - 22, 2, text_wh);
    fill_circle(r, pod2_cx, pod_cy, pod_rad, rgba(6, 14, 24, 255));
    draw_circle(r, pod2_cx, pod_cy, pod_rad, 2, audi_cyan);
    draw_circle(r, pod2_cx, pod_cy, pod_rad - 4, 1, rgba(20, 45, 75, 255));
    draw_text_centered(r, "%", pod2_cx, pod_cy - 24, 2, text_dim);
    draw_modern_number_centered(r, (int)torque_pct, 1, false, pod2_cx, pod_cy - 4, 22, 36, 5, 5, text_wh);
    draw_segmented_arc(r, pod2_cx, pod_cy, pod_rad - 6, 4, 120.0f, 420.0f, 16, torque_pct / 100.0f, audi_red, rgba(0, 0, 0, 0));
    fill_rect(r, pod2_cx - 2, pod_cy + pod_rad - 8, 4, 6, audi_red);

    /* Horizontal Boost Bar underneath Pods */
    int bst_y = pod_cy + pod_rad + 34;
    int bst_x = pod1_cx - 40;
    int bst_w = (pod2_cx + 40) - bst_x;
    draw_text(r, "MIN", bst_x, bst_y - 2, 1, text_dim);
    draw_text_centered(r, "BOOST", bst_x + bst_w / 2, bst_y - 2, 2, text_wh);
    draw_text_right(r, "MAX", bst_x + bst_w, bst_y - 2, 1, text_dim);
    fill_rect(r, bst_x, bst_y + 16, bst_w, 4, rgba(30, 45, 65, 255));
    fill_rect(r, bst_x, bst_y + 16, (int)(bst_w * boost_pct), 4, audi_red);

    /* RS Performance & Quattro e-tron AWD Panel */
    draw_panel(r, bst_x, bst_y + 28, bst_w, 48, rgba(8, 14, 24, 230), rgba(35, 55, 80, 255), 8);
    draw_text(r, "RS QUATTRO e-tron", bst_x + 12, bst_y + 34, 1, audi_red);
    char rs_perf[48];
    if (state->has_packet_tilde || fabsf(state->energy_recovery_kw) > 1.0f) {
        snprintf(rs_perf, sizeof(rs_perf), "%+.0f kW  |  AWD 46:54", state->energy_recovery_kw);
    } else {
        snprintf(rs_perf, sizeof(rs_perf), "BOOST +%.2f BAR", state->turbo_bar);
    }
    draw_text(r, rs_perf, bst_x + 12, bst_y + 54, 1, text_wh);

    /* Center Lap & Pos Info under Dynamic mode */
    char a_lap[48];
    snprintf(a_lap, sizeof(a_lap), "LAP %d/%d   P%d", state->current_lap > 0 ? state->current_lap : 5, state->total_laps > 0 ? state->total_laps : 15, state->race_position > 0 ? state->race_position : 2);
    draw_text_centered(r, a_lap, center_disp_x, mid_y + 115, 1, text_dim);

    /* ================= BOTTOM STATUS BAR ================= */
    int bot_y = content_y + content_h - 32;
    draw_thick_line(r, 180, bot_y - 14, 1100, bot_y - 14, 1, rgba(35, 50, 75, 255));

    /* Dynamic Left Odometer & Time */
    float trip_km = (float)state->current_lap * 4.5f + ((float)state->current_lap_ms / 60000.0f) * (state->speed_kmh * 0.015f);
    char odo_buf[24];
    snprintf(odo_buf, sizeof(odo_buf), "%d %s", (int)(8340 + trip_km), ui->use_mph ? "mi" : "km");
    draw_icon_seatbelt(r, 480, bot_y + 4, audi_red);
    draw_text(r, odo_buf, 500, bot_y - 2, 2, text_wh);
    draw_text(r, "10:43 AM", 500, bot_y + 18, 2, text_dim);

    /* Dynamic Trip Distance */
    char trip_buf[24];
    snprintf(trip_buf, sizeof(trip_buf), "%.1f %s", ui->use_mph ? (trip_km * 0.621371f) : trip_km, ui->use_mph ? "mi" : "km");
    draw_text(r, trip_buf, 680, bot_y - 2, 2, text_wh);

    /* Temperature */
    char a_temp_buf[24];
    snprintf(a_temp_buf, sizeof(a_temp_buf), "%s", ui->use_mph ? "+ 74 °F" : "+ 23.5 °C");
    draw_text(r, a_temp_buf, 680, bot_y + 18, 2, text_wh);
}

/* 4. VOLVO EX90 / GOOGLE BUILT-IN PANORAMIC CLUSTER (Full-Width Map, Floating Glass Pods, Sweeping Arcs) */
static void render_ev_volvo_ex(SDL_Renderer *r, const gt7_state_t *state, const hud_ui_state_t *ui, uint32_t now_ms, int content_y, int content_h) {
    (void)now_ms;
    rgba_t volvo_cyan = rgba(0, 210, 255, 255);
    rgba_t volvo_green = rgba(50, 220, 130, 255);
    rgba_t text_dim   = rgba(140, 160, 185, 255);
    rgba_t text_wh    = rgba(245, 250, 255, 255);

    float speed = ui->use_mph ? state->speed_mph : state->speed_kmh;
    float max_spd = ui->use_mph ? 140.0f : 200.0f;
    float spd_frac = speed / max_spd;
    if (spd_frac > 1.0f) spd_frac = 1.0f;

    float power_pct = state->throttle_pct;
    float charge_pct = state->brake_pct;

    int cy = content_y + content_h / 2;

    /* ================= 100% FULL-SCREEN PANORAMIC 3D GOOGLE MAPS ================= */
    int map_x = 0;
    int map_w = SCREEN_W;
    int map_y = content_y;
    int map_h = content_h;

    fill_rect(r, map_x, map_y, map_w, map_h, rgba(5, 12, 28, 255));

    /* Perspective Road Grid spanning full 1280px screen */
    for (int gx = -100; gx <= SCREEN_W + 100; gx += 55) {
        int target_x = SCREEN_W / 2 + (gx - SCREEN_W / 2) / 3;
        draw_line(r, gx, map_y + map_h, target_x, map_y + 10, rgba(14, 32, 65, 255));
    }
    for (int gy = 0; gy < 10; ++gy) {
        float f = (float)gy / 9.0f;
        int py = map_y + map_h - (int)(powf(f, 1.35f) * (map_h - 15));
        draw_line(r, 0, py, SCREEN_W, py, rgba(18, 42, 80, 255));
    }

    /* City Street Names (Google Maps authentic look) */
    draw_text(r, "Granville St", 290, cy + 90, 1, rgba(60, 95, 145, 255));
    draw_text(r, "Oak St", 460, cy + 30, 1, rgba(60, 95, 145, 255));
    draw_text(r, "W 49th Ave", 730, cy + 50, 1, rgba(60, 95, 145, 255));
    draw_text(r, "W 57th Ave", 840, cy + 110, 1, rgba(60, 95, 145, 255));

    /* Cyan Highlighted Lake / Park Polygon (Devonshire Park) */
    fill_rect(r, 610, cy - 30, 105, 80, rgba(0, 140, 190, 210));
    draw_rect(r, 610, cy - 30, 105, 80, 1, volvo_cyan);
    draw_text_centered(r, "Devonshire", 662, cy - 4, 1, text_wh);
    draw_text_centered(r, "park", 662, cy + 12, 1, text_wh);

    /* Real GT7 Circuit Geometry Overlaid in Center between Pods */
    draw_track_map_geometry(r, state, 400, map_y + 30, SCREEN_W - 800, map_h - 60, 3, 2, false);

    /* Blue Navigation Arrow Cursor */
    int nav_x = 615;
    int nav_y = map_y + map_h - 90;
    fill_circle(r, nav_x, nav_y, 18, rgba(0, 120, 255, 220));
    draw_thick_line(r, nav_x, nav_y - 10, nav_x - 7, nav_y + 7, 3, text_wh);
    draw_thick_line(r, nav_x, nav_y - 10, nav_x + 7, nav_y + 7, 3, text_wh);
    draw_thick_line(r, nav_x - 7, nav_y + 7, nav_x, nav_y + 3, 2, text_wh);
    draw_thick_line(r, nav_x + 7, nav_y + 7, nav_x, nav_y + 3, 2, text_wh);

    draw_text(r, "Google", 760, map_y + map_h - 85, 2, rgba(180, 200, 225, 220));

    /* ================= LEFT FLOATING POD & SPEEDOMETER ARC ================= */
    int l_pod_w = 210;
    int l_pod_h = 240;
    int l_pod_x = 160;
    int l_pod_y = cy - l_pod_h / 2;

    /* Left Outer Sweeping Speedometer Arc (hugging left margin) */
    int l_arc_cx = -35;
    int l_arc_rad = 195;
    draw_segmented_arc(r, l_arc_cx, cy, l_arc_rad, 5, -55.0f, 55.0f, 24, 1.0f, rgba(35, 50, 75, 255), rgba(0, 0, 0, 0));
    if (spd_frac > 0.02f) {
        float lit_ang = 55.0f - spd_frac * 110.0f;
        draw_segmented_arc(r, l_arc_cx, cy, l_arc_rad, 7, lit_ang, 55.0f, 24, 1.0f, text_wh, rgba(0, 0, 0, 0));
    }

    /* Speed Ticks and Numbers: 200 (top) down to 0 (bottom) */
    static const int volvo_spds[8] = {200, 180, 140, 100, 60, 40, 20, 0};
    for (int i = 0; i < 8; ++i) {
        float deg = -50.0f + (float)i * (100.0f / 7.0f);
        float rad = deg * 3.14159265f / 180.0f;
        int x_in  = l_arc_cx + (int)((l_arc_rad - 12) * cosf(rad));
        int y_in  = cy + (int)((l_arc_rad - 12) * sinf(rad));
        int x_out = l_arc_cx + (int)((l_arc_rad + 2) * cosf(rad));
        int y_out = cy + (int)((l_arc_rad + 2) * sinf(rad));
        draw_thick_line(r, x_in, y_in, x_out, y_out, (i % 2 == 0) ? 3 : 1, text_wh);

        int tx = l_arc_cx + (int)((l_arc_rad - 24) * cosf(rad));
        int ty = cy + (int)((l_arc_rad - 24) * sinf(rad));
        char sbuf[8];
        snprintf(sbuf, sizeof(sbuf), "%d", volvo_spds[i]);
        draw_text_right(r, sbuf, tx, ty - 6, 1, text_wh);
    }

    /* Left Glass Pod */
    draw_panel(r, l_pod_x, l_pod_y, l_pod_w, l_pod_h, rgba(10, 18, 30, 235), rgba(35, 55, 85, 255), 16);

    /* Speed Limit Sign: White rounded square [ 20 ] */
    fill_rect(r, l_pod_x + 16, l_pod_y + 88, 36, 44, rgba(245, 250, 255, 255));
    draw_rect(r, l_pod_x + 16, l_pod_y + 88, 36, 44, 2, rgba(20, 20, 20, 255));
    draw_text_centered(r, "20", l_pod_x + 34, l_pod_y + 98, 3, rgba(20, 20, 20, 255));

    /* Large Modern Digital Speed */
    int ispd = (int)roundf(speed);
    int spd_cx = l_pod_x + 125;
    draw_modern_number_centered(r, ispd, 1, false, spd_cx, l_pod_y + 75, 36, 58, 8, 7, text_wh);
    draw_text_centered(r, ui->use_mph ? "mph" : "km/h", spd_cx, l_pod_y + 144, 2, text_dim);

    /* ================= RIGHT FLOATING POD & POWER/CHARGE ARC ================= */
    int r_pod_w = 210;
    int r_pod_h = 240;
    int r_pod_x = SCREEN_W - 160 - r_pod_w;
    int r_pod_y = cy - r_pod_h / 2;

    /* Right Outer Sweeping Power Arc */
    int r_arc_cx = SCREEN_W + 35;
    int r_arc_rad = 195;
    draw_segmented_arc(r, r_arc_cx, cy, r_arc_rad, 5, 125.0f, 235.0f, 24, 1.0f, rgba(35, 50, 75, 255), rgba(0, 0, 0, 0));
    float pwr_frac = (charge_pct > 2.0f) ? (charge_pct / 100.0f) : (power_pct / 100.0f);
    if (pwr_frac > 0.02f) {
        float lit_ang = 180.0f + ((charge_pct > 2.0f) ? -pwr_frac * 50.0f : pwr_frac * 50.0f);
        if (charge_pct > 2.0f) {
            draw_segmented_arc(r, r_arc_cx, cy, r_arc_rad, 7, lit_ang, 180.0f, 16, 1.0f, volvo_green, rgba(0, 0, 0, 0));
        } else {
            draw_segmented_arc(r, r_arc_cx, cy, r_arc_rad, 7, 180.0f, lit_ang, 16, 1.0f, text_wh, rgba(0, 0, 0, 0));
        }
    }

    /* Right Glass Pod */
    draw_panel(r, r_pod_x, r_pod_y, r_pod_w, r_pod_h, rgba(10, 18, 30, 235), rgba(35, 55, 85, 255), 16);

    /* Green READY Badge */
    draw_text(r, "READY", r_pod_x + 32, r_pod_y + 24, 2, volvo_green);

    /* Dynamic Transmission Gear & PRND Column */
    bool is_p = (state->speed_kmh < 1.0f && state->current_gear <= 0);
    bool is_r = (state->current_gear == 0);
    bool is_n = (state->current_gear == -1);
    bool is_d = (state->current_gear >= 1);
    bool is_regen = (state->brake_pct > 12.0f);

    char v_gear_char = is_p ? 'P' : (is_r ? 'R' : (is_n ? 'N' : 'D'));
    draw_modern_char(r, v_gear_char, r_pod_x + 24, r_pod_y + 68, 36, 56, 7, is_r ? rgba(255, 195, 45, 255) : text_wh);

    if (is_d && state->current_gear > 0) {
        draw_modern_char(r, '0' + state->current_gear, r_pod_x + 64, r_pod_y + 76, 16, 26, 4, volvo_cyan);
    }

    draw_text(r, "R", r_pod_x + 88, r_pod_y + 68, 2, is_r ? rgba(255, 195, 45, 255) : text_dim);
    draw_text(r, "N", r_pod_x + 88, r_pod_y + 86, 2, is_n ? text_wh : text_dim);
    draw_text(r, "D", r_pod_x + 88, r_pod_y + 104, 2, is_d ? text_wh : text_dim);

    /* - B + (Regen One-Pedal Drive Level, lights up green during braking/regen) */
    draw_text(r, "- B +", r_pod_x + 48, r_pod_y + 130, 2, is_regen ? volvo_green : text_wh);

    /* Power / Charge Text on Right Side of Pod */
    draw_text(r, "power", r_pod_x + 145, r_pod_y + 80, 2, (state->throttle_pct > 10.0f) ? text_wh : text_dim);
    draw_text(r, "charge", r_pod_x + 145, r_pod_y + 115, 2, is_regen ? volvo_green : text_dim);
    draw_line(r, r_pod_x + 135, r_pod_y + 104, r_pod_x + 195, r_pod_y + 104, text_wh);

    /* ================= TOP STATUS BAR ================= */
    int t_row_y = content_y + 12;
    draw_icon_headlight(r, 420, t_row_y + 8, volvo_green);
    draw_icon_lane_assist(r, 590, t_row_y + 8, text_wh);
    char v_temp_buf[24];
    snprintf(v_temp_buf, sizeof(v_temp_buf), "%s", ui->use_mph ? "52 °F" : "11 °C");
    draw_text(r, v_temp_buf, 725, t_row_y, 2, text_wh);
    draw_icon_seatbelt(r, 775, t_row_y + 8, rgba(235, 45, 55, 255));
    draw_icon_car_topdown(r, 805, t_row_y + 8, 16, 22, text_wh, rgba(235, 45, 55, 255), true, true);

    /* ================= BOTTOM RIGHT DUAL BATTERY / FUEL RANGE STATUS ================= */
    int b_row_y = content_y + content_h - 38;

    /* Bottom Left: Volvo Trip & Telemetry Summary */
    char v_trip_buf[64];
    snprintf(v_trip_buf, sizeof(v_trip_buf), "LAP %d/%d   P%d", state->current_lap > 0 ? state->current_lap : 5, state->total_laps > 0 ? state->total_laps : 15, state->race_position > 0 ? state->race_position : 2);
    draw_text(r, v_trip_buf, 165, b_row_y - 12, 2, text_wh);
    char sc_fl = (state->surface_type[0] != 0) ? (char)state->surface_type[0] : 'T';
    snprintf(v_trip_buf, sizeof(v_trip_buf), "TYRES %.0f°C [%c]  |  AWD 46:54", state->tire_temp[0], sc_fl);
    draw_text(r, v_trip_buf, 165, b_row_y + 10, 1, volvo_cyan);

    int batt_km = (int)((state->fuel_pct > 0.0f ? state->fuel_pct : 85.0f) * (ui->use_mph ? 2.5f : 4.0f));
    int fuel_km = (int)(state->fuel_pct * (ui->use_mph ? 3.4f : 5.4f));

    /* Battery Icon & Bar */
    draw_icon_battery(r, 765, b_row_y - 12, 16, 10, (state->fuel_pct > 0.0f ? state->fuel_pct : 85.0f) / 100.0f, volvo_cyan);
    fill_rect(r, 790, b_row_y - 8, 40, 4, rgba(30, 45, 65, 255));
    fill_rect(r, 790, b_row_y - 8, (int)(40.0f * ((state->fuel_pct > 0.0f ? state->fuel_pct : 85.0f) / 100.0f)), 4, volvo_cyan);
    char v_bat_buf[32];
    snprintf(v_bat_buf, sizeof(v_bat_buf), "%d %s", batt_km, ui->use_mph ? "mi" : "km");
    draw_text(r, v_bat_buf, 840, b_row_y - 16, 2, volvo_cyan);

    /* Fuel Pump Icon & Bar */
    draw_icon_fuel_pump(r, 768, b_row_y + 8, text_wh);
    fill_rect(r, 790, b_row_y + 14, 40, 4, rgba(30, 45, 65, 255));
    fill_rect(r, 790, b_row_y + 14, (int)(40.0f * (state->fuel_pct / 100.0f)), 4, text_wh);
    char v_fuel_buf[32];
    snprintf(v_fuel_buf, sizeof(v_fuel_buf), "%d %s", fuel_km, ui->use_mph ? "mi" : "km");
    draw_text(r, v_fuel_buf, 840, b_row_y + 10, 2, text_wh);
}

static void render_theme_ev_cockpit(SDL_Renderer *r, const gt7_state_t *state, const hud_ui_state_t *ui, uint32_t now_ms, int top_y, int bot_y) {
    draw_ev_brand_selector(r, ui, top_y);
    int content_y = top_y + 40;
    int content_h = bot_y - content_y;

    switch (ui->ev_style) {
        case EV_STYLE_BENZ_EQ:    render_ev_benz_eq(r, state, ui, now_ms, content_y, content_h); break;
        case EV_STYLE_BMW_I:      render_ev_bmw_i(r, state, ui, now_ms, content_y, content_h); break;
        case EV_STYLE_AUDI_ETRON: render_ev_audi_etron(r, state, ui, now_ms, content_y, content_h); break;
        case EV_STYLE_VOLVO_EX:   render_ev_volvo_ex(r, state, ui, now_ms, content_y, content_h); break;
        default:                  render_ev_benz_eq(r, state, ui, now_ms, content_y, content_h); break;
    }
}

/* =========================================================================
 * THEME 3: F1 GRAND PRIX STEERING WHEEL DISPLAY (W14 / SF23 PCU-8D Style)
 * ========================================================================= */
static void render_theme_f1(SDL_Renderer *r, const gt7_state_t *state, const hud_ui_state_t *ui, uint32_t now_ms, int top_y, int bot_y) {
    rgba_t cyan = rgba(0, 230, 255, 255);
    rgba_t purple = rgba(190, 70, 255, 255);
    rgba_t amber = rgba(255, 195, 0, 255);

    draw_rpm_led_strip(r, state, now_ms, top_y, 28);
    int content_y = top_y + 38;
    int content_h = bot_y - content_y;
    int top_box_h = (int)(content_h * 0.34f);
    int bot_box_h = content_h - top_box_h - 12;

    /* Left Top: Lap Delta & Timing */
    draw_panel(r, 16, content_y, 352, top_box_h, rgba(0, 0, 0, 255), purple, 12);
    draw_text(r, "DELTA VS BEST", 34, content_y + 12, 2, purple);
    char dbuf[32];
    snprintf(dbuf, sizeof(dbuf), "%s%.3f", (state->delta_ms <= 0) ? "-" : "+", fabsf((float)state->delta_ms / 1000.0f));
    rgba_t dcol = (state->delta_ms <= 0) ? rgba(0, 245, 110, 255) : rgba(255, 65, 65, 255);
    draw_text_right(r, dbuf, 348, content_y + 36, 5, dcol);

    char lbuf[32];
    format_lap_time(state->best_lap_ms, lbuf, sizeof(lbuf));
    draw_text(r, "BEST", 34, content_y + top_box_h - 76, 2, rgba(160, 170, 190, 255));
    draw_text_right(r, lbuf, 348, content_y + top_box_h - 82, 3, purple);

    format_lap_time(state->last_lap_ms, lbuf, sizeof(lbuf));
    draw_text(r, "LAST", 34, content_y + top_box_h - 36, 2, rgba(160, 170, 190, 255));
    draw_text_right(r, lbuf, 348, content_y + top_box_h - 42, 3, rgba(235, 240, 250, 255));

    /* Sector Splits in Left Top Box */
    char s_f1_buf[64];
    snprintf(s_f1_buf, sizeof(s_f1_buf), "S1: %.2fs   S2: %.2fs   S3: %.2fs",
             (float)state->cur_sector_ms[0] / 1000.0f,
             (float)state->cur_sector_ms[1] / 1000.0f,
             (float)state->cur_sector_ms[2] / 1000.0f);
    draw_text(r, s_f1_buf, 34, content_y + top_box_h - 16, 1, cyan);

    /* Left Bottom: 4-Wheel Tire Heatmap */
    draw_tires_widget(r, state, 16, content_y + top_box_h + 12, 352, bot_box_h, cyan);

    /* Mini GPS Circuit Corner Radar in Top-Left of Center F1 Core */
    if (state->track_pt_count > 10) {
        int m_x = 450, m_y = content_y + 62, m_w = 104, m_h = 68;
        draw_track_map_geometry(r, state, m_x, m_y, m_w, m_h, 3, 1, false);
    }

    /* Center Core: F1 PCU-8D Ultra-Large Gear, Speed, RPM & Vertical Pedal Bars */
    draw_panel(r, 380, content_y, 520, content_h, rgba(0, 0, 0, 255), cyan, 18);

    int bar_y = content_y + 54;
    int bar_h = content_h - 165;
    fill_rect(r, 402, bar_y, 34, bar_h, rgba(35, 20, 25, 255));
    int brk_fill = (int)((state->brake_pct / 100.0f) * bar_h);
    fill_rect(r, 402, bar_y + bar_h - brk_fill, 34, brk_fill, rgba(255, 35, 55, 255));
    /* Peak Brake Hold Marker Line */
    int peak_by = bar_y + bar_h - (int)((state->peak_brake_pct / 100.0f) * bar_h);
    fill_rect(r, 400, peak_by, 38, 3, amber);
    draw_rect(r, 402, bar_y, 34, bar_h, 2, rgba(120, 50, 60, 255));
    draw_text_centered(r, "BRK", 419, bar_y + bar_h + 8, 2, rgba(255, 80, 95, 255));

    fill_rect(r, 844, bar_y, 34, bar_h, rgba(18, 35, 25, 255));
    int thr_fill = (int)((state->throttle_pct / 100.0f) * bar_h);
    fill_rect(r, 844, bar_y + bar_h - thr_fill, 34, thr_fill, rgba(0, 245, 100, 255));
    draw_rect(r, 844, bar_y, 34, bar_h, 2, rgba(50, 120, 75, 255));
    draw_text_centered(r, "THR", 861, bar_y + bar_h + 8, 2, rgba(50, 245, 120, 255));

    char rpm_str[32];
    snprintf(rpm_str, sizeof(rpm_str), "%05.0f RPM", state->engine_rpm);
    draw_text_centered(r, rpm_str, 640, content_y + 18, 3, amber);

    /* Category Badge on left & Steer angle on right */
    if (state->car_category[0]) {
        draw_text(r, state->car_category, 404, content_y + 20, 2, amber);
    }
    char f1_steer[32];
    snprintf(f1_steer, sizeof(f1_steer), "STEER %+.1f", state->steer_angle_fl_deg);
    draw_text_right(r, f1_steer, 878, content_y + 20, 2, cyan);

    bool rev_blink = (state->flags & GT7_FLAG_REV_BLINK) != 0;
    rgba_t gear_col = rev_blink ? rgba(0, 235, 255, 255) : rgba(255, 255, 255, 255);
    draw_7seg_char(r, gear_to_char(state->current_gear), 570, content_y + 72, 140, 230, 24, gear_col, rgba(24, 28, 40, 255));

    if (state->suggested_gear >= 1 && state->suggested_gear <= 8 && state->suggested_gear != state->current_gear) {
        fill_rect(r, 726, content_y + 125, 98, 118, rgba(235, 30, 45, 255));
        draw_rect(r, 726, content_y + 125, 98, 118, 3, rgba(255, 210, 210, 255));
        draw_text_centered(r, "SUGG", 775, content_y + 135, 2, rgba(255, 255, 255, 255));
        draw_7seg_char(r, (char)('0' + state->suggested_gear), 750, content_y + 162, 50, 70, 9, rgba(255, 255, 255, 255), rgba(0, 0, 0, 0));
    }

    int spd = (int)roundf(ui->use_mph ? state->speed_mph : state->speed_kmh);
    int spd_y = content_y + content_h - 235;
    draw_7seg_number(r, spd, 3, false, 715, spd_y, 64, 105, 13, 12, cyan, rgba(22, 28, 38, 255));
    draw_text(r, ui->use_mph ? "MPH" : "KM/H", 732, spd_y + 68, 3, cyan);

    int pill_y = content_y + content_h - 82;
    bool tcs_on = (state->flags & GT7_FLAG_TCS_ACTIVE) != 0;
    bool asm_on = (state->flags & GT7_FLAG_ASM_ACTIVE) != 0;
    bool drs_ready = (state->throttle_pct > 95.0f && state->current_gear >= 6);

    fill_rect(r, 402, pill_y, 145, 64, drs_ready ? rgba(0, 215, 90, 255) : rgba(24, 30, 40, 255));
    draw_text_centered(r, "DRS / OVR", 474, pill_y + 24, 2, drs_ready ? rgba(10, 14, 18, 255) : rgba(110, 120, 140, 255));

    fill_rect(r, 567, pill_y, 145, 64, tcs_on ? amber : rgba(24, 30, 40, 255));
    draw_text_centered(r, "TCS ACT", 639, pill_y + 24, 2, tcs_on ? rgba(10, 14, 18, 255) : rgba(110, 120, 140, 255));

    fill_rect(r, 732, pill_y, 145, 64, asm_on ? rgba(255, 60, 60, 255) : rgba(24, 30, 40, 255));
    draw_text_centered(r, "ASM / ABS", 804, pill_y + 24, 2, asm_on ? rgba(255, 255, 255, 255) : rgba(110, 120, 140, 255));

    /* Right Column: Race Position, Laps, Fuel & Telemetry Trace */
    int r_top_h = (int)(content_h * 0.68f);
    int r_bot_h = content_h - r_top_h - 12;
    draw_panel(r, 912, content_y, 352, r_top_h, rgba(0, 0, 0, 255), amber, 12);
    draw_text(r, "LAP", 932, content_y + 14, 2, amber);
    char lap_str[64];
    snprintf(lap_str, sizeof(lap_str), "%d / %d", state->current_lap > 0 ? state->current_lap : 1, state->total_laps > 0 ? state->total_laps : 15);
    draw_text(r, lap_str, 932, content_y + 38, 4, rgba(255, 255, 255, 255));

    draw_text_right(r, "POS", 1244, content_y + 14, 2, amber);
    snprintf(lap_str, sizeof(lap_str), "P%d", state->race_position > 0 ? state->race_position : 1);
    draw_text_right(r, lap_str, 1244, content_y + 38, 4, cyan);

    draw_text(r, "FUEL REMAINING", 932, content_y + 96, 2, rgba(170, 185, 205, 255));
    snprintf(lap_str, sizeof(lap_str), "%.1f%%", state->fuel_pct);
    draw_text_right(r, lap_str, 1244, content_y + 90, 3, (state->fuel_pct < 15.0f) ? rgba(255, 60, 60, 255) : rgba(0, 240, 120, 255));
    fill_rect(r, 932, content_y + 120, 312, 16, rgba(30, 36, 48, 255));
    fill_rect(r, 932, content_y + 120, (int)(3.12f * state->fuel_pct), 16, (state->fuel_pct < 15.0f) ? rgba(255, 60, 60, 255) : rgba(0, 230, 115, 255));

    float est_laps = (state->fuel_burn_per_lap > 0.1f) ? (state->fuel_pct / state->fuel_burn_per_lap) : (state->fuel_pct / 3.2f);
    snprintf(lap_str, sizeof(lap_str), "EST. +%.1f LAPS  (%.1f%%/L)", est_laps, state->fuel_burn_per_lap);
    draw_text(r, lap_str, 932, content_y + 146, 2, amber);

    snprintf(lap_str, sizeof(lap_str), "BOOST: %+.2f BAR (PK %+.2f)", state->turbo_bar, state->peak_turbo_bar);
    draw_text(r, lap_str, 932, content_y + 180, 2, cyan);
    snprintf(lap_str, sizeof(lap_str), "WATER: %.0fC   OIL: %.0fC", state->water_temp, state->oil_temp);
    draw_text(r, lap_str, 932, content_y + 208, 2, rgba(190, 205, 225, 255));
    snprintf(lap_str, sizeof(lap_str), "OIL PRESS: %.1f BAR  TOP: %.0f", state->oil_pressure, state->session_top_speed_kmh);
    draw_text(r, lap_str, 932, content_y + 236, 2, rgba(190, 205, 225, 255));

    /* F1 ERS / Hybrid Energy Bar & MGU-K Status */
    draw_text(r, "ERS / HYBRID DEPLOYMENT", 932, content_y + 264, 2, amber);
    float soc = (state->battery_soc_pct > 0.0f) ? state->battery_soc_pct : (78.4f - fmodf((float)now_ms * 0.00005f, 20.0f));
    fill_rect(r, 932, content_y + 284, 312, 12, rgba(20, 36, 48, 255));
    fill_rect(r, 932, content_y + 284, (int)(3.12f * soc), 12, cyan);
    snprintf(lap_str, sizeof(lap_str), "MGU-K: %+.0f KW   SOC: %.1f%%", state->energy_recovery_kw, soc);
    draw_text(r, lap_str, 932, content_y + 302, 2, cyan);

    draw_gforce_and_trace(r, state, 912, content_y + r_top_h + 12, 352, r_bot_h, cyan);
}

/* =========================================================================
 * THEME 2: GT3 ENDURANCE DIGITAL DASH (MoTeC C1212 / Bosch DDU Style)
 * ========================================================================= */
static void render_theme_gt3(SDL_Renderer *r, const gt7_state_t *state, const hud_ui_state_t *ui, uint32_t now_ms, int top_y, int bot_y) {
    (void)now_ms;
    rgba_t gold = rgba(255, 175, 20, 255);
    rgba_t green = rgba(0, 235, 110, 255);
    rgba_t white = rgba(245, 248, 255, 255);

    draw_panel(r, 16, top_y, 1248, 92, rgba(0, 0, 0, 255), gold, 10);
    int segs = 44;
    float max_rpm = (state->rpm_limiter > 1000) ? (float)state->rpm_limiter : 8800.0f;
    int lit_segs = (int)((state->engine_rpm / max_rpm) * segs);
    if (lit_segs > segs) lit_segs = segs;

    for (int i = 0; i < segs; ++i) {
        int sx = 34 + i * 27;
        rgba_t scol = rgba(245, 185, 20, 255);
        if (i >= 32) scol = rgba(255, 45, 45, 255);
        else if (i < 16) scol = rgba(230, 235, 245, 255);
        fill_rect(r, sx, top_y + 14, 22, 46, (i < lit_segs) ? scol : rgba(30, 34, 44, 255));
    }
    for (int rpm_k = 1; rpm_k <= 9; ++rpm_k) {
        char kbuf[8];
        snprintf(kbuf, sizeof(kbuf), "%d", rpm_k);
        int kx = 34 + (int)(((float)(rpm_k * 1000) / max_rpm) * (segs * 27));
        if (kx < 1230) draw_text_centered(r, kbuf, kx, top_y + 66, 2, rgba(160, 170, 190, 255));
    }

    int content_y = top_y + 102;
    int content_h = bot_y - content_y;
    int half_h = (content_h - 12) / 2;

    draw_panel(r, 16, content_y, 376, half_h, rgba(0, 0, 0, 255), gold, 10);
    draw_text(r, "LAP TIMING (GT3 SPEC)", 34, content_y + 14, 2, gold);
    char tbuf[64];
    format_lap_time(state->current_lap_ms, tbuf, sizeof(tbuf));
    draw_text(r, "CURR", 34, content_y + 48, 2, rgba(150, 160, 180, 255));
    draw_text_right(r, tbuf, 372, content_y + 40, 4, white);

    format_lap_time(state->best_lap_ms, tbuf, sizeof(tbuf));
    draw_text(r, "BEST", 34, content_y + 100, 2, rgba(150, 160, 180, 255));
    draw_text_right(r, tbuf, 372, content_y + 94, 3, green);

    format_lap_time(state->last_lap_ms, tbuf, sizeof(tbuf));
    draw_text(r, "LAST", 34, content_y + 142, 2, rgba(150, 160, 180, 255));
    draw_text_right(r, tbuf, 372, content_y + 136, 3, gold);

    /* Sector Splits & Real-time Delta in GT3 Spec Timing */
    snprintf(tbuf, sizeof(tbuf), "S1: %.2fs   S2: %.2fs   S3: %.2fs",
             (float)state->cur_sector_ms[0] / 1000.0f,
             (float)state->cur_sector_ms[1] / 1000.0f,
             (float)state->cur_sector_ms[2] / 1000.0f);
    draw_text(r, tbuf, 34, content_y + 170, 1, rgba(0, 225, 255, 255));
    snprintf(tbuf, sizeof(tbuf), "DELTA: %s%.3fs", (state->delta_ms <= 0) ? "-" : "+", fabsf((float)state->delta_ms / 1000.0f));
    draw_text(r, tbuf, 34, content_y + 190, 2, (state->delta_ms <= 0) ? green : rgba(255, 55, 65, 255));

    snprintf(tbuf, sizeof(tbuf), "LAP %d/%d   POS P%d/%d",
             state->current_lap > 0 ? state->current_lap : 1,
             state->total_laps > 0 ? state->total_laps : 15,
             state->race_position > 0 ? state->race_position : 1,
             state->total_cars > 0 ? state->total_cars : 20);
    draw_text(r, tbuf, 34, content_y + half_h - 32, 2, rgba(0, 225, 255, 255));

    draw_tires_widget(r, state, 16, content_y + half_h + 12, 376, content_h - half_h - 12, gold);

    /* Center MoTeC Core */
    draw_panel(r, 404, content_y, 472, content_h, rgba(0, 0, 0, 255), white, 14);
    draw_text_centered(r, "GEAR", 640, content_y + 16, 3, gold);
    if (state->car_category[0]) {
        draw_text(r, state->car_category, 422, content_y + 18, 2, gold);
    }
    char gt3_steer[32];
    snprintf(gt3_steer, sizeof(gt3_steer), "STEER %+.1f°", state->steer_angle_fl_deg);
    draw_text_right(r, gt3_steer, 858, content_y + 18, 2, white);

    bool shift_alert = (state->flags & GT7_FLAG_REV_BLINK) != 0;
    if (shift_alert) {
        fill_rect(r, 420, content_y + 42, 440, 245, rgba(215, 25, 35, 255));
    }
    draw_7seg_char(r, gear_to_char(state->current_gear), 562, content_y + 56, 156, 220, 25,
                   shift_alert ? white : gold, rgba(0, 0, 0, 0));

    if (state->suggested_gear >= 1 && state->suggested_gear <= 8) {
        fill_rect(r, 745, content_y + 98, 98, 122, rgba(240, 40, 40, 255));
        draw_text_centered(r, "SUGG", 794, content_y + 108, 2, white);
        draw_7seg_char(r, (char)('0' + state->suggested_gear), 769, content_y + 136, 48, 66, 9, white, rgba(0, 0, 0, 0));
    }

    int spd = (int)roundf(ui->use_mph ? state->speed_mph : state->speed_kmh);
    int spd_y = content_y + content_h - 210;
    draw_7seg_number(r, spd, 3, false, 705, spd_y, 58, 92, 11, 12, white, rgba(0, 0, 0, 0));
    draw_text(r, ui->use_mph ? "MPH" : "KM/H", 722, spd_y + 60, 3, gold);

    int pb_y = content_y + content_h - 86;
    draw_text(r, "THR", 426, pb_y + 2, 2, green);
    fill_rect(r, 485, pb_y, 366, 24, rgba(25, 35, 30, 255));
    fill_rect(r, 485, pb_y, (int)(3.66f * state->throttle_pct), 24, green);

    draw_text(r, "BRK", 426, pb_y + 38, 2, rgba(255, 55, 65, 255));
    fill_rect(r, 485, pb_y + 36, 366, 24, rgba(35, 25, 28, 255));
    fill_rect(r, 485, pb_y + 36, (int)(3.66f * state->brake_pct), 24, rgba(255, 45, 55, 255));

    /* Right Panel: Dual-Column Compact Endurance Vitals */
    draw_panel(r, 888, content_y, 376, half_h, rgba(0, 0, 0, 255), gold, 10);
    draw_text(r, "ENDURANCE FUEL & VITALS", 906, content_y + 14, 2, gold);

    float laps_left = (state->fuel_burn_per_lap > 0.1f) ? (state->fuel_pct / state->fuel_burn_per_lap) : (state->fuel_pct / 3.2f);
    snprintf(tbuf, sizeof(tbuf), "FUEL %.1f L", state->fuel_level);
    draw_text(r, tbuf, 906, content_y + 44, 2, green);
    snprintf(tbuf, sizeof(tbuf), "BOOST %+.2f BAR", state->turbo_bar);
    draw_text_right(r, tbuf, 1244, content_y + 44, 2, rgba(0, 225, 255, 255));

    snprintf(tbuf, sizeof(tbuf), "LAPS REM %.1f", laps_left);
    draw_text(r, tbuf, 906, content_y + 72, 2, green);
    snprintf(tbuf, sizeof(tbuf), "OIL %.0fC (%.1fB)", state->oil_temp, state->oil_pressure);
    draw_text_right(r, tbuf, 1244, content_y + 72, 2, white);

    snprintf(tbuf, sizeof(tbuf), "WATER %.0f C", state->water_temp);
    draw_text(r, tbuf, 906, content_y + 100, 2, white);
    snprintf(tbuf, sizeof(tbuf), "TOP %.1f KM/H", state->session_top_speed_kmh);
    draw_text_right(r, tbuf, 1244, content_y + 100, 2, gold);

    snprintf(tbuf, sizeof(tbuf), "TRANS %.0f C", state->trans_oil_temp);
    draw_text(r, tbuf, 906, content_y + 128, 2, rgba(175, 195, 220, 255));
    snprintf(tbuf, sizeof(tbuf), "SWAY %+.2fG", state->native_sway);
    draw_text_right(r, tbuf, 1244, content_y + 128, 2, gold);

    snprintf(tbuf, sizeof(tbuf), "AERO F%.0f/R%.0fK", state->downforce_front_kg, state->downforce_rear_kg);
    draw_text(r, tbuf, 906, content_y + 154, 2, rgba(185, 205, 230, 255));
    if (state->has_packet_tilde || fabsf(state->energy_recovery_kw) > 1.0f) {
        snprintf(tbuf, sizeof(tbuf), "HYBRID %+.0f KW", state->energy_recovery_kw);
    } else {
        snprintf(tbuf, sizeof(tbuf), "HEAVE %+.2fG", state->native_heave);
    }
    draw_text_right(r, tbuf, 1244, content_y + 154, 2, rgba(0, 225, 255, 255));

    bool tcs = (state->flags & GT7_FLAG_TCS_ACTIVE) != 0;
    bool asm_f = (state->flags & GT7_FLAG_ASM_ACTIVE) != 0;
    fill_rect(r, 906, content_y + half_h - 44, 162, 30, tcs ? gold : rgba(30, 35, 45, 255));
    draw_text_centered(r, "TC ACTIVE", 987, content_y + half_h - 36, 2, tcs ? rgba(10, 12, 16, 255) : rgba(100, 110, 130, 255));
    fill_rect(r, 1082, content_y + half_h - 44, 162, 30, asm_f ? rgba(255, 55, 55, 255) : rgba(30, 35, 45, 255));
    draw_text_centered(r, "ABS / ASM", 1163, content_y + half_h - 36, 2, asm_f ? white : rgba(100, 110, 130, 255));

    draw_gforce_and_trace(r, state, 888, content_y + half_h + 12, 376, content_h - half_h - 12, gold);
}

/* =========================================================================
 * THEME 3: WRC RALLY STAGE COCKPIT (High-Contrast Gear, Turbo & Pitch/Roll)
 * ========================================================================= */
static void render_theme_rally(SDL_Renderer *r, const gt7_state_t *state, const hud_ui_state_t *ui, uint32_t now_ms, int top_y, int bot_y) {
    rgba_t orange = rgba(255, 115, 0, 255);
    rgba_t yellow = rgba(255, 230, 0, 255);
    rgba_t white  = rgba(250, 250, 250, 255);

    int pairs = 10;
    float max_rpm = (state->rpm_limiter > 1000) ? (float)state->rpm_limiter : 7800.0f;
    int active_pairs = (int)(((state->engine_rpm - max_rpm * 0.45f) / (max_rpm * 0.52f)) * pairs);
    if (active_pairs < 0) active_pairs = 0;
    if (active_pairs > pairs) active_pairs = pairs;
    bool flash = (state->flags & GT7_FLAG_REV_BLINK) && ((now_ms / 60) % 2 == 0);

    for (int i = 0; i < pairs; ++i) {
        rgba_t col = (i < 5) ? rgba(0, 240, 90, 255) : ((i < 8) ? yellow : rgba(255, 30, 30, 255));
        if (flash) col = white;
        bool on = flash || (i < active_pairs);
        rgba_t c = on ? col : rgba(26, 30, 38, 255);
        int lx = 18 + i * 61;
        int rx = SCREEN_W - 18 - 54 - i * 61;
        fill_rect(r, lx, top_y, 54, 30, c);
        fill_rect(r, rx, top_y, 54, 30, c);
    }

    int content_y = top_y + 40;
    int content_h = bot_y - content_y;
    int top_box_h = (int)(content_h * 0.48f);
    int bot_box_h = content_h - top_box_h - 12;

    draw_panel(r, 16, content_y, 368, top_box_h, rgba(0, 0, 0, 255), orange, 12);
    draw_text(r, "TURBO ANTI-LAG BOOST", 34, content_y + 12, 2, orange);

    int bcx = 200, bcy = content_y + top_box_h / 2 + 8, brad = 84;
    draw_circle(r, bcx, bcy, brad, 4, rgba(60, 68, 80, 255));
    for (int b = -10; b <= 20; b += 5) {
        float norm = (float)(b + 10) / 30.0f;
        float ang = (135.0f + norm * 270.0f) * (float)M_PI / 180.0f;
        int x1 = bcx + (int)(cosf(ang) * (brad - 14));
        int y1 = bcy + (int)(sinf(ang) * (brad - 14));
        int x2 = bcx + (int)(cosf(ang) * brad);
        int y2 = bcy + (int)(sinf(ang) * brad);
        draw_thick_line(r, x1, y1, x2, y2, 3, (b >= 10) ? rgba(255, 60, 40, 255) : yellow);
    }
    float b_clamped = state->turbo_bar;
    if (b_clamped < -1.0f) b_clamped = -1.0f;
    if (b_clamped > 2.0f)  b_clamped = 2.0f;
    float b_ang = (135.0f + ((b_clamped + 1.0f) / 3.0f) * 270.0f) * (float)M_PI / 180.0f;
    draw_thick_line(r, bcx, bcy, bcx + (int)(cosf(b_ang) * (brad - 10)), bcy + (int)(sinf(b_ang) * (brad - 10)), 5, orange);
    fill_circle(r, bcx, bcy, 10, yellow);

    char bbuf[128];
    snprintf(bbuf, sizeof(bbuf), "%+.2f BAR", state->turbo_bar);
    draw_text_centered(r, bbuf, bcx, bcy + 36, 3, white);

    snprintf(bbuf, sizeof(bbuf), "PEAK %+.2fB  ALS ACT", state->peak_turbo_bar);
    draw_text_centered(r, bbuf, bcx, bcy + 60, 2, yellow);

    char sc_fl = (state->surface_type[0] != 0) ? (char)state->surface_type[0] : 'T';
    const char *s_name = (sc_fl == 'C' || sc_fl == 'c') ? "CURB" : ((sc_fl == 'D' || sc_fl == 'd') ? "DIRT / GRAVEL" : ((sc_fl == 'G' || sc_fl == 'g') ? "GRASS" : ((sc_fl == 'S' || sc_fl == 's') ? "SNOW / ICE" : "TARMAC")));
    snprintf(bbuf, sizeof(bbuf), "SURFACE: %s [%c]", s_name, sc_fl);
    draw_text_centered(r, bbuf, bcx, bcy + 80, 2, orange);

    draw_tires_widget(r, state, 16, content_y + top_box_h + 12, 368, bot_box_h, orange);

    /* Center Giant Rally Gear Drop Zone */
    draw_panel(r, 396, content_y, 488, content_h, rgba(0, 0, 0, 255), yellow, 16);
    fill_rect(r, 414, content_y + 14, 452, 42, orange);
    draw_text_centered(r, "WRC STAGE TELEMETRY", 640, content_y + 26, 3, rgba(12, 14, 16, 255));
    if (state->car_category[0]) {
        draw_text(r, state->car_category, 424, content_y + 26, 2, white);
    }

    int gbox_y = content_y + 72;
    fill_rect(r, 515, gbox_y, 250, 285, rgba(0, 0, 0, 255));
    draw_rect(r, 515, gbox_y, 250, 285, 4, yellow);
    draw_7seg_char(r, gear_to_char(state->current_gear), 550, gbox_y + 20, 180, 245, 28, yellow, rgba(36, 38, 45, 255));

    bool hbrake = (state->flags & GT7_FLAG_HANDBRAKE) != 0;
    int spd_y = gbox_y + 302;
    if (hbrake) {
        fill_rect(r, 418, spd_y, 444, 64, rgba(255, 20, 40, 255));
        draw_text_centered(r, "!! HANDBRAKE LOCK !!", 640, spd_y + 20, 4, white);
    } else {
        int spd = (int)roundf(ui->use_mph ? state->speed_mph : state->speed_kmh);
        draw_7seg_number(r, spd, 3, false, 685, spd_y, 62, 95, 12, 12, white, rgba(28, 32, 40, 255));
        draw_text(r, ui->use_mph ? "MPH" : "KM/H", 705, spd_y + 55, 3, orange);
    }

    fill_rect(r, 428, gbox_y, 30, 285, rgba(35, 20, 22, 255));
    int b_h = (int)((state->brake_pct / 100.0f) * 285);
    fill_rect(r, 428, gbox_y + 285 - b_h, 30, b_h, rgba(255, 40, 40, 255));
    draw_text_centered(r, "B", 443, gbox_y + 292, 2, rgba(255, 70, 70, 255));

    fill_rect(r, 822, gbox_y, 30, 285, rgba(20, 35, 22, 255));
    int t_h = (int)((state->throttle_pct / 100.0f) * 285);
    fill_rect(r, 822, gbox_y + 285 - t_h, 30, t_h, rgba(0, 245, 95, 255));
    draw_text_centered(r, "T", 837, gbox_y + 292, 2, rgba(0, 245, 95, 255));

    snprintf(bbuf, sizeof(bbuf), "STEER %+.1f DEG   AWD 50:50", state->steer_angle_fl_deg);
    draw_text_centered(r, bbuf, 640, content_y + content_h - 60, 2, white);

    snprintf(bbuf, sizeof(bbuf), "RPM: %.0f / %u", state->engine_rpm, state->rpm_limiter);
    draw_text_centered(r, bbuf, 640, content_y + content_h - 36, 3, yellow);

    /* Right Panel: Chassis Pitch/Roll Inclinometer */
    draw_panel(r, 896, content_y, 368, top_box_h, rgba(0, 0, 0, 255), orange, 12);
    draw_text(r, "CHASSIS ROLL / PITCH & RIDE", 914, content_y + 12, 2, orange);

    int icx = 1080, icy = content_y + top_box_h / 2 - 12, irad = 60;
    draw_circle(r, icx, icy, irad, 2, rgba(80, 90, 110, 255));
    float roll_rad = state->roll * (float)M_PI / 180.0f;
    int rx1 = icx - (int)(cosf(roll_rad) * (irad - 8));
    int ry1 = icy - (int)(sinf(roll_rad) * (irad - 8));
    int rx2 = icx + (int)(cosf(roll_rad) * (irad - 8));
    int ry2 = icy + (int)(sinf(roll_rad) * (irad - 8));
    draw_thick_line(r, rx1, ry1, rx2, ry2, 4, yellow);

    snprintf(bbuf, sizeof(bbuf), "ROLL: %+.1f DEG   PITCH: %+.1f DEG", state->roll, state->pitch);
    draw_text_centered(r, bbuf, 1080, content_y + top_box_h - 78, 2, white);
    snprintf(bbuf, sizeof(bbuf), "RIDE: %.0f MM   ROT: %+.1f R/S", state->body_height, state->steering_ang_vel);
    draw_text_centered(r, bbuf, 1080, content_y + top_box_h - 56, 2, orange);
    snprintf(bbuf, sizeof(bbuf), "SURGE: %+.2fG  HEAVE: %+.2fG  SWAY: %+.2fG", state->native_surge, state->native_heave, state->native_sway);
    draw_text_centered(r, bbuf, 1080, content_y + top_box_h - 34, 1, yellow);
    snprintf(bbuf, sizeof(bbuf), "TOP SPEED: %.1f KM/H", state->session_top_speed_kmh);
    draw_text_centered(r, bbuf, 1080, content_y + top_box_h - 16, 1, white);

    draw_gforce_and_trace(r, state, 896, content_y + top_box_h + 12, 368, bot_box_h, orange);
}

/* =========================================================================
 * THEME 4: JDM STREET TUNED CLUSTER (Defi Analog Tachometer + Boost Cluster)
 * ========================================================================= */
static void render_theme_jdm(SDL_Renderer *r, const gt7_state_t *state, const hud_ui_state_t *ui, uint32_t now_ms, int top_y, int bot_y) {
    (void)now_ms;
    rgba_t neon_blue = rgba(0, 195, 255, 255);
    rgba_t neon_red  = rgba(255, 35, 75, 255);
    rgba_t white     = rgba(245, 250, 255, 255);

    int content_h = bot_y - top_y;
    int top_box_h = (int)(content_h * 0.54f);
    int bot_box_h = content_h - top_box_h - 12;

    draw_panel(r, 16, top_y, 368, top_box_h, rgba(0, 0, 0, 255), neon_blue, 14);
    draw_text(r, "DEFI-LINK TURBO BOOST", 34, top_y + 14, 2, neon_blue);

    int bcx = 200, bcy = top_y + top_box_h / 2 + 12, brad = 104;
    draw_circle(r, bcx, bcy, brad, 4, neon_blue);
    draw_circle(r, bcx, bcy, brad - 8, 2, rgba(30, 50, 80, 255));

    for (int i = 0; i <= 30; ++i) {
        float norm = (float)i / 30.0f;
        float ang = (135.0f + norm * 270.0f) * (float)M_PI / 180.0f;
        int inner = (i % 5 == 0) ? (brad - 22) : (brad - 14);
        int x1 = bcx + (int)(cosf(ang) * inner);
        int y1 = bcy + (int)(sinf(ang) * inner);
        int x2 = bcx + (int)(cosf(ang) * (brad - 6));
        int y2 = bcy + (int)(sinf(ang) * (brad - 6));
        draw_thick_line(r, x1, y1, x2, y2, (i % 5 == 0) ? 3 : 1, (i >= 22) ? neon_red : white);
    }

    /* Peak Boost Hold Triangle Marker */
    float pk_val = state->peak_turbo_bar;
    if (pk_val < -1.0f) pk_val = -1.0f;
    if (pk_val > 2.0f)  pk_val = 2.0f;
    float pk_ang = (135.0f + ((pk_val + 1.0f) / 3.0f) * 270.0f) * (float)M_PI / 180.0f;
    fill_circle(r, bcx + (int)(cosf(pk_ang) * (brad - 4)), bcy + (int)(sinf(pk_ang) * (brad - 4)), 5, rgba(255, 220, 0, 255));

    float bval = state->turbo_bar;
    if (bval < -1.0f) bval = -1.0f;
    if (bval > 2.0f)  bval = 2.0f;
    float bang = (135.0f + ((bval + 1.0f) / 3.0f) * 270.0f) * (float)M_PI / 180.0f;
    draw_thick_line(r, bcx, bcy, bcx + (int)(cosf(bang) * (brad - 16)), bcy + (int)(sinf(bang) * (brad - 16)), 5, neon_red);
    fill_circle(r, bcx, bcy, 12, neon_red);

    char buf[64];
    snprintf(buf, sizeof(buf), "%+.2f BAR", state->turbo_bar);
    draw_text_centered(r, buf, bcx, bcy + 42, 3, white);

    snprintf(buf, sizeof(buf), "PEAK %+.2fB  VAC %+.2fB",
             state->peak_turbo_bar, (state->turbo_bar < 0.0f) ? state->turbo_bar : -0.65f);
    draw_text_centered(r, buf, bcx, top_y + top_box_h - 26, 2, rgba(255, 220, 0, 255));

    draw_tires_widget(r, state, 16, top_y + top_box_h + 12, 368, bot_box_h, neon_blue);

    /* Center Masterpiece: Large Circular Sweeping JDM Tachometer */
    draw_panel(r, 396, top_y, 488, content_h, rgba(0, 0, 0, 255), neon_blue, 18);
    snprintf(buf, sizeof(buf), "STEER %+.1f DEG", state->steer_angle_fl_deg);
    draw_text(r, buf, 416, top_y + 18, 2, neon_blue);
    if (state->car_category[0]) {
        draw_text_right(r, state->car_category, 864, top_y + 18, 2, neon_red);
    }
    int tcx = 640, tcy = top_y + content_h / 2, trad = 220;
    draw_circle(r, tcx, tcy, trad, 5, neon_blue);
    draw_circle(r, tcx, tcy, trad - 12, 2, rgba(25, 45, 75, 255));

    float max_rpm = (state->rpm_limiter > 1000) ? (float)state->rpm_limiter : 9000.0f;
    int max_k = (int)ceilf(max_rpm / 1000.0f);
    if (max_k < 8) max_k = 8;

    int total_ticks = max_k * 5;
    for (int i = 0; i <= total_ticks; ++i) {
        float norm = (float)i / (float)total_ticks;
        float ang = (135.0f + norm * 270.0f) * (float)M_PI / 180.0f;
        bool major = (i % 5 == 0);
        bool redzone = ((float)i / 5.0f * 1000.0f) >= (float)state->rpm_warning;
        int inner = major ? (trad - 32) : (trad - 18);
        int x1 = tcx + (int)(cosf(ang) * inner);
        int y1 = tcy + (int)(sinf(ang) * inner);
        int x2 = tcx + (int)(cosf(ang) * (trad - 6));
        int y2 = tcy + (int)(sinf(ang) * (trad - 6));
        draw_thick_line(r, x1, y1, x2, y2, major ? 4 : 2, redzone ? neon_red : white);
    }

    /* Needle rendered BEFORE numbers and readouts so dial text remains 100% visible */
    float rpm_norm = state->engine_rpm / (float)(max_k * 1000);
    if (rpm_norm < 0.0f) rpm_norm = 0.0f;
    if (rpm_norm > 1.0f) rpm_norm = 1.0f;
    float r_ang = (135.0f + rpm_norm * 270.0f) * (float)M_PI / 180.0f;
    draw_thick_line(r, tcx + (int)(cosf(r_ang) * 20), tcy + (int)(sinf(r_ang) * 20),
                    tcx + (int)(cosf(r_ang) * (trad - 20)), tcy + (int)(sinf(r_ang) * (trad - 20)), 5, neon_red);
    fill_circle(r, tcx, tcy, 18, rgba(30, 35, 45, 255));
    draw_circle(r, tcx, tcy, 18, 3, neon_red);

    /* Major graduation numbers rendered on top */
    for (int i = 0; i <= total_ticks; i += 5) {
        float norm = (float)i / (float)total_ticks;
        float ang = (135.0f + norm * 270.0f) * (float)M_PI / 180.0f;
        bool redzone = ((float)i / 5.0f * 1000.0f) >= (float)state->rpm_warning;
        char kstr[16];
        snprintf(kstr, sizeof(kstr), "%d", i / 5);
        int tx = tcx + (int)(cosf(ang) * (trad - 54));
        int ty = tcy + (int)(sinf(ang) * (trad - 54)) - 8;
        draw_text_centered(r, kstr, tx, ty, 3, redzone ? neon_red : neon_blue);
    }

    draw_7seg_char(r, gear_to_char(state->current_gear), tcx - 34, tcy - 120, 68, 96, 12,
                   (state->flags & GT7_FLAG_REV_BLINK) ? neon_red : white, rgba(0, 0, 0, 0));

    int spd = (int)roundf(ui->use_mph ? state->speed_mph : state->speed_kmh);
    draw_7seg_number_centered(r, spd, 3, false, tcx, tcy + 50, 44, 72, 9, 8, neon_blue, rgba(0, 0, 0, 0));
    draw_text_centered(r, ui->use_mph ? "MPH" : "KM/H", tcx, tcy + 134, 2, white);

    bool shift_lamp = (state->flags & GT7_FLAG_REV_BLINK) != 0;
    fill_circle(r, 830, top_y + 56, 26, shift_lamp ? neon_red : rgba(35, 20, 25, 255));
    draw_circle(r, 830, top_y + 56, 26, 3, white);
    draw_text_centered(r, "SHIFT", 830, top_y + 90, 2, shift_lamp ? neon_red : rgba(120, 130, 150, 255));

    /* Right Panel: JDM Advance ZD Multi-Meter */
    draw_panel(r, 896, top_y, 368, top_box_h, rgba(0, 0, 0, 255), neon_blue, 14);
    draw_text(r, "ADVANCE ZD MULTI-DISPLAY", 914, top_y + 14, 2, neon_blue);

    snprintf(buf, sizeof(buf), "WATER TEMP     %.1f C", state->water_temp);
    draw_text(r, buf, 916, top_y + 52, 2, white);
    fill_rect(r, 916, top_y + 72, 326, 10, rgba(25, 35, 50, 255));
    fill_rect(r, 916, top_y + 72, (int)((state->water_temp / 130.0f) * 326), 10, neon_blue);

    snprintf(buf, sizeof(buf), "OIL TEMP       %.1f C", state->oil_temp);
    draw_text(r, buf, 916, top_y + 96, 2, white);
    fill_rect(r, 916, top_y + 116, 326, 10, rgba(25, 35, 50, 255));
    fill_rect(r, 916, top_y + 116, (int)((state->oil_temp / 150.0f) * 326), 10, rgba(255, 175, 0, 255));

    snprintf(buf, sizeof(buf), "OIL PRESS      %.1f BAR", state->oil_pressure);
    draw_text(r, buf, 916, top_y + 140, 2, white);
    fill_rect(r, 916, top_y + 160, 326, 10, rgba(25, 35, 50, 255));
    fill_rect(r, 916, top_y + 160, (int)((state->oil_pressure / 8.0f) * 326), 10, rgba(0, 240, 120, 255));

    format_lap_time(state->best_lap_ms, buf, sizeof(buf));
    draw_text(r, "BEST LAP", 916, top_y + 196, 2, neon_blue);
    draw_text_right(r, buf, 1244, top_y + 190, 3, white);

    format_lap_time(state->last_lap_ms, buf, sizeof(buf));
    draw_text(r, "LAST LAP", 916, top_y + 236, 2, neon_blue);
    draw_text_right(r, buf, 1244, top_y + 230, 3, rgba(200, 210, 230, 255));

    snprintf(buf, sizeof(buf), "DELTA %s%.3fs   TOP: %.0f KM/H",
             (state->delta_ms <= 0) ? "-" : "+", fabsf((float)state->delta_ms / 1000.0f), state->session_top_speed_kmh);
    draw_text(r, buf, 916, top_y + 264, 2, (state->delta_ms <= 0) ? rgba(0, 240, 120, 255) : neon_red);

    snprintf(buf, sizeof(buf), "FUEL:%.0f%%  THR:%.0f%%  BRK:%.0f%%", state->fuel_pct, state->throttle_pct, state->brake_pct);
    draw_text(r, buf, 916, top_y + top_box_h - 26, 2, neon_blue);

    draw_gforce_and_trace(r, state, 896, top_y + top_box_h + 12, 368, bot_box_h, neon_blue);
}

/* =========================================================================
 * THEME 5: TRACK INFO & RACE ENGINEER MODE (GPS Map, Sectors & Stint Table)
 * ========================================================================= */
static void render_theme_track(SDL_Renderer *r, const gt7_state_t *state, const hud_ui_state_t *ui, uint32_t now_ms, int top_y, int bot_y) {
    (void)now_ms;
    rgba_t emerald = rgba(0, 240, 155, 255);
    rgba_t cyan    = rgba(0, 215, 255, 255);
    rgba_t purple  = rgba(195, 80, 255, 255);
    rgba_t amber   = rgba(255, 195, 0, 255);
    rgba_t white   = rgba(245, 250, 255, 255);

    /* Top Compact Telemetry Strip (Always keeps Gear, Speed, RPM & Pedals visible while inspecting Track Info) */
    draw_panel(r, 16, top_y, 1248, 76, rgba(0, 0, 0, 255), emerald, 10);

    draw_text(r, "GEAR", 34, top_y + 12, 2, emerald);
    draw_7seg_char(r, gear_to_char(state->current_gear), 102, top_y + 10, 38, 56, 7, white, rgba(28, 34, 46, 255));

    int spd = (int)roundf(ui->use_mph ? state->speed_mph : state->speed_kmh);
    draw_7seg_number(r, spd, 3, false, 295, top_y + 10, 34, 56, 6, 6, cyan, rgba(25, 32, 44, 255));
    draw_text(r, ui->use_mph ? "MPH" : "KM/H", 306, top_y + 42, 2, cyan);

    char cbuf[256];
    snprintf(cbuf, sizeof(cbuf), "RPM %.0f", state->engine_rpm);
    draw_text(r, cbuf, 395, top_y + 14, 2, amber);
    fill_rect(r, 395, top_y + 38, 210, 18, rgba(25, 32, 44, 255));
    float rfrac = state->engine_rpm / ((state->rpm_limiter > 1000) ? (float)state->rpm_limiter : 9000.0f);
    if (rfrac > 1.0f) rfrac = 1.0f;
    fill_rect(r, 395, top_y + 38, (int)(210 * rfrac), 18, (rfrac > 0.88f) ? rgba(255, 45, 65, 255) : emerald);

    draw_text(r, "THR / BRK", 630, top_y + 14, 2, rgba(170, 185, 205, 255));
    fill_rect(r, 630, top_y + 34, 150, 10, rgba(22, 34, 28, 255));
    fill_rect(r, 630, top_y + 34, (int)(1.5f * state->throttle_pct), 10, rgba(0, 240, 100, 255));
    fill_rect(r, 630, top_y + 48, 150, 10, rgba(36, 22, 26, 255));
    fill_rect(r, 630, top_y + 48, (int)(1.5f * state->brake_pct), 10, rgba(255, 50, 60, 255));

    snprintf(cbuf, sizeof(cbuf), "LAP %d/%d  |  POS P%d/%d",
             state->current_lap > 0 ? state->current_lap : 5,
             state->total_laps > 0 ? state->total_laps : 15,
             state->race_position > 0 ? state->race_position : 2,
             state->total_cars > 0 ? state->total_cars : 20);
    draw_text(r, cbuf, 815, top_y + 16, 2, white);

    snprintf(cbuf, sizeof(cbuf), "TOP: %.1fK  FUEL: %.0f%%", state->session_top_speed_kmh, state->fuel_pct);
    draw_text(r, cbuf, 815, top_y + 44, 2, emerald);

    if (state->car_category[0]) {
        fill_rect(r, 1146, top_y + 12, 94, 22, rgba(16, 28, 45, 255));
        draw_rect(r, 1146, top_y + 12, 94, 22, 1, cyan);
        draw_text_centered(r, state->car_category, 1193, top_y + 15, 2, cyan);
    }
    snprintf(cbuf, sizeof(cbuf), "STEER %+.1f", state->steer_angle_fl_deg);
    draw_text_right(r, cbuf, 1240, top_y + 44, 2, amber);

    int main_y = top_y + 88;
    int main_h = bot_y - main_y;

    /* Left Half: Large 2D GPS Circuit Map with Braking/Acceleration Heatmap */
    draw_circuit_map_widget(r, state, 16, main_y, 585, main_h, emerald, true);

    /* Right Top: Sector Split Analysis (S1 / S2 / S3 + Theoretical Optimal Lap) */
    int r_top_h = (int)(main_h * 0.44f);
    int r_bot_h = main_h - r_top_h - 12;
    draw_panel(r, 613, main_y, 651, r_top_h, rgba(0, 0, 0, 255), cyan, 12);
    draw_text(r, "SECTOR SPLIT ANALYSIS & OPTIMAL LAP", 631, main_y + 12, 2, cyan);

    int sw = 195, sh = r_top_h - 92;
    for (int s = 0; s < 3; ++s) {
        int sx = 631 + s * (sw + 14);
        int sy = main_y + 38;
        bool active_sec = (state->current_sector == s);
        bool purple_sec = (state->cur_sector_ms[s] <= state->best_sector_ms[s]);

        rgba_t s_border = active_sec ? amber : (purple_sec ? purple : emerald);
        fill_rect(r, sx, sy, sw, sh, rgba(0, 0, 0, 255));
        draw_rect(r, sx, sy, sw, sh, 2, s_border);

        snprintf(cbuf, sizeof(cbuf), "SECTOR %d %s", s + 1, active_sec ? "[LIVE]" : "");
        draw_text(r, cbuf, sx + 12, sy + 10, 2, s_border);

        char s_cur[32], s_best[32];
        format_sector_time(state->cur_sector_ms[s], s_cur, sizeof(s_cur));
        format_sector_time(state->best_sector_ms[s], s_best, sizeof(s_best));
        draw_text(r, s_cur, sx + 12, sy + 36, 3, white);

        snprintf(cbuf, sizeof(cbuf), "BEST: %s", s_best);
        draw_text(r, cbuf, sx + 12, sy + sh - 22, 2, purple_sec ? purple : rgba(155, 170, 195, 255));
    }

    char opt_str[32], best_str[32];
    format_lap_time(state->optimal_lap_ms, opt_str, sizeof(opt_str));
    format_lap_time(state->best_lap_ms, best_str, sizeof(best_str));
    snprintf(cbuf, sizeof(cbuf), "THEORETICAL OPTIMAL: %s", opt_str);
    draw_text(r, cbuf, 631, main_y + r_top_h - 32, 2, purple);
    snprintf(cbuf, sizeof(cbuf), "BEST: %s", best_str);
    draw_text_right(r, cbuf, 1244, main_y + r_top_h - 32, 2, amber);

    /* Right Bottom: Stint Lap History Table & Pit Stop Strategy Calculator */
    int sy2 = main_y + r_top_h + 12;
    draw_panel(r, 613, sy2, 651, r_bot_h, rgba(0, 0, 0, 255), amber, 12);
    draw_text(r, "STINT LAP HISTORY & PIT WINDOW STRATEGY", 631, sy2 + 12, 2, amber);

    draw_text(r, "LAP     TIME         S1        S2        S3       FUEL    TOP SPD", 631, sy2 + 40, 2, rgba(145, 160, 185, 255));
    SDL_RenderDrawLine(r, 631, sy2 + 58, 1244, sy2 + 58);

    int show_laps = state->lap_history_count > 4 ? 4 : state->lap_history_count;
    for (int i = 0; i < show_laps; ++i) {
        const gt7_lap_record_t *rec = &state->lap_history[state->lap_history_count - show_laps + i];
        int ry = sy2 + 66 + i * 28;
        char t_str[24], s1_s[16], s2_s[16], s3_s[16];
        format_lap_time(rec->time_ms, t_str, sizeof(t_str));
        format_sector_time(rec->s1_ms, s1_s, sizeof(s1_s));
        format_sector_time(rec->s2_ms, s2_s, sizeof(s2_s));
        format_sector_time(rec->s3_ms, s3_s, sizeof(s3_s));

        bool is_best = (rec->time_ms == state->best_lap_ms);
        snprintf(cbuf, sizeof(cbuf), "L%02d   %s   %s    %s    %s    -%.1f%%   %.1fK",
                 rec->lap_num, t_str, s1_s, s2_s, s3_s, rec->fuel_used, rec->top_speed_kmh);
        draw_text(r, cbuf, 631, ry, 2, is_best ? purple : white);
    }

    /* Pit Strategy Summary Footer */
    float est_laps = (state->fuel_burn_per_lap > 0.1f) ? (state->fuel_pct / state->fuel_burn_per_lap) : (state->fuel_pct / 3.2f);
    int pit_lap = (state->current_lap > 0 ? state->current_lap : 5) + (int)floorf(est_laps);
    fill_rect(r, 631, sy2 + r_bot_h - 46, 613, 32, rgba(22, 30, 44, 255));
    char sc_fl = (state->surface_type[0] != 0) ? (char)state->surface_type[0] : 'T';
    snprintf(cbuf, sizeof(cbuf), "BOX L%d (%.1f LAPS) | TIRE %.0fC [%c] | SWAY %+.2fG",
             pit_lap, est_laps,
             (state->tire_temp[0] + state->tire_temp[1] + state->tire_temp[2] + state->tire_temp[3]) * 0.25f,
             sc_fl, state->native_sway);
    draw_text(r, cbuf, 643, sy2 + r_bot_h - 37, 2, emerald);
}

/* =========================================================================
 * TOP STATUS BAR, BOTTOM TOUCH DOCK & TOUCH IP NUMPAD MODAL
 * ========================================================================= */
static const char *THEME_SHORT_NAMES[HUD_THEME_COUNT] = {
    "1: GT7",
    "2: DIGI",
    "3: EV",
    "4: F1",
    "5: GT3",
    "6: WRC",
    "7: JDM",
    "8: MAP"
};

static const char *THEME_FULL_NAMES[HUD_THEME_COUNT] = {
    "MODE 1: OFFICIAL GRAN TURISMO 7 NATIVE HUD & MFD",
    "MODE 2: HIGH-RES DIGITAL TACHOMETER & TELEMETRY DDU",
    "MODE 3: LUXURY EV DIGITAL COCKPIT (BENZ / BMW / AUDI / VOLVO)",
    "MODE 4: F1 GRAND PRIX STEERING WHEEL",
    "MODE 5: GT3 ENDURANCE MoTeC DDU",
    "MODE 6: WRC RALLY STAGE COCKPIT",
    "MODE 7: JDM DEFI TUNED MULTI-GAUGE",
    "MODE 8: GPS TRACK MAP & RACE ENGINEER"
};

static void render_ip_modal(SDL_Renderer *r, const gt7_client_t *client, hud_ui_state_t *ui) {
    fill_rect(r, 0, 0, SCREEN_W, SCREEN_H, rgba(0, 0, 0, 225));
    int mw = 600, mh = 570;
    int mx = (SCREEN_W - mw) / 2;
    int my = (SCREEN_H - mh) / 2;

    draw_panel(r, mx, my, mw, mh, rgba(0, 0, 0, 255), rgba(0, 225, 255, 255), 16);
    draw_text_centered(r, "PS5 / PS5 PRO IP & TELEMETRY SETUP", SCREEN_W / 2, my + 18, 2, rgba(0, 225, 255, 255));

    char sub_info[256];
    snprintf(sub_info, sizeof(sub_info), "LOCAL IP: %s    AUTO-SCAN: %s.1-254", client->local_ip, client->subnet_prefix);
    draw_text_centered(r, sub_info, SCREEN_W / 2, my + 46, 2, rgba(160, 185, 215, 255));

    fill_rect(r, mx + 40, my + 72, mw - 80, 50, rgba(0, 0, 0, 255));
    draw_rect(r, mx + 40, my + 72, mw - 80, 50, 2, rgba(0, 225, 255, 255));
    draw_text_centered(r, ui->edit_ip[0] ? ui->edit_ip : "AUTO-DETECT OR TAP IP...", SCREEN_W / 2, my + 86, 4, rgba(255, 255, 255, 255));

    static const char *keys[12] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", ".", "0", "DEL"};
    int kw = 156, kh = 50, gap = 10;
    int gx = mx + (mw - (kw * 3 + gap * 2)) / 2;
    int gy = my + 132;

    for (int i = 0; i < 12; ++i) {
        int row = i / 3;
        int col = i % 3;
        int bx = gx + col * (kw + gap);
        int by = gy + row * (kh + gap);
        fill_rect(r, bx, by, kw, kh, rgba(32, 42, 62, 255));
        draw_rect(r, bx, by, kw, kh, 2, rgba(80, 105, 145, 255));
        draw_text_centered(r, keys[i], bx + kw / 2, by + 14, 3, rgba(245, 250, 255, 255));
    }

    /* Dedicated Heartbeat Protocol Row */
    int proto_y = gy + 4 * (kh + gap) + 8;
    draw_text_centered(r, "GT7 TELEMETRY PROTOCOL HEARTBEAT:", SCREEN_W / 2, proto_y, 1, rgba(160, 185, 215, 255));

    static const char *proto_labels[4] = {"AUTO (SMART)", "PKT C (368B)", "PKT ~ (344B)", "PKT A (296B)"};
    int pw = 124, ph = 36, pgap = 10;
    int px_start = mx + (mw - (pw * 4 + pgap * 3)) / 2;
    int proto_btn_y = proto_y + 16;
    for (int p = 0; p < 4; ++p) {
        int px = px_start + p * (pw + pgap);
        bool selected = (client->heartbeat_mode == (gt7_heartbeat_mode_t)p);
        fill_rect(r, px, proto_btn_y, pw, ph, selected ? rgba(0, 215, 255, 255) : rgba(24, 34, 52, 255));
        draw_rect(r, px, proto_btn_y, pw, ph, 1, selected ? rgba(255, 255, 255, 255) : rgba(70, 95, 130, 255));
        draw_text_centered(r, proto_labels[p], px + pw / 2, proto_btn_y + 11, 1,
                           selected ? rgba(0, 0, 0, 255) : rgba(200, 220, 245, 255));
    }

    /* 3 Action Buttons: [AUTO DETECT (DEFAULT)], [SET MANUAL IP], [CANCEL] */
    int by = proto_btn_y + ph + 16;
    fill_rect(r, gx, by, 180, 52, rgba(0, 215, 255, 255));
    draw_text_centered(r, "AUTO DETECT", gx + 90, by + 18, 2, rgba(10, 14, 20, 255));

    fill_rect(r, gx + 192, by, 160, 52, rgba(0, 210, 100, 255));
    draw_text_centered(r, "MANUAL IP", gx + 272, by + 18, 2, rgba(10, 14, 20, 255));

    fill_rect(r, gx + 364, by, 124, 52, rgba(215, 50, 60, 255));
    draw_text_centered(r, "CANCEL", gx + 426, by + 18, 2, rgba(255, 255, 255, 255));
}

void hud_render(SDL_Renderer *r, const gt7_state_t *state, const gt7_client_t *client, hud_ui_state_t *ui, uint32_t now_ms) {
    draw_oled_black_background(r);

    /* Auto-hide dock in Fullscreen Canvas mode after 4 seconds of no touch */
    if (ui->fullscreen_canvas && ui->dock_visible && now_ms > ui->dock_hide_at_ms && !ui->show_ip_modal) {
        ui->dock_visible = false;
    }

    bool show_bars = !ui->fullscreen_canvas || ui->dock_visible;
    int top_y = show_bars ? 46 : 10;
    int bot_y = show_bars ? 662 : 710;

    /* Render Active Motorsport Dashboard Mode (Dynamically stretches to 100% full screen when bars are hidden!) */
    switch (ui->theme) {
        case HUD_THEME_GT7:        render_theme_gt7_official(r, state, ui, now_ms, top_y, bot_y); break;
        case HUD_THEME_DIGITACH:   render_theme_digitach(r, state, ui, now_ms, top_y, bot_y); break;
        case HUD_THEME_EV_COCKPIT: render_theme_ev_cockpit(r, state, ui, now_ms, top_y, bot_y); break;
        case HUD_THEME_F1:         render_theme_f1(r, state, ui, now_ms, top_y, bot_y); break;
        case HUD_THEME_GT3:        render_theme_gt3(r, state, ui, now_ms, top_y, bot_y); break;
        case HUD_THEME_RALLY:      render_theme_rally(r, state, ui, now_ms, top_y, bot_y); break;
        case HUD_THEME_JDM:        render_theme_jdm(r, state, ui, now_ms, top_y, bot_y); break;
        case HUD_THEME_TRACK:      render_theme_track(r, state, ui, now_ms, top_y, bot_y); break;
        default:                   render_theme_gt7_official(r, state, ui, now_ms, top_y, bot_y); break;
    }

    if (show_bars) {
        /* Top Status & Quick Action Bar */
        fill_rect(r, 0, 0, SCREEN_W, 40, rgba(0, 0, 0, 255));
        set_color(r, rgba(0, 215, 255, 140));
        SDL_RenderDrawLine(r, 0, 40, SCREEN_W, 40);
        draw_text(r, "GT7 DDU", 16, 12, 2, rgba(240, 245, 255, 255));
        draw_text(r, GT7_APP_VERSION, 106, 15, 1, rgba(140, 160, 190, 255));

        /* Telemetry Protocol Lock Badge: PKT C (368B) / PKT ~ (344B) / PKT A (296B) */
        char pkt_badge[32];
        if (state->has_packet_c) {
            snprintf(pkt_badge, sizeof(pkt_badge), "PKT C (368B)");
        } else if (state->has_packet_tilde) {
            snprintf(pkt_badge, sizeof(pkt_badge), "PKT ~ (344B)");
        } else if (state->has_packet_b) {
            snprintf(pkt_badge, sizeof(pkt_badge), "PKT B (316B)");
        } else {
            snprintf(pkt_badge, sizeof(pkt_badge), "PKT A (296B)");
        }
        fill_rect(r, 225, 7, 108, 26, rgba(14, 25, 42, 255));
        draw_rect(r, 225, 7, 108, 26, 1, rgba(0, 225, 255, 200));
        draw_text_centered(r, pkt_badge, 279, 14, 1, rgba(0, 225, 255, 255));

        char status_str[128];
        rgba_t status_col;
        if (state->connected) {
            snprintf(status_str, sizeof(status_str), "LIVE PS5 (%s) 60HZ", client->ps5_ip);
            status_col = rgba(0, 240, 110, 255);
        } else if (client->auto_detect) {
            snprintf(status_str, sizeof(status_str), "AUTO PS5 (%s.*) %s",
                     client->subnet_prefix, state->demo_mode ? "| DEMO" : "");
            status_col = rgba(0, 220, 255, 255);
        } else {
            snprintf(status_str, sizeof(status_str), "MANUAL PS5 (%s) %s",
                     client->ps5_ip, state->demo_mode ? "| DEMO" : "");
            status_col = rgba(255, 195, 0, 255);
        }
        fill_circle(r, 350, 19, 6, status_col);
        draw_text(r, status_str, 365, 12, 2, status_col);

        /* Top-Right Touch Buttons: [RESET LAPS] [FULLSCREEN] [EXIT APP] (Works without Joy-Cons!) */
        fill_rect(r, 835, 5, 135, 30, rgba(0, 0, 0, 255));
        draw_rect(r, 835, 5, 135, 30, 1, rgba(90, 110, 140, 255));
        draw_text_centered(r, "RESET LAPS", 902, 13, 2, rgba(210, 225, 245, 255));

        fill_rect(r, 980, 5, 165, 30, ui->fullscreen_canvas ? rgba(0, 210, 120, 255) : rgba(0, 165, 225, 255));
        draw_text_centered(r, ui->fullscreen_canvas ? "EXIT FULLSCR" : "FULLSCREEN", 1062, 13, 2, rgba(0, 0, 0, 255));

        fill_rect(r, 1155, 5, 110, 30, rgba(215, 45, 60, 255));
        draw_text_centered(r, "EXIT APP", 1210, 13, 2, rgba(255, 255, 255, 255));

        /* Bottom Touch Control Dock (8 Mode Tabs + PS5 IP + Demo + Unit + Fullscreen) */
        int dock_y = 668, dock_h = 52;
        fill_rect(r, 0, dock_y, SCREEN_W, dock_h, rgba(0, 0, 0, 255));
        set_color(r, rgba(0, 215, 255, 140));
        SDL_RenderDrawLine(r, 0, dock_y, SCREEN_W, dock_y);

        for (int i = 0; i < HUD_THEME_COUNT; ++i) {
            int bx = 8 + i * 89;
            bool active = ((int)ui->theme == i);
            fill_rect(r, bx, dock_y + 8, 86, 36, active ? rgba(0, 215, 255, 255) : rgba(0, 0, 0, 255));
            draw_rect(r, bx, dock_y + 8, 86, 36, 1, active ? rgba(255, 255, 255, 220) : rgba(50, 68, 92, 255));
            draw_text_centered(r, THEME_SHORT_NAMES[i], bx + 43, dock_y + 19, 2,
                               active ? rgba(0, 0, 0, 255) : rgba(200, 215, 235, 255));
        }

        fill_rect(r, 730, dock_y + 8, 126, 36, client->auto_detect ? rgba(10, 45, 65, 255) : rgba(0, 0, 0, 255));
        draw_rect(r, 730, dock_y + 8, 126, 36, 1, rgba(0, 215, 255, 255));
        draw_text_centered(r, client->auto_detect ? "IP: AUTO" : "IP: MANU", 793, dock_y + 19, 2, rgba(0, 225, 255, 255));

        fill_rect(r, 866, dock_y + 8, 126, 36, state->demo_mode ? rgba(225, 155, 0, 255) : rgba(0, 195, 100, 255));
        draw_text_centered(r, state->demo_mode ? "DEMO ON" : "LIVE PS5", 929, dock_y + 19, 2, rgba(0, 0, 0, 255));

        fill_rect(r, 1002, dock_y + 8, 118, 36, rgba(0, 0, 0, 255));
        draw_rect(r, 1002, dock_y + 8, 118, 36, 1, rgba(60, 80, 110, 255));
        draw_text_centered(r, ui->use_mph ? "MPH" : "KM/H", 1061, dock_y + 19, 2, rgba(240, 245, 255, 255));

        fill_rect(r, 1130, dock_y + 8, 142, 36, ui->fullscreen_canvas ? rgba(0, 215, 130, 255) : rgba(0, 0, 0, 255));
        draw_rect(r, 1130, dock_y + 8, 142, 36, 1, ui->fullscreen_canvas ? rgba(255, 255, 255, 220) : rgba(60, 80, 110, 255));
        draw_text_centered(r, "FULL", 1201, dock_y + 19, 2, ui->fullscreen_canvas ? rgba(0, 0, 0, 255) : rgba(245, 250, 255, 255));
    } else {
        /* Subtle translucent corner buttons in pure Fullscreen mode so user can exit fullscreen or pop up menu with 1 tap */
        draw_panel(r, 12, SCREEN_H - 38, 152, 28, rgba(14, 18, 28, 175), rgba(0, 215, 255, 130), 6);
        draw_text_centered(r, "EXIT FULLSCR", 88, SCREEN_H - 30, 2, rgba(0, 225, 255, 220));

        draw_panel(r, SCREEN_W - 148, SCREEN_H - 38, 136, 28, rgba(14, 18, 28, 175), rgba(0, 215, 255, 130), 6);
        draw_text_centered(r, "TAP: MENU", SCREEN_W - 80, SCREEN_H - 30, 2, rgba(180, 210, 240, 220));
    }

    if (now_ms < ui->theme_banner_until_ms) {
        draw_panel(r, SCREEN_W / 2 - 290, 50, 580, 46, rgba(0, 215, 255, 242), rgba(255, 255, 255, 255), 10);
        if (ui->theme == HUD_THEME_EV_COCKPIT) {
            static const char *ev_titles[EV_STYLE_COUNT] = {
                "EV MODE: MERCEDES-BENZ MBUX HYPERSCREEN EQ",
                "EV MODE: BMW i-CURVED DISPLAY (iDrive 8.5/9 M-EV)",
                "EV MODE: AUDI VIRTUAL COCKPIT PLUS (e-tron GT RS)",
                "EV MODE: VOLVO EX90 / POLESTAR SCANDINAVIAN EV"
            };
            draw_text_centered(r, ev_titles[ui->ev_style], SCREEN_W / 2, 65, 2, rgba(10, 14, 20, 255));
        } else {
            draw_text_centered(r, THEME_FULL_NAMES[ui->theme], SCREEN_W / 2, 65, 2, rgba(10, 14, 20, 255));
        }
    }

    if (ui->show_ip_modal) {
        render_ip_modal(r, client, ui);
    }

    SDL_RenderPresent(r);
}

bool hud_handle_touch_down(hud_ui_state_t *ui, gt7_client_t *client, gt7_state_t *state, int x, int y, uint32_t now_ms) {
    ui->touch_start_x = x;
    ui->touch_start_y = y;
    ui->touch_tracking = true;

    /* Reset auto-hide timer on any touch */
    if (ui->fullscreen_canvas) {
        ui->dock_hide_at_ms = now_ms + 4500;
    }

    if (ui->show_ip_modal) {
        int mw = 600, mh = 570;
        int mx = (SCREEN_W - mw) / 2;
        int my = (SCREEN_H - mh) / 2;
        int kw = 156, kh = 50, gap = 10;
        int gx = mx + (mw - (kw * 3 + gap * 2)) / 2;
        int gy = my + 132;
        static const char *keys[12] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", ".", "0", "DEL"};

        for (int i = 0; i < 12; ++i) {
            int row = i / 3;
            int col = i % 3;
            int bx = gx + col * (kw + gap);
            int by = gy + row * (kh + gap);
            if (x >= bx && x <= bx + kw && y >= by && y <= by + kh) {
                if (strcasecmp(ui->edit_ip, "AUTO") == 0) {
                    ui->edit_ip[0] = '\0';
                    ui->ip_cursor = 0;
                }
                if (i == 11) {
                    if (ui->ip_cursor > 0) {
                        ui->edit_ip[--ui->ip_cursor] = '\0';
                    }
                } else if (ui->ip_cursor < 15) {
                    ui->edit_ip[ui->ip_cursor++] = keys[i][0];
                    ui->edit_ip[ui->ip_cursor] = '\0';
                }
                return true;
            }
        }

        /* Dedicated Heartbeat Protocol Selector Row */
        int proto_y = gy + 4 * (kh + gap) + 8;
        int pw = 124, ph = 36, pgap = 10;
        int px_start = mx + (mw - (pw * 4 + pgap * 3)) / 2;
        int proto_btn_y = proto_y + 16;
        if (y >= proto_btn_y && y <= proto_btn_y + ph) {
            for (int p = 0; p < 4; ++p) {
                int px = px_start + p * (pw + pgap);
                if (x >= px && x <= px + pw) {
                    gt7_set_heartbeat_mode(client, (gt7_heartbeat_mode_t)p);
                    return true;
                }
            }
        }

        int by = proto_btn_y + ph + 16;
        if (x >= gx && x <= gx + 180 && y >= by && y <= by + 52) {
            /* AUTO DETECT (DEFAULT) */
            gt7_set_auto_detect(client, state);
            strncpy(ui->edit_ip, "AUTO", sizeof(ui->edit_ip) - 1);
            ui->ip_cursor = (int)strlen(ui->edit_ip);
            ui->show_ip_modal = false;
            return true;
        }
        if (x >= gx + 192 && x <= gx + 352 && y >= by && y <= by + 52) {
            /* MANUAL IP */
            if (ui->ip_cursor >= 7 && strcasecmp(ui->edit_ip, "AUTO") != 0) {
                gt7_set_ip(client, ui->edit_ip);
            } else {
                gt7_set_auto_detect(client, state);
            }
            ui->show_ip_modal = false;
            return true;
        }
        if (x >= gx + 364 && x <= gx + 488 && y >= by && y <= by + 52) {
            /* CANCEL */
            snprintf(ui->edit_ip, sizeof(ui->edit_ip), "%s", client->ps5_ip);
            ui->ip_cursor = (int)strlen(ui->edit_ip);
            ui->show_ip_modal = false;
            return true;
        }
        return true;
    }

    bool show_bars = !ui->fullscreen_canvas || ui->dock_visible;

    /* If in pure Fullscreen mode with bars hidden, tapping bottom-left exits fullscreen immediately, bottom-right pops up dock! */
    if (!show_bars) {
        if (x <= 170 && y >= SCREEN_H - 50) {
            ui->fullscreen_canvas = false;
            ui->dock_visible = true;
            return true;
        }
        if ((x >= SCREEN_W - 160 && y >= SCREEN_H - 50) || y < 50 || y > SCREEN_H - 50) {
            ui->dock_visible = true;
            ui->dock_hide_at_ms = now_ms + 4500;
            return true;
        }
    }

    if (show_bars) {
        /* Top Bar Touch Actions: [PROTOCOL / IP MODAL] [RESET LAPS] [FULLSCREEN] [EXIT APP] */
        if (y <= 42) {
            if (x >= 220 && x <= 750) {
                if (client->auto_detect && strcmp(client->ps5_ip, "AUTO") == 0) {
                    snprintf(ui->edit_ip, sizeof(ui->edit_ip), "%s.", client->subnet_prefix);
                } else {
                    snprintf(ui->edit_ip, sizeof(ui->edit_ip), "%s", client->ps5_ip);
                }
                ui->ip_cursor = (int)strlen(ui->edit_ip);
                ui->show_ip_modal = true;
                return true;
            }
            if (x >= 835 && x <= 970) {
                gt7_reset_session(state);
                return true;
            }
            if (x >= 980 && x <= 1145) {
                ui->fullscreen_canvas = !ui->fullscreen_canvas;
                ui->dock_visible = !ui->fullscreen_canvas;
                return true;
            }
            if (x >= 1155 && x <= 1270) {
                ui->request_quit = true;
                return true;
            }
        }

        /* Bottom Dock Touch Actions */
        if (y >= 668) {
            for (int i = 0; i < HUD_THEME_COUNT; ++i) {
                int bx = 8 + i * 89;
                if (x >= bx && x <= bx + 86) {
                    ui->theme = (hud_theme_t)i;
                    ui->theme_banner_until_ms = now_ms + 1800;
                    return true;
                }
            }
            if (x >= 730 && x <= 858) {
                if (client->auto_detect && strcmp(client->ps5_ip, "AUTO") == 0) {
                    snprintf(ui->edit_ip, sizeof(ui->edit_ip), "%s.", client->subnet_prefix);
                } else {
                    snprintf(ui->edit_ip, sizeof(ui->edit_ip), "%s", client->ps5_ip);
                }
                ui->ip_cursor = (int)strlen(ui->edit_ip);
                ui->show_ip_modal = true;
                return true;
            }
            if (x >= 866 && x <= 994) {
                state->demo_mode = !state->demo_mode;
                return true;
            }
            if (x >= 1002 && x <= 1122) {
                ui->use_mph = !ui->use_mph;
                return true;
            }
            if (x >= 1130 && x <= 1272) {
                ui->fullscreen_canvas = !ui->fullscreen_canvas;
                ui->dock_visible = !ui->fullscreen_canvas;
                return true;
            }
        }
    }

    /* In Luxury EV Cockpit (Mode 3), top 4-brand tab bar touch handling */
    if (ui->theme == HUD_THEME_EV_COCKPIT && y >= (show_bars ? 46 : 10) && y <= (show_bars ? 86 : 50)) {
        if (x >= 24 && x <= 326) {
            ui->ev_style = EV_STYLE_BENZ_EQ;
            ui->theme_banner_until_ms = now_ms + 1800;
            return true;
        } else if (x >= 334 && x <= 636) {
            ui->ev_style = EV_STYLE_BMW_I;
            ui->theme_banner_until_ms = now_ms + 1800;
            return true;
        } else if (x >= 644 && x <= 946) {
            ui->ev_style = EV_STYLE_AUDI_ETRON;
            ui->theme_banner_until_ms = now_ms + 1800;
            return true;
        } else if (x >= 954 && x <= 1256) {
            ui->ev_style = EV_STYLE_VOLVO_EX;
            ui->theme_banner_until_ms = now_ms + 1800;
            return true;
        }
    }

    /* In Luxury EV Cockpit, tapping center map toggles track circuit */
    if (ui->theme == HUD_THEME_EV_COCKPIT && x >= 380 && x <= 900 && y >= 110 && y <= 560) {
        gt7_cycle_demo_circuit(state);
        return true;
    }

    /* In Official GT7 Mode (Mode 1), tapping the Bottom-Right MFD card cycles MFD pages or adjusts settings! */
    if (ui->theme == HUD_THEME_GT7 && x >= 912 && x <= 1262 && y >= 165 && y <= 660) {
        if (y <= 245) {
            state->mfd_page = (state->mfd_page + 1) % 4;
        } else {
            int p = state->mfd_page % 4;
            if (p == 0) {
                gt7_cycle_demo_circuit(state);
            } else if (p == 1) {
                state->gt7_fuel_map = (state->gt7_fuel_map % 6) + 1;
            } else if (p == 2) {
                state->gt7_brake_bal++;
                if (state->gt7_brake_bal > 5) state->gt7_brake_bal = -5;
            } else {
                state->gt7_tcs_level = (state->gt7_tcs_level + 1) % 6;
            }
        }
        return true;
    }

    /* In Track Info Mode (Mode 8), tapping inside the GPS Circuit Map cycles preset circuits! */
    if (ui->theme == HUD_THEME_TRACK && x >= 16 && x <= 601 && y >= 130 && y <= 655) {
        gt7_cycle_demo_circuit(state);
        return true;
    }

    return false;
}

bool hud_handle_touch_up(hud_ui_state_t *ui, int x, int y, uint32_t now_ms) {
    if (!ui->touch_tracking) return false;
    ui->touch_tracking = false;
    if (ui->show_ip_modal) return false;

    int dx = x - ui->touch_start_x;
    int dy = y - ui->touch_start_y;

    /* Horizontal Swipe Gesture (> 90px) anywhere on screen cycles dashboard modes! */
    if (abs(dx) > 90 && abs(dx) > abs(dy) * 2) {
        hud_cycle_theme(ui, (dx < 0) ? +1 : -1, now_ms);
        return true;
    }

    /* Tapping left/right edge zones cycles themes; tapping center in Fullscreen toggles the touch dock */
    if (abs(dx) < 25 && abs(dy) < 25 && y > 50 && y < 660) {
        if (x < 95) {
            hud_cycle_theme(ui, -1, now_ms);
            return true;
        } else if (x > SCREEN_W - 95) {
            hud_cycle_theme(ui, +1, now_ms);
            return true;
        } else if (ui->fullscreen_canvas) {
            ui->dock_visible = !ui->dock_visible;
            if (ui->dock_visible) {
                ui->dock_hide_at_ms = now_ms + 4500;
            }
            return true;
        }
    }
    return false;
}
