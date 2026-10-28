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
        draw_text_centered(r, tbuf, tx + box_w / 2, ty + box_h / 2 - 6, 3, rgba(10, 12, 16, 255));

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
    char short_circuit[32];
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
        draw_text_centered(r, t_lbl[i], tx + 26, ty + 10, 2, rgba(10, 12, 18, 255));
        snprintf(pbuf, sizeof(pbuf), "%.0fC", state->tire_temp[i]);
        draw_text_centered(r, pbuf, tx + 26, ty + 38, 2, rgba(10, 12, 18, 255));
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

    /* Fuel & G-Force Summary Strip */
    int fuel_strip_y = brk_y + 52;
    fill_rect(r, center_x + 24, fuel_strip_y, pedal_w, body_h - (fuel_strip_y - body_y) - 12, ddu_slot);
    draw_text(r, "FUEL", center_x + 36, fuel_strip_y + 10, 2, text_dim);
    snprintf(buf, sizeof(buf), "%.1f%%  (%.1f LAPS)", state->fuel_pct, (state->fuel_burn_per_lap > 0.1f) ? (state->fuel_pct / state->fuel_burn_per_lap) : (state->fuel_pct / 3.0f));
    draw_text(r, buf, center_x + 95, fuel_strip_y + 10, 2, (state->fuel_pct < 15.0f) ? neon_red : neon_green);

    snprintf(buf, sizeof(buf), "G-LOAD: %.2fG  (LAT: %+.2f  LON: %+.2f)",
             sqrtf(state->g_lat * state->g_lat + state->g_lon * state->g_lon),
             state->g_lat, state->g_lon);
    draw_text(r, buf, center_x + 36, fuel_strip_y + 34, 2, text_bright);

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
        snprintf(buf, sizeof(buf), "%.0f C", state->tire_temp[t]);
        draw_text_centered(r, buf, tx + tcard_w / 2, ty + 38, 3, tcol);
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

