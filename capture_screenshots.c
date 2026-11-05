#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <SDL.h>
#include "gt7_telemetry.h"
#include "hud_renderer.h"

static void save_frame(SDL_Renderer *r, const char *bmp_path) {
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, SCREEN_W, SCREEN_H, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!surface) {
        fprintf(stderr, "Failed to create surface: %s\n", SDL_GetError());
        return;
    }
    if (SDL_RenderReadPixels(r, NULL, SDL_PIXELFORMAT_ARGB8888, surface->pixels, surface->pitch) != 0) {
        fprintf(stderr, "SDL_RenderReadPixels failed: %s\n", SDL_GetError());
        SDL_FreeSurface(surface);
        return;
    }
    SDL_SaveBMP(surface, bmp_path);
    SDL_FreeSurface(surface);
}

int main() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("GT7 HUD Capture", 0, 0, SCREEN_W, SCREEN_H, SDL_WINDOW_HIDDEN);
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    gt7_client_t client;
    gt7_state_t state;
    hud_ui_state_t ui;

    gt7_init(&client, &state, "192.168.1.100");
    hud_init(&ui, "192.168.1.100");
    state.demo_mode = true;
    state.connected = true;

    system("mkdir -p raw_screens");

    struct {
        const char *name;
        hud_theme_t theme;
        ev_style_t ev_style;
        bmw_view_t bmw_view;
        bool fullscreen;
        bool show_modal;
        uint32_t sim_time_ms;
    } captures[] = {
        {"01_mode_gt7_official",       HUD_THEME_GT7,        EV_STYLE_BENZ_EQ,    BMW_VIEW_M_DYNAMIC,    false, false, 15000},
        {"02_mode_digitach",           HUD_THEME_DIGITACH,   EV_STYLE_BENZ_EQ,    BMW_VIEW_M_DYNAMIC,    false, false, 18000},
        {"03_mode_ev_benz_eq",         HUD_THEME_EV_COCKPIT, EV_STYLE_BENZ_EQ,    BMW_VIEW_M_DYNAMIC,    false, false, 21000},
        {"04_mode_ev_bmw_i",           HUD_THEME_EV_COCKPIT, EV_STYLE_BMW_I,      BMW_VIEW_M_DYNAMIC,    false, false, 24000},
        {"04a_mode_ev_bmw_m_dynamic",  HUD_THEME_EV_COCKPIT, EV_STYLE_BMW_I,      BMW_VIEW_M_DYNAMIC,    false, false, 24000},
        {"04b_mode_ev_bmw_nav_map",    HUD_THEME_EV_COCKPIT, EV_STYLE_BMW_I,      BMW_VIEW_MAP_COCKPIT,  false, false, 24000},
        {"04c_mode_ev_bmw_assist_radar", HUD_THEME_EV_COCKPIT, EV_STYLE_BMW_I,    BMW_VIEW_ASSIST_RADAR, false, false, 24000},
        {"04d_mode_ev_bmw_minimal_map", HUD_THEME_EV_COCKPIT, EV_STYLE_BMW_I,    BMW_VIEW_MINIMAL_MAP,  false, false, 24000},
        {"05_mode_ev_audi_etron",      HUD_THEME_EV_COCKPIT, EV_STYLE_AUDI_ETRON, BMW_VIEW_M_DYNAMIC,    false, false, 27000},
        {"06_mode_ev_volvo_ex",        HUD_THEME_EV_COCKPIT, EV_STYLE_VOLVO_EX,   BMW_VIEW_M_DYNAMIC,    false, false, 30000},
        {"07_mode_f1_wheel",           HUD_THEME_F1,         EV_STYLE_BENZ_EQ,    BMW_VIEW_M_DYNAMIC,    false, false, 33000},
        {"08_mode_gt3_motec",          HUD_THEME_GT3,        EV_STYLE_BENZ_EQ,    BMW_VIEW_M_DYNAMIC,    false, false, 36000},
        {"09_mode_rally_wrc",          HUD_THEME_RALLY,      EV_STYLE_BENZ_EQ,    BMW_VIEW_M_DYNAMIC,    false, false, 39000},
        {"10_mode_jdm_defi",           HUD_THEME_JDM,        EV_STYLE_BENZ_EQ,    BMW_VIEW_M_DYNAMIC,    false, false, 42000},
        {"11_mode_track_radar",        HUD_THEME_TRACK,      EV_STYLE_BENZ_EQ,    BMW_VIEW_M_DYNAMIC,    false, false, 45000},
        {"12_ip_touch_modal",          HUD_THEME_GT7,        EV_STYLE_BENZ_EQ,    BMW_VIEW_M_DYNAMIC,    false, true,  15000},
        {"13_fullscreen_canvas",       HUD_THEME_EV_COCKPIT, EV_STYLE_BENZ_EQ,    BMW_VIEW_M_DYNAMIC,    true,  false, 21000}
    };

    int num_captures = sizeof(captures) / sizeof(captures[0]);
    for (int i = 0; i < num_captures; ++i) {
        ui.theme = captures[i].theme;
        ui.ev_style = captures[i].ev_style;
        ui.bmw_view = captures[i].bmw_view;
        ui.fullscreen_canvas = captures[i].fullscreen;
        ui.dock_visible = !captures[i].fullscreen;
        ui.show_ip_modal = captures[i].show_modal;
        ui.theme_banner_until_ms = 0;

        gt7_update_demo(&state, captures[i].sim_time_ms, (int)ui.theme);
        hud_render(renderer, &state, &client, &ui, captures[i].sim_time_ms);

        char bmp_path[128];
        snprintf(bmp_path, sizeof(bmp_path), "raw_screens/%s.bmp", captures[i].name);
        save_frame(renderer, bmp_path);
        printf("Captured: %s\n", bmp_path);
    }

    /* Video recording generation: record 90 frames (~3 seconds at 30fps) of EV Benz EQ mode */
    system("mkdir -p raw_screens/video_frames");
    ui.theme = HUD_THEME_EV_COCKPIT;
    ui.ev_style = EV_STYLE_BENZ_EQ;
    ui.fullscreen_canvas = false;
    ui.dock_visible = true;
    ui.show_ip_modal = false;
    ui.theme_banner_until_ms = 0;

    for (int f = 0; f < 90; ++f) {
        uint32_t t_ms = 20000 + f * 33;
        gt7_update_demo(&state, t_ms, (int)ui.theme);
        hud_render(renderer, &state, &client, &ui, t_ms);

        char frame_path[128];
        snprintf(frame_path, sizeof(frame_path), "raw_screens/video_frames/frame_%03d.bmp", f);
        save_frame(renderer, frame_path);
    }
    printf("Captured 90 animation frames for video recording.\n");

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
