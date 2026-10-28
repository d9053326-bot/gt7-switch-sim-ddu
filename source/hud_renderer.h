#ifndef HUD_RENDERER_H
#define HUD_RENDERER_H

#include <SDL.h>
#include <stdbool.h>
#include "gt7_telemetry.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SCREEN_W 1280
#define SCREEN_H 720

typedef enum {
    EV_STYLE_BENZ_EQ    = 0,  /* Mercedes-Benz MBUX Hyperscreen EQ Cockpit */
    EV_STYLE_BMW_I      = 1,  /* BMW i-Curved Display / iDrive 8.5/9 M-EV */
    EV_STYLE_AUDI_ETRON = 2,  /* Audi Virtual Cockpit Plus e-tron GT */
    EV_STYLE_VOLVO_EX   = 3,  /* Volvo EX90 / Polestar Minimalist Cockpit */
    EV_STYLE_COUNT      = 4
} ev_style_t;

typedef enum {
    HUD_THEME_GT7        = 0,  /* Official Gran Turismo 7 Native HUD & MFD Style (Default) */
    HUD_THEME_DIGITACH   = 1,  /* High-Res Digital Tachometer & Telemetry Dashboard */
    HUD_THEME_EV_COCKPIT = 2,  /* Modern Luxury EV Digital Cockpit (Benz EQ / BMW i / Audi e-tron / Volvo) */
    HUD_THEME_F1         = 3,  /* Bosch/Cosworth F1 GP Wheel Display */
    HUD_THEME_GT3        = 4,  /* MoTeC C1212 GT3 Endurance Pro DDU */
    HUD_THEME_RALLY      = 5,  /* WRC Rally Stage Cockpit Display */
    HUD_THEME_JDM        = 6,  /* Defi / GReddy JDM Tuned Multi-Gauge */
    HUD_THEME_TRACK      = 7,  /* Track Radar & Race Engineer Pit Wall Mode */
    HUD_THEME_COUNT      = 8
} hud_theme_t;

typedef struct {
    hud_theme_t theme;
    ev_style_t ev_style;     /* Sub-style for HUD_THEME_EV_COCKPIT */
    bool fullscreen_canvas;  /* True = 100% full-screen DDU (auto-hides top/bottom bars) */
    bool dock_visible;       /* True = Top/Bottom touch bars shown */
    uint32_t dock_hide_at_ms;/* Auto-hide timer when in fullscreen mode */
    bool use_mph;
    bool show_ip_modal;
    bool request_quit;       /* Touch EXIT button sets this to true */
    char edit_ip[64];
    int ip_cursor;
    uint32_t theme_banner_until_ms;
    int touch_start_x;
    int touch_start_y;
    bool touch_tracking;
} hud_ui_state_t;

void hud_init(hud_ui_state_t *ui, const char *initial_ip);
void hud_cycle_theme(hud_ui_state_t *ui, int dir, uint32_t now_ms);
void hud_cycle_ev_style(hud_ui_state_t *ui, int dir, uint32_t now_ms);
void hud_render(SDL_Renderer *r, const gt7_state_t *state, const gt7_client_t *client, hud_ui_state_t *ui, uint32_t now_ms);
bool hud_handle_touch_down(hud_ui_state_t *ui, gt7_client_t *client, gt7_state_t *state, int x, int y, uint32_t now_ms);
bool hud_handle_touch_up(hud_ui_state_t *ui, int x, int y, uint32_t now_ms);

#ifdef __cplusplus
}
#endif

#endif /* HUD_RENDERER_H */