/* 1. MERCEDES-BENZ MBUX HYPERSCREEN EQ COCKPIT */
static void render_ev_benz_eq(SDL_Renderer *r, const gt7_state_t *state, const hud_ui_state_t *ui, uint32_t now_ms, int content_y, int content_h) {
    (void)now_ms;
    rgba_t eq_cyan   = rgba(0, 225, 255, 255);
    rgba_t eq_green  = rgba(0, 245, 140, 255);
    rgba_t text_dim  = rgba(140, 155, 175, 255);
    rgba_t text_wh   = rgba(245, 250, 255, 255);

    draw_thick_line(r, 60, content_y + 2, 1220, content_y + 2, 2, rgba(0, 225, 255, 120));

    float speed = ui->use_mph ? state->speed_mph : state->speed_kmh;
    float max_spd = ui->use_mph ? 200.0f : 320.0f;
    float spd_frac = speed / max_spd;
    if (spd_frac > 1.0f) spd_frac = 1.0f;

    float power_pct = state->throttle_pct;
    float charge_pct = state->brake_pct;
    float power_kw = power_pct * 4.6f;
    float regen_kw = charge_pct * 2.2f;

    float batt_pct = state->fuel_pct;
    float est_range = batt_pct * 4.2f;
    float kwh_lap = (state->fuel_burn_per_lap > 0.1f) ? (state->fuel_burn_per_lap * 0.85f) : 2.4f;

    int cy = content_y + content_h / 2 + 6;
    int dial_rad = (content_h > 580) ? 146 : 130;

    /* LEFT DIAL: Mercedes EQ Speedometer & PRND */
    int left_cx = 215;
    fill_circle(r, left_cx, cy, dial_rad, rgba(0, 0, 0, 255));
    draw_circle(r, left_cx, cy, dial_rad, 2, rgba(30, 48, 70, 255));
    draw_circle(r, left_cx, cy, dial_rad - 18, 1, rgba(18, 30, 45, 255));
    draw_segmented_arc(r, left_cx, cy, dial_rad - 6, 10, 150.0f, 390.0f, 32, spd_frac, eq_cyan, rgba(0, 0, 0, 0));

    int ispd = (int)roundf(speed);
    draw_7seg_number_centered(r, ispd, 3, false, left_cx, cy - 36, 42, 62, 8, 7, text_wh, rgba(0, 0, 0, 0));
    draw_text_centered(r, ui->use_mph ? "MPH" : "KM/H", left_cx, cy + 34, 2, eq_cyan);

    int prnd_y = cy + dial_rad - 40;
    static const char *prnd[4] = {"P", "R", "N", "D"};
    int cur_gear = state->current_gear;
    int active_idx = 3;
    if (state->speed_kmh < 1.0f && (state->flags & GT7_FLAG_HANDBRAKE)) active_idx = 0;
    else if (cur_gear == 0) active_idx = 1;
    else if (cur_gear < 0 || cur_gear > 9) active_idx = 2;
    else active_idx = 3;

    for (int p = 0; p < 4; ++p) {
        int px = left_cx - 54 + p * 28;
        bool act = (p == active_idx);
        if (act) {
            fill_rect(r, px - 2, prnd_y - 2, 24, 22, eq_cyan);
            draw_text_centered(r, prnd[p], px + 10, prnd_y + 2, 2, rgba(0, 0, 0, 255));
        } else {
            draw_text_centered(r, prnd[p], px + 10, prnd_y + 2, 2, text_dim);
        }
    }

    fill_rect(r, left_cx - 56, cy - dial_rad + 20, 112, 22, rgba(0, 0, 0, 255));
    draw_rect(r, left_cx - 56, cy - dial_rad + 20, 112, 22, 1, eq_cyan);
    draw_text_centered(r, "EQ SPORT", left_cx, cy - dial_rad + 25, 2, eq_cyan);

    /* RIGHT DIAL: Mercedes EQ Power & Recuperation Meter */
    int right_cx = SCREEN_W - 215;
    fill_circle(r, right_cx, cy, dial_rad, rgba(0, 0, 0, 255));
    draw_circle(r, right_cx, cy, dial_rad, 2, rgba(30, 48, 70, 255));
    draw_circle(r, right_cx, cy, dial_rad - 18, 1, rgba(18, 30, 45, 255));
    draw_segmented_arc(r, right_cx, cy, dial_rad - 6, 10, 180.0f, 360.0f, 20, power_pct / 100.0f, eq_cyan, rgba(0, 0, 0, 0));
    draw_segmented_arc(r, right_cx, cy, dial_rad - 6, 10, 180.0f, 90.0f, 14, charge_pct / 100.0f, eq_green, rgba(0, 0, 0, 0));

    char pwr_buf[48];
    if (charge_pct > 3.0f) {
        snprintf(pwr_buf, sizeof(pwr_buf), "-%.0f kW", regen_kw);
        draw_text_centered(r, pwr_buf, right_cx, cy - 20, 3, eq_green);
        draw_text_centered(r, "RECUPERATION", right_cx, cy + 18, 2, eq_green);
    } else {
        snprintf(pwr_buf, sizeof(pwr_buf), "+%.0f kW", power_kw);
        draw_text_centered(r, pwr_buf, right_cx, cy - 20, 3, eq_cyan);
        draw_text_centered(r, "EQ POWER", right_cx, cy + 18, 2, (power_pct > 80.0f) ? rgba(215, 60, 255, 255) : eq_cyan);
    }

    int batt_bar_y = cy + dial_rad - 40;
    fill_rect(r, right_cx - 60, batt_bar_y, 120, 16, rgba(0, 0, 0, 255));
    fill_rect(r, right_cx - 60, batt_bar_y, (int)(1.2f * batt_pct), 16, (batt_pct < 20.0f) ? rgba(255, 50, 60, 255) : eq_cyan);
    draw_rect(r, right_cx - 60, batt_bar_y, 120, 16, 1, rgba(60, 85, 120, 255));
    snprintf(pwr_buf, sizeof(pwr_buf), "SoC %.0f%%", batt_pct);
    draw_text_centered(r, pwr_buf, right_cx, batt_bar_y + 2, 2, text_wh);

    /* CENTER AREA: Hyperscreen Panoramic Navigation Display */
    int center_x = left_cx + dial_rad + 18;
    int center_w = (right_cx - dial_rad - 18) - center_x;
    int center_y = content_y + 8;
    int center_h = content_h - 16;

    draw_panel(r, center_x, center_y, center_w, center_h, rgba(0, 0, 0, 255), eq_cyan, 14);
    draw_text_centered(r, "- MERCEDES-BENZ MBUX HYPERSCREEN -", center_x + center_w / 2, center_y + 10, 2, eq_cyan);

    /* Clean Lap Time & Delta Header Row above Map */
    char buf[64];
    format_lap_time(state->current_lap_ms, buf, sizeof(buf));
    char lap_str[64];
    snprintf(lap_str, sizeof(lap_str), "LAP: %s", buf);
    draw_text(r, lap_str, center_x + 18, center_y + 30, 2, text_wh);

    char d_buf[32];
    snprintf(d_buf, sizeof(d_buf), "DELTA %s%.3f", (state->delta_ms <= 0) ? "-" : "+", fabsf((float)state->delta_ms / 1000.0f));
    draw_text_right(r, d_buf, center_x + center_w - 18, center_y + 30, 2, (state->delta_ms <= 0) ? eq_green : rgba(255, 55, 65, 255));

    int map_pad = 16;
    int map_w = center_w - map_pad * 2;
    int map_h = center_h - 175;
    int map_x = center_x + map_pad;
    int map_y = center_y + 52;
    fill_rect(r, map_x, map_y, map_w, map_h, rgba(0, 0, 0, 255));
    draw_rect(r, map_x, map_y, map_w, map_h, 1, rgba(35, 50, 75, 255));
    draw_track_map_geometry(r, state, map_x + 6, map_y + 6, map_w - 12, map_h - 12, 4, 2, false);

    int card_y = map_y + map_h + 8;
    int card_w = (center_w - 36) / 2;
    int card_h = center_h - (card_y - center_y) - 8;

    draw_panel(r, center_x + 12, card_y, card_w, card_h, rgba(0, 0, 0, 255), rgba(40, 60, 90, 255), 8);
    draw_text(r, "RANGE & EFFICIENCY", center_x + 22, card_y + 8, 2, eq_cyan);
    snprintf(buf, sizeof(buf), "%.0f KM", est_range);
    draw_text(r, buf, center_x + 22, card_y + 30, 3, text_wh);
    snprintf(buf, sizeof(buf), "AVG: %.1f kWh/lap", kwh_lap);
    draw_text(r, buf, center_x + 22, card_y + 62, 2, text_dim);

    int rcx = center_x + 16 + card_w;
    draw_panel(r, rcx, card_y, card_w, card_h, rgba(0, 0, 0, 255), rgba(40, 60, 90, 255), 8);
    draw_text(r, "4MATIC+ MOTOR & TIRES", rcx + 14, card_y + 8, 2, eq_cyan);
    snprintf(buf, sizeof(buf), "FL %.0fC   FR %.0fC", state->tire_temp[0], state->tire_temp[1]);
    draw_text(r, buf, rcx + 14, card_y + 32, 2, tire_temp_color(state->tire_temp[0]));
    snprintf(buf, sizeof(buf), "RL %.0fC   RR %.0fC", state->tire_temp[2], state->tire_temp[3]);
    draw_text(r, buf, rcx + 14, card_y + 58, 2, tire_temp_color(state->tire_temp[2]));
}

/* 2. BMW i-CURVED DISPLAY / iDRIVE 8.5/9 M-EV */
static void render_ev_bmw_i(SDL_Renderer *r, const gt7_state_t *state, const hud_ui_state_t *ui, uint32_t now_ms, int content_y, int content_h) {
    rgba_t bmw_blue  = rgba(0, 160, 255, 255);
    rgba_t bmw_cyan  = rgba(0, 235, 255, 255);
    rgba_t bmw_red   = rgba(255, 45, 60, 255);
    rgba_t bmw_green = rgba(0, 230, 130, 255);
    rgba_t text_dim  = rgba(140, 155, 175, 255);
    rgba_t text_wh   = rgba(245, 250, 255, 255);

    float speed = ui->use_mph ? state->speed_mph : state->speed_kmh;
    float max_spd = ui->use_mph ? 200.0f : 320.0f;
    float spd_frac = speed / max_spd;
    if (spd_frac > 1.0f) spd_frac = 1.0f;

    float power_pct = state->throttle_pct;
    float charge_pct = state->brake_pct;
    float power_kw = power_pct * 4.8f;
    float regen_kw = charge_pct * 2.4f;
    float batt_pct = state->fuel_pct;
    float est_range = batt_pct * 4.0f;

    bool eboost = (power_pct >= 90.0f);
    bool blink = ((now_ms / 70) % 2) == 0;

    int body_y = content_y + 8;
    int body_h = content_h - 16;
    int cy = body_y + body_h / 2;

    /* LEFT WING: BMW Angular Geometric Speed Bracket */
    int left_w = 330;
    int left_x = 20;
    draw_panel(r, left_x, body_y, left_w, body_h, rgba(0, 0, 0, 255), bmw_blue, 12);

    /* Geometric Angular Bracket Track */
    draw_thick_line(r, left_x + 18, body_y + body_h - 26, left_x + 18, body_y + 80, 4, rgba(35, 50, 75, 255));
    draw_thick_line(r, left_x + 18, body_y + 80, left_x + 180, body_y + 26, 4, rgba(35, 50, 75, 255));

    int track_tot_h = (body_h - 106);
    int lit_h = (int)(track_tot_h * spd_frac);
    if (lit_h > 0) {
        int v_part = lit_h;
        if (v_part > (body_h - 106)) v_part = (body_h - 106);
        draw_thick_line(r, left_x + 18, body_y + body_h - 26, left_x + 18, body_y + body_h - 26 - v_part, 5, bmw_cyan);
    }

    draw_text(r, "///M ELECTRIC", left_x + 26, body_y + 14, 2, bmw_blue);

    int left_cx = left_x + left_w / 2;
    int ispd = (int)roundf(speed);
    draw_7seg_number_centered(r, ispd, 3, false, left_cx, cy - 36, 44, 70, 9, 8, text_wh, rgba(0, 0, 0, 0));
    draw_text_centered(r, ui->use_mph ? "MPH" : "KM/H", left_cx, cy + 42, 2, bmw_cyan);

    fill_rect(r, left_cx - 65, cy + 76, 130, 28, rgba(0, 0, 0, 255));
    draw_rect(r, left_cx - 65, cy + 76, 130, 28, 2, bmw_blue);
    draw_text_centered(r, eboost ? "SPORT BOOST" : "SPORT", left_cx, cy + 82, 2, eboost ? bmw_cyan : text_wh);

    draw_text_centered(r, "ASSIST: ACTIVE", left_cx, body_y + body_h - 26, 2, text_dim);

    /* RIGHT WING: BMW ePOWER & eBOOST Polygon Meter */
    int right_x = SCREEN_W - 20 - left_w;
    draw_panel(r, right_x, body_y, left_w, body_h, rgba(0, 0, 0, 255), bmw_blue, 12);

    /* Inverted Geometric Angular Bracket Track on Right */
    draw_thick_line(r, right_x + left_w - 18, body_y + body_h - 26, right_x + left_w - 18, body_y + 80, 4, rgba(35, 50, 75, 255));
    draw_thick_line(r, right_x + left_w - 18, body_y + 80, right_x + left_w - 180, body_y + 26, 4, rgba(35, 50, 75, 255));

    int lit_r = (int)(track_tot_h * (power_pct / 100.0f));
    if (lit_r > 0) {
        draw_thick_line(r, right_x + left_w - 18, body_y + body_h - 26, right_x + left_w - 18, body_y + body_h - 26 - lit_r, 5,
                        eboost ? (blink ? bmw_cyan : bmw_red) : bmw_blue);
    }

    draw_text_right(r, "BMW i-DRIVE 9.0", right_x + left_w - 26, body_y + 14, 2, bmw_blue);

    int right_cx = right_x + left_w / 2;
    char bbuf[128];

    if (eboost) {
        fill_rect(r, right_x + 28, cy - 80, left_w - 56, 32, blink ? bmw_cyan : bmw_red);
        draw_text_centered(r, ">> eBOOST ACTIVE <<", right_cx, cy - 72, 2, rgba(0, 0, 0, 255));
    }

    if (charge_pct > 3.0f) {
        snprintf(bbuf, sizeof(bbuf), "-%.0f kW", regen_kw);
        draw_text_centered(r, bbuf, right_cx, cy - 20, 3, bmw_green);
        draw_text_centered(r, "CHARGE / REGEN", right_cx, cy + 22, 2, bmw_green);
    } else {
        snprintf(bbuf, sizeof(bbuf), "+%.0f kW", power_kw);
        draw_text_centered(r, bbuf, right_cx, cy - 20, 3, eboost ? bmw_cyan : bmw_blue);
        snprintf(bbuf, sizeof(bbuf), "%.0f%% ePOWER", power_pct);
        draw_text_centered(r, bbuf, right_cx, cy + 22, 2, text_wh);
    }

    int batt_y = body_y + body_h - 48;
    fill_rect(r, right_x + 30, batt_y, left_w - 60, 22, rgba(0, 0, 0, 255));
    fill_rect(r, right_x + 30, batt_y, (int)((left_w - 60) * (batt_pct / 100.0f)), 22, bmw_blue);
    draw_rect(r, right_x + 30, batt_y, left_w - 60, 22, 1, rgba(60, 90, 130, 255));
    snprintf(bbuf, sizeof(bbuf), "BATTERY %.0f%% (%.0f KM)", batt_pct, est_range);
    draw_text_centered(r, bbuf, right_cx, batt_y + 4, 2, text_wh);

    /* CENTER AREA: BMW i-Curved Navigation & Telemetry */
    int center_x = left_x + left_w + 14;
    int center_w = right_x - 14 - center_x;
    draw_panel(r, center_x, body_y, center_w, body_h, rgba(0, 0, 0, 255), bmw_blue, 14);

    draw_text(r, "BMW LIVE COCKPIT PROFESSIONAL", center_x + 20, body_y + 12, 2, bmw_cyan);
    snprintf(bbuf, sizeof(bbuf), "DELTA %s%.3f", (state->delta_ms <= 0) ? "-" : "+", fabsf((float)state->delta_ms / 1000.0f));
    draw_text_right(r, bbuf, center_x + center_w - 20, body_y + 12, 2, (state->delta_ms <= 0) ? bmw_green : bmw_red);

    int map_pad = 16;
    int map_w = center_w - map_pad * 2;
    int map_h = body_h - 165;
    int map_x = center_x + map_pad;
    int map_y = body_y + 36;
    fill_rect(r, map_x, map_y, map_w, map_h, rgba(0, 0, 0, 255));
    draw_rect(r, map_x, map_y, map_w, map_h, 1, rgba(35, 55, 85, 255));
    draw_track_map_geometry(r, state, map_x + 6, map_y + 6, map_w - 12, map_h - 12, 4, 2, false);

    int bot_card_y = map_y + map_h + 10;
    draw_panel(r, center_x + 14, bot_card_y, center_w - 28, 96, rgba(0, 0, 0, 255), rgba(45, 68, 100, 255), 8);

    snprintf(bbuf, sizeof(bbuf), "G-FORCE: %.2f G   (LAT: %+.2f  LON: %+.2f)",
             sqrtf(state->g_lat * state->g_lat + state->g_lon * state->g_lon), state->g_lat, state->g_lon);
    draw_text(r, bbuf, center_x + 26, bot_card_y + 12, 2, text_wh);

    snprintf(bbuf, sizeof(bbuf), "TIRES: FL %.0fC | FR %.0fC | RL %.0fC | RR %.0fC",
             state->tire_temp[0], state->tire_temp[1], state->tire_temp[2], state->tire_temp[3]);
    draw_text(r, bbuf, center_x + 26, bot_card_y + 38, 2, bmw_cyan);

    snprintf(bbuf, sizeof(bbuf), "DSC: ON  |  M eDRIVE: 100%%  |  LAP %d/%d",
             state->current_lap > 0 ? state->current_lap : 1, state->total_laps > 0 ? state->total_laps : 15);
    draw_text(r, bbuf, center_x + 26, bot_card_y + 64, 2, text_dim);
}

/* 3. AUDI VIRTUAL COCKPIT PLUS e-tron GT */
static void render_ev_audi_etron(SDL_Renderer *r, const gt7_state_t *state, const hud_ui_state_t *ui, uint32_t now_ms, int content_y, int content_h) {
    (void)now_ms;
    rgba_t audi_red   = rgba(235, 30, 45, 255);
    rgba_t audi_cyan  = rgba(0, 220, 245, 255);
    rgba_t audi_green = rgba(0, 235, 120, 255);
    rgba_t text_dim   = rgba(140, 155, 175, 255);
    rgba_t text_wh    = rgba(245, 250, 255, 255);

    float speed = ui->use_mph ? state->speed_mph : state->speed_kmh;
    float max_spd = ui->use_mph ? 200.0f : 320.0f;
    float spd_frac = speed / max_spd;
    if (spd_frac > 1.0f) spd_frac = 1.0f;

    float power_pct = state->throttle_pct;
    float charge_pct = state->brake_pct;
    float power_kw = power_pct * 4.4f;
    float regen_kw = charge_pct * 2.2f;

    int body_y = content_y + 8;
    int body_h = content_h - 16;
    int cy = body_y + body_h / 2 + 10;
    int dial_rad = (body_h > 580) ? 142 : 128;

    /* LEFT DIAL: Audi Virtual Cockpit Speedometer */
    int left_cx = 210;
    fill_circle(r, left_cx, cy, dial_rad, rgba(0, 0, 0, 255));
    draw_circle(r, left_cx, cy, dial_rad, 2, rgba(45, 55, 75, 255));
    draw_segmented_arc(r, left_cx, cy, dial_rad - 6, 8, 140.0f, 400.0f, 28, spd_frac, audi_red, rgba(0, 0, 0, 0));

    int ispd = (int)roundf(speed);
    draw_7seg_number_centered(r, ispd, 3, false, left_cx, cy - 36, 42, 64, 8, 7, text_wh, rgba(0, 0, 0, 0));
    draw_text_centered(r, ui->use_mph ? "MPH" : "KM/H", left_cx, cy + 34, 2, audi_red);

    draw_text_centered(r, "RS e-tron GT", left_cx, cy - dial_rad + 20, 2, text_wh);
    draw_text_centered(r, "DYNAMIC", left_cx, cy + dial_rad - 30, 2, text_dim);

    /* RIGHT DIAL: Audi e-tron Power Meter */
    int right_cx = SCREEN_W - 210;
    fill_circle(r, right_cx, cy, dial_rad, rgba(0, 0, 0, 255));
    draw_circle(r, right_cx, cy, dial_rad, 2, rgba(45, 55, 75, 255));
    draw_segmented_arc(r, right_cx, cy, dial_rad - 6, 8, 180.0f, 360.0f, 20, power_pct / 100.0f, audi_cyan, rgba(0, 0, 0, 0));
    draw_segmented_arc(r, right_cx, cy, dial_rad - 6, 8, 180.0f, 100.0f, 12, charge_pct / 100.0f, audi_green, rgba(0, 0, 0, 0));

    char abuf[128];
    if (charge_pct > 3.0f) {
        snprintf(abuf, sizeof(abuf), "-%.0f kW", regen_kw);
        draw_text_centered(r, abuf, right_cx, cy - 20, 3, audi_green);
        draw_text_centered(r, "CHARGE", right_cx, cy + 18, 2, audi_green);
    } else {
        snprintf(abuf, sizeof(abuf), "+%.0f kW", power_kw);
        draw_text_centered(r, abuf, right_cx, cy - 20, 3, audi_cyan);
        draw_text_centered(r, (power_pct > 90.0f) ? "RS BOOST" : "POWER %", right_cx, cy + 18, 2, audi_red);
    }
    snprintf(abuf, sizeof(abuf), "BATTERY %.0f%%", state->fuel_pct);
    draw_text_centered(r, abuf, right_cx, cy + dial_rad - 30, 2, text_wh);

    /* CENTER AREA: Audi MMI Virtual Cockpit Navigation */
    int center_x = left_cx + dial_rad + 18;
    int center_w = (right_cx - dial_rad - 18) - center_x;
    draw_panel(r, center_x, body_y, center_w, body_h, rgba(0, 0, 0, 255), audi_red, 14);

    draw_text(r, "AUDI VIRTUAL COCKPIT PLUS", center_x + 18, body_y + 12, 2, text_wh);
    snprintf(abuf, sizeof(abuf), "DELTA %s%.3f", (state->delta_ms <= 0) ? "-" : "+", fabsf((float)state->delta_ms / 1000.0f));
    draw_text_right(r, abuf, center_x + center_w - 18, body_y + 12, 2, (state->delta_ms <= 0) ? audi_green : audi_red);

    int map_pad = 16;
    int map_w = center_w - map_pad * 2;
    int map_h = body_h - 165;
    int map_x = center_x + map_pad;
    int map_y = body_y + 36;
    fill_rect(r, map_x, map_y, map_w, map_h, rgba(0, 0, 0, 255));
    draw_rect(r, map_x, map_y, map_w, map_h, 1, rgba(40, 50, 70, 255));
    draw_track_map_geometry(r, state, map_x + 6, map_y + 6, map_w - 12, map_h - 12, 4, 2, false);

    int bot_y = map_y + map_h + 10;
    draw_panel(r, center_x + 14, bot_y, center_w - 28, 96, rgba(0, 0, 0, 255), rgba(50, 65, 88, 255), 8);

    char s1_s[16], s2_s[16], s3_s[16];
    format_sector_time(state->cur_sector_ms[0], s1_s, sizeof(s1_s));
    format_sector_time(state->cur_sector_ms[1], s2_s, sizeof(s2_s));
    format_sector_time(state->cur_sector_ms[2], s3_s, sizeof(s3_s));
    snprintf(abuf, sizeof(abuf), "SECTORS: S1 %s  |  S2 %s  |  S3 %s", s1_s, s2_s, s3_s);
    draw_text(r, abuf, center_x + 26, bot_y + 16, 2, text_wh);

    snprintf(abuf, sizeof(abuf), "RS e-tron GT  |  EFFICIENCY: 20.8 kWh/100km  |  G-LOAD: %.2f",
             sqrtf(state->g_lat * state->g_lat + state->g_lon * state->g_lon));
    draw_text(r, abuf, center_x + 26, bot_y + 44, 2, text_dim);
}

/* 4. VOLVO EX90 / POLESTAR SCANDINAVIAN EV COCKPIT */
static void render_ev_volvo_ex(SDL_Renderer *r, const gt7_state_t *state, const hud_ui_state_t *ui, uint32_t now_ms, int content_y, int content_h) {
    (void)now_ms;
    rgba_t scand_ice  = rgba(180, 225, 255, 255);
    rgba_t scand_mint = rgba(50, 230, 150, 255);
    rgba_t text_dim   = rgba(135, 150, 170, 255);
    rgba_t text_wh    = rgba(250, 252, 255, 255);

    float speed = ui->use_mph ? state->speed_mph : state->speed_kmh;
    float power_pct = state->throttle_pct;
    float charge_pct = state->brake_pct;
    float batt_pct = state->fuel_pct;
    float est_range = batt_pct * 4.4f;

    int body_y = content_y + 6;
    int body_h = content_h - 12;

    int center_w = 640;
    int center_x = (SCREEN_W - center_w) / 2;
    draw_panel(r, center_x, body_y, center_w, body_h, rgba(0, 0, 0, 255), rgba(40, 55, 75, 255), 16);

    int ispd = (int)roundf(speed);
    draw_7seg_number_centered(r, ispd, 3, false, center_x + center_w / 2, body_y + 18, 48, 76, 9, 8, text_wh, rgba(0, 0, 0, 0));
    draw_text_centered(r, ui->use_mph ? "MPH  [ D ]" : "KM/H  [ D ]", center_x + center_w / 2, body_y + 102, 2, scand_ice);

    int map_pad = 20;
    int map_w = center_w - map_pad * 2;
    int map_h = body_h - 185;
    int map_x = center_x + map_pad;
    int map_y = body_y + 128;
    fill_rect(r, map_x, map_y, map_w, map_h, rgba(0, 0, 0, 255));
    draw_rect(r, map_x, map_y, map_w, map_h, 1, rgba(30, 42, 58, 255));
    draw_track_map_geometry(r, state, map_x + 8, map_y + 8, map_w - 16, map_h - 16, 4, 2, false);

    char vbuf[128];
    snprintf(vbuf, sizeof(vbuf), "DELTA: %s%.3f  |  LAP %d", (state->delta_ms <= 0) ? "-" : "+", fabsf((float)state->delta_ms / 1000.0f), state->current_lap > 0 ? state->current_lap : 1);
    draw_text_centered(r, vbuf, center_x + center_w / 2, body_y + body_h - 32, 2, (state->delta_ms <= 0) ? scand_mint : rgba(255, 60, 70, 255));

    int left_x = 20;
    int side_w = center_x - 34;
    draw_panel(r, left_x, body_y, side_w, body_h, rgba(0, 0, 0, 255), rgba(35, 48, 65, 255), 14);

    draw_text(r, "VOLVO EX90 / POLESTAR", left_x + 18, body_y + 16, 2, scand_ice);
    draw_text(r, "PILOT ASSIST: ACTIVE", left_x + 18, body_y + 50, 2, scand_mint);

    draw_text(r, "ELECTRIC RANGE", left_x + 18, body_y + 96, 2, text_dim);
    snprintf(vbuf, sizeof(vbuf), "%.0f KM", est_range);
    draw_text(r, vbuf, left_x + 18, body_y + 120, 3, text_wh);

    draw_text(r, "CONSUMPTION", left_x + 18, body_y + 175, 2, text_dim);
    draw_text(r, "17.4 kWh/100km", left_x + 18, body_y + 198, 2, text_wh);

    draw_text(r, "TEMPERATURE", left_x + 18, body_y + 245, 2, text_dim);
    draw_text(r, "CABIN 21C | EXT 19C", left_x + 18, body_y + 268, 2, text_wh);

    draw_text(r, "TIRE CONDITIONS", left_x + 18, body_y + 320, 2, text_dim);
    snprintf(vbuf, sizeof(vbuf), "FL %.0fC   FR %.0fC", state->tire_temp[0], state->tire_temp[1]);
    draw_text(r, vbuf, left_x + 18, body_y + 346, 2, scand_ice);
    snprintf(vbuf, sizeof(vbuf), "RL %.0fC   RR %.0fC", state->tire_temp[2], state->tire_temp[3]);
    draw_text(r, vbuf, left_x + 18, body_y + 372, 2, scand_ice);

    int right_x = center_x + center_w + 14;
    draw_panel(r, right_x, body_y, side_w, body_h, rgba(0, 0, 0, 255), rgba(35, 48, 65, 255), 14);

    draw_text(r, "ENERGY & BATTERY", right_x + 18, body_y + 16, 2, scand_ice);

    int bar_x = right_x + 28;
    int bar_y = body_y + 56;
    int bar_w = 36;
    int bar_h = body_h - 170;
    int mid_y = bar_y + bar_h / 2;

    fill_rect(r, bar_x, bar_y, bar_w, bar_h, rgba(0, 0, 0, 255));
    draw_rect(r, bar_x, bar_y, bar_w, bar_h, 1, rgba(35, 50, 75, 255));
    SDL_RenderDrawLine(r, bar_x - 6, mid_y, bar_x + bar_w + 6, mid_y);

    int p_fill = (int)((bar_h / 2) * (power_pct / 100.0f));
    fill_rect(r, bar_x + 2, mid_y - p_fill, bar_w - 4, p_fill, scand_ice);

    int r_fill = (int)((bar_h / 2) * (charge_pct / 100.0f));
    fill_rect(r, bar_x + 2, mid_y, bar_w - 4, r_fill, scand_mint);

    draw_text(r, "PWR", bar_x + bar_w + 14, bar_y + 8, 2, scand_ice);
    draw_text(r, "REGEN", bar_x + bar_w + 14, bar_y + bar_h - 24, 2, scand_mint);

    snprintf(vbuf, sizeof(vbuf), "+%.0f kW", power_pct * 4.4f);
    draw_text(r, vbuf, bar_x + bar_w + 14, mid_y - 24, 2, text_wh);
    snprintf(vbuf, sizeof(vbuf), "-%.0f kW", charge_pct * 2.2f);
    draw_text(r, vbuf, bar_x + bar_w + 14, mid_y + 8, 2, scand_mint);

    int batt_y = body_y + body_h - 60;
    snprintf(vbuf, sizeof(vbuf), "BATTERY %.0f%% (%.0f KM)", batt_pct, est_range);
    draw_text_centered(r, vbuf, right_x + side_w / 2, batt_y - 20, 2, scand_ice);
    fill_rect(r, right_x + 20, batt_y, side_w - 40, 28, rgba(0, 0, 0, 255));
    fill_rect(r, right_x + 20, batt_y, (int)((side_w - 40) * (batt_pct / 100.0f)), 28, scand_mint);
    draw_rect(r, right_x + 20, batt_y, side_w - 40, 28, 1, rgba(50, 75, 105, 255));
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
    draw_text_centered(r, rpm_str, 640, content_y + 18, 4, amber);

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
    int r_top_h = (int)(content_h * 0.56f);
    int r_bot_h = content_h - r_top_h - 12;
    draw_panel(r, 912, content_y, 352, r_top_h, rgba(0, 0, 0, 255), amber, 12);
    draw_text(r, "LAP", 932, content_y + 14, 2, amber);
    char lap_str[64];
    snprintf(lap_str, sizeof(lap_str), "%d / %d", state->current_lap > 0 ? state->current_lap : 1, state->total_laps > 0 ? state->total_laps : 15);
    draw_text(r, lap_str, 932, content_y + 38, 4, rgba(255, 255, 255, 255));

    draw_text_right(r, "POS", 1244, content_y + 14, 2, amber);
    snprintf(lap_str, sizeof(lap_str), "P%d", state->race_position > 0 ? state->race_position : 1);
    draw_text_right(r, lap_str, 1244, content_y + 38, 4, cyan);

    draw_text(r, "FUEL REMAINING", 932, content_y + 98, 2, rgba(170, 185, 205, 255));
    snprintf(lap_str, sizeof(lap_str), "%.1f%%", state->fuel_pct);
    draw_text_right(r, lap_str, 1244, content_y + 92, 3, (state->fuel_pct < 15.0f) ? rgba(255, 60, 60, 255) : rgba(0, 240, 120, 255));
    fill_rect(r, 932, content_y + 122, 312, 18, rgba(30, 36, 48, 255));
    fill_rect(r, 932, content_y + 122, (int)(3.12f * state->fuel_pct), 18, (state->fuel_pct < 15.0f) ? rgba(255, 60, 60, 255) : rgba(0, 230, 115, 255));

    float est_laps = (state->fuel_burn_per_lap > 0.1f) ? (state->fuel_pct / state->fuel_burn_per_lap) : (state->fuel_pct / 3.2f);
    snprintf(lap_str, sizeof(lap_str), "EST. +%.1f LAPS  (%.1f%%/L)", est_laps, state->fuel_burn_per_lap);
    draw_text(r, lap_str, 932, content_y + 150, 2, amber);

    snprintf(lap_str, sizeof(lap_str), "BOOST: %+.2f BAR (PK %+.2f)", state->turbo_bar, state->peak_turbo_bar);
    draw_text(r, lap_str, 932, content_y + 188, 2, cyan);
    snprintf(lap_str, sizeof(lap_str), "WATER: %.0fC   OIL: %.0fC", state->water_temp, state->oil_temp);
    draw_text(r, lap_str, 932, content_y + 218, 2, rgba(190, 205, 225, 255));
    snprintf(lap_str, sizeof(lap_str), "OIL PRESS: %.1f BAR  TOP: %.0f", state->oil_pressure, state->session_top_speed_kmh);
    draw_text(r, lap_str, 932, content_y + 248, 2, rgba(190, 205, 225, 255));

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

    /* Right Panel */
    draw_panel(r, 888, content_y, 376, half_h, rgba(0, 0, 0, 255), gold, 10);
    draw_text(r, "ENDURANCE FUEL & VITALS", 906, content_y + 14, 2, gold);

    snprintf(tbuf, sizeof(tbuf), "%.1f L", state->fuel_level);
    draw_text(r, "FUEL LEVEL", 906, content_y + 48, 2, rgba(160, 170, 190, 255));
    draw_text_right(r, tbuf, 1244, content_y + 42, 3, green);

    float laps_left = (state->fuel_burn_per_lap > 0.1f) ? (state->fuel_pct / state->fuel_burn_per_lap) : (state->fuel_pct / 3.2f);
    snprintf(tbuf, sizeof(tbuf), "%.1f LAPS", laps_left);
    draw_text(r, "LAPS REM", 906, content_y + 88, 2, rgba(160, 170, 190, 255));
    draw_text_right(r, tbuf, 1244, content_y + 82, 3, gold);

    snprintf(tbuf, sizeof(tbuf), "%+.2f BAR", state->turbo_bar);
    draw_text(r, "MAP BOOST", 906, content_y + 128, 2, rgba(160, 170, 190, 255));
    draw_text_right(r, tbuf, 1244, content_y + 122, 3, rgba(0, 225, 255, 255));

    snprintf(tbuf, sizeof(tbuf), "WATER %.0fC  |  OIL %.0fC (%.1fB)", state->water_temp, state->oil_temp, state->oil_pressure);
    draw_text(r, tbuf, 906, content_y + 166, 2, white);

    bool tcs = (state->flags & GT7_FLAG_TCS_ACTIVE) != 0;
    bool asm_f = (state->flags & GT7_FLAG_ASM_ACTIVE) != 0;
    fill_rect(r, 906, content_y + half_h - 46, 162, 32, tcs ? gold : rgba(30, 35, 45, 255));
    draw_text_centered(r, "TC ACTIVE", 987, content_y + half_h - 37, 2, tcs ? rgba(10, 12, 16, 255) : rgba(100, 110, 130, 255));
    fill_rect(r, 1082, content_y + half_h - 46, 162, 32, asm_f ? rgba(255, 55, 55, 255) : rgba(30, 35, 45, 255));
    draw_text_centered(r, "ABS / ASM", 1163, content_y + half_h - 37, 2, asm_f ? white : rgba(100, 110, 130, 255));

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
    draw_text_centered(r, bbuf, bcx, bcy + 40, 3, white);

    draw_tires_widget(r, state, 16, content_y + top_box_h + 12, 368, bot_box_h, orange);

    /* Center Giant Rally Gear Drop Zone */
    draw_panel(r, 396, content_y, 488, content_h, rgba(0, 0, 0, 255), yellow, 16);
    fill_rect(r, 414, content_y + 14, 452, 42, orange);
    draw_text_centered(r, "WRC STAGE TELEMETRY", 640, content_y + 26, 3, rgba(12, 14, 16, 255));

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

    snprintf(bbuf, sizeof(bbuf), "RPM: %.0f / %u", state->engine_rpm, state->rpm_limiter);
    draw_text_centered(r, bbuf, 640, content_y + content_h - 38, 3, yellow);

    /* Right Panel: Chassis Pitch/Roll Inclinometer */
    draw_panel(r, 896, content_y, 368, top_box_h, rgba(0, 0, 0, 255), orange, 12);
    draw_text(r, "CHASSIS ROLL / PITCH & RIDE", 914, content_y + 12, 2, orange);

    int icx = 1080, icy = content_y + top_box_h / 2 - 4, irad = 64;
    draw_circle(r, icx, icy, irad, 2, rgba(80, 90, 110, 255));
    float roll_rad = state->roll * (float)M_PI / 180.0f;
    int rx1 = icx - (int)(cosf(roll_rad) * (irad - 8));
    int ry1 = icy - (int)(sinf(roll_rad) * (irad - 8));
    int rx2 = icx + (int)(cosf(roll_rad) * (irad - 8));
    int ry2 = icy + (int)(sinf(roll_rad) * (irad - 8));
    draw_thick_line(r, rx1, ry1, rx2, ry2, 4, yellow);

    snprintf(bbuf, sizeof(bbuf), "ROLL: %+.1f DEG   PITCH: %+.1f DEG", state->roll, state->pitch);
    draw_text_centered(r, bbuf, 1080, content_y + top_box_h - 56, 2, white);
    snprintf(bbuf, sizeof(bbuf), "RIDE HEIGHT: %.0f MM", state->body_height);
    draw_text_centered(r, bbuf, 1080, content_y + top_box_h - 28, 2, orange);

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
    draw_text_centered(r, buf, bcx, bcy + 48, 3, white);

    draw_tires_widget(r, state, 16, top_y + top_box_h + 12, 368, bot_box_h, neon_blue);

    /* Center Masterpiece: Large Circular Sweeping JDM Tachometer */
    draw_panel(r, 396, top_y, 488, content_h, rgba(0, 0, 0, 255), neon_blue, 18);
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

        if (major) {
            char kstr[16];
            snprintf(kstr, sizeof(kstr), "%d", i / 5);
            int tx = tcx + (int)(cosf(ang) * (trad - 54));
            int ty = tcy + (int)(sinf(ang) * (trad - 54)) - 8;
            draw_text_centered(r, kstr, tx, ty, 3, redzone ? neon_red : neon_blue);
        }
    }

    draw_7seg_char(r, gear_to_char(state->current_gear), tcx - 38, tcy - 108, 76, 115, 14,
                   (state->flags & GT7_FLAG_REV_BLINK) ? neon_red : white, rgba(0, 0, 0, 0));

    int spd = (int)roundf(ui->use_mph ? state->speed_mph : state->speed_kmh);
    draw_7seg_number_centered(r, spd, 3, false, tcx, tcy + 50, 44, 72, 9, 8, neon_blue, rgba(0, 0, 0, 0));
    draw_text_centered(r, ui->use_mph ? "MPH" : "KM/H", tcx, tcy + 134, 2, white);

    float rpm_norm = state->engine_rpm / (float)(max_k * 1000);
    if (rpm_norm < 0.0f) rpm_norm = 0.0f;
    if (rpm_norm > 1.0f) rpm_norm = 1.0f;
    float r_ang = (135.0f + rpm_norm * 270.0f) * (float)M_PI / 180.0f;
    draw_thick_line(r, tcx - (int)(cosf(r_ang) * 28), tcy - (int)(sinf(r_ang) * 28),
                    tcx + (int)(cosf(r_ang) * (trad - 20)), tcy + (int)(sinf(r_ang) * (trad - 20)), 6, neon_red);
    fill_circle(r, tcx, tcy, 18, rgba(30, 35, 45, 255));
    draw_circle(r, tcx, tcy, 18, 3, neon_red);

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
    draw_text(r, "LAST LAP", 916, top_y + 238, 2, neon_blue);
    draw_text_right(r, buf, 1244, top_y + 232, 3, rgba(200, 210, 230, 255));

    snprintf(buf, sizeof(buf), "FUEL:%.0f%%  THR:%.0f%%  BRK:%.0f%%", state->fuel_pct, state->throttle_pct, state->brake_pct);
    draw_text(r, buf, 916, top_y + top_box_h - 32, 2, neon_blue);

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

    snprintf(cbuf, sizeof(cbuf), "TOP SPEED: %.1f KM/H   FUEL: %.1f%%", state->session_top_speed_kmh, state->fuel_pct);
    draw_text(r, cbuf, 815, top_y + 44, 2, emerald);

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
    snprintf(cbuf, sizeof(cbuf), "PIT STRATEGY: %.1f LAPS FUEL REM  ->  BOX ON LAP %d  |  TIRE AVG: %.0fC",
             est_laps, pit_lap,
             (state->tire_temp[0] + state->tire_temp[1] + state->tire_temp[2] + state->tire_temp[3]) * 0.25f);
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
    int mw = 580, mh = 550;
    int mx = (SCREEN_W - mw) / 2;
    int my = (SCREEN_H - mh) / 2;

    draw_panel(r, mx, my, mw, mh, rgba(0, 0, 0, 255), rgba(0, 225, 255, 255), 16);
    draw_text_centered(r, "PS5 / PS5 PRO IP CONNECTION", SCREEN_W / 2, my + 20, 3, rgba(0, 225, 255, 255));

    char sub_info[256];
    snprintf(sub_info, sizeof(sub_info), "LOCAL IP: %s   |   AUTO-SCAN: %s.1~254", client->local_ip, client->subnet_prefix);
    draw_text_centered(r, sub_info, SCREEN_W / 2, my + 54, 2, rgba(160, 185, 215, 255));

    fill_rect(r, mx + 40, my + 82, mw - 80, 56, rgba(0, 0, 0, 255));
    draw_rect(r, mx + 40, my + 82, mw - 80, 56, 2, rgba(0, 225, 255, 255));
    draw_text_centered(r, ui->edit_ip[0] ? ui->edit_ip : "AUTO-DETECT OR TAP IP...", SCREEN_W / 2, my + 98, 4, rgba(255, 255, 255, 255));

    static const char *keys[12] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", ".", "0", "DEL"};
    int kw = 152, kh = 60, gap = 14;
    int gx = mx + (mw - (kw * 3 + gap * 2)) / 2;
    int gy = my + 154;

    for (int i = 0; i < 12; ++i) {
        int row = i / 3;
        int col = i % 3;
        int bx = gx + col * (kw + gap);
        int by = gy + row * (kh + gap);
        fill_rect(r, bx, by, kw, kh, rgba(32, 42, 62, 255));
        draw_rect(r, bx, by, kw, kh, 2, rgba(80, 105, 145, 255));
        draw_text_centered(r, keys[i], bx + kw / 2, by + 19, 3, rgba(245, 250, 255, 255));
    }

    /* 3 Action Buttons: [AUTO DETECT (DEFAULT)], [SET MANUAL IP], [CANCEL] */
    int by = gy + 4 * (kh + gap) + 6;
    fill_rect(r, gx, by, 180, 58, rgba(0, 215, 255, 255));
    draw_text_centered(r, "AUTO DETECT", gx + 90, by + 20, 2, rgba(10, 14, 20, 255));

    fill_rect(r, gx + 192, by, 160, 58, rgba(0, 210, 100, 255));
    draw_text_centered(r, "MANUAL IP", gx + 272, by + 20, 2, rgba(10, 14, 20, 255));

    fill_rect(r, gx + 364, by, 120, 58, rgba(215, 50, 60, 255));
    draw_text_centered(r, "CANCEL", gx + 424, by + 20, 2, rgba(255, 255, 255, 255));
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
        draw_text(r, "GT7 SIM-DDU " GT7_APP_VERSION, 18, 12, 2, rgba(240, 245, 255, 255));

        char status_str[128];
        rgba_t status_col;
        if (state->connected) {
            snprintf(status_str, sizeof(status_str), "LIVE PS5 (%s %s) 60HZ",
                     client->ps5_ip, client->auto_detect ? "AUTO" : "MANUAL");
            status_col = rgba(0, 240, 110, 255);
        } else if (client->auto_detect) {
            snprintf(status_str, sizeof(status_str), "AUTO-DETECT PS5 (%s.*) %s",
                     client->subnet_prefix, state->demo_mode ? "| DEMO" : "");
            status_col = rgba(0, 220, 255, 255);
        } else {
            snprintf(status_str, sizeof(status_str), "MANUAL PS5 (%s) %s",
                     client->ps5_ip, state->demo_mode ? "| DEMO" : "");
            status_col = rgba(255, 195, 0, 255);
        }
        fill_circle(r, 410, 19, 6, status_col);
        draw_text(r, status_str, 425, 12, 2, status_col);

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
        int mw = 580, mh = 550;
        int mx = (SCREEN_W - mw) / 2;
        int my = (SCREEN_H - mh) / 2;
        int kw = 152, kh = 60, gap = 14;
        int gx = mx + (mw - (kw * 3 + gap * 2)) / 2;
        int gy = my + 154;
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

        int by = gy + 4 * (kh + gap) + 6;
        if (x >= gx && x <= gx + 180 && y >= by && y <= by + 58) {
            /* AUTO DETECT (DEFAULT) */
            gt7_set_auto_detect(client, state);
            strncpy(ui->edit_ip, "AUTO", sizeof(ui->edit_ip) - 1);
            ui->ip_cursor = (int)strlen(ui->edit_ip);
            ui->show_ip_modal = false;
            return true;
        }
        if (x >= gx + 192 && x <= gx + 352 && y >= by && y <= by + 58) {
            /* MANUAL IP */
            if (ui->ip_cursor >= 7 && strcasecmp(ui->edit_ip, "AUTO") != 0) {
                gt7_set_ip(client, ui->edit_ip);
            } else {
                gt7_set_auto_detect(client, state);
            }
            ui->show_ip_modal = false;
            return true;
        }
        if (x >= gx + 364 && x <= gx + 484 && y >= by && y <= by + 58) {
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
        /* Top Bar Touch Actions: [RESET LAPS] [FULLSCREEN] [EXIT APP] */
        if (y <= 42) {
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
