#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <SDL.h>

#ifdef __SWITCH__
#include <switch.h>
#include <sys/stat.h>
#endif

#include "gt7_telemetry.h"
#include "hud_renderer.h"

#ifdef __SWITCH__
#define CONFIG_PATH "sdmc:/switch/gt7_hud/config.ini"
#else
#define CONFIG_PATH "gt7_config.ini"
#endif

static void load_saved_ip(char *ip_out, size_t max_len) {
    /* Default to AUTO-DETECT so user does not need to type PS5 IP */
    strncpy(ip_out, "AUTO", max_len - 1);
    ip_out[max_len - 1] = '\0';

    FILE *f = fopen(CONFIG_PATH, "r");
    if (f) {
        char line[128];
        if (fgets(line, sizeof(line), f)) {
            line[strcspn(line, "\r\n ")] = '\0';
            if (strcasecmp(line, "AUTO") == 0 || strlen(line) >= 7) {
                snprintf(ip_out, max_len, "%s", line);
            }
        }
        fclose(f);
    }
}

static void save_ip_config(const gt7_client_t *client) {
    FILE *f = fopen(CONFIG_PATH, "w");
    if (f) {
        fprintf(f, "%s\n", client->auto_detect ? "AUTO" : client->ps5_ip);
        fclose(f);
    }
}

static void sync_window_fullscreen(SDL_Window *window, bool fullscreen) {
#ifndef __SWITCH__
    SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
#else
    (void)window;
    (void)fullscreen;
#endif
}

int main(int argc, char *argv[]) {
#ifdef __SWITCH__
    socketInitializeDefault();
    mkdir("sdmc:/switch/gt7_hud", 0777); /* ensure config dir exists for saved IP/AUTO setting */
    /* Prevent Nintendo Switch OLED/V1 screen from auto-dimming or sleeping while mounted on wheel */
    appletSetMediaPlaybackState(true);
#endif

    char target_ip[64];
    load_saved_ip(target_ip, sizeof(target_ip));
    if (argc > 1 && strlen(argv[1]) >= 7) {
        strncpy(target_ip, argv[1], sizeof(target_ip) - 1);
        target_ip[sizeof(target_ip) - 1] = '\0';
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK | SDL_INIT_TIMER) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
#ifdef __SWITCH__
        socketExit();
#endif
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "GT7 Switch Telemetry HUD (PS5 / PS5 Pro)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_W, SCREEN_H,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );

    if (!window) {
        SDL_Quit();
#ifdef __SWITCH__
        socketExit();
#endif
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(
        window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (!renderer) {
        renderer = SDL_CreateRenderer(window, -1, 0);
    }
    SDL_RenderSetLogicalSize(renderer, SCREEN_W, SCREEN_H);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            SDL_GameControllerOpen(i);
        } else {
            SDL_JoystickOpen(i);
        }
    }

    gt7_client_t client;
    gt7_state_t state;
    hud_ui_state_t ui;

    gt7_init(&client, &state, target_ip);
    hud_init(&ui, target_ip);

    bool running = true;
    bool prev_fullscreen = ui.fullscreen_canvas;
    bool prev_auto = client.auto_detect;
    char prev_ip[64];
    strncpy(prev_ip, client.ps5_ip, sizeof(prev_ip));

    while (running && !ui.request_quit) {
#ifdef __SWITCH__
        if (!appletMainLoop()) {
            break;
        }
#endif
        uint32_t now_ms = SDL_GetTicks();
        SDL_Event ev;

        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                running = false;
            } else if (ev.type == SDL_KEYDOWN) {
                switch (ev.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        if (ui.show_ip_modal) {
                            ui.show_ip_modal = false;
                        } else if (ui.fullscreen_canvas) {
                            ui.fullscreen_canvas = false;
                            ui.dock_visible = true;
                        } else {
                            running = false;
                        }
                        break;
                    case SDLK_q:
                        running = false;
                        break;
                    case SDLK_f:
                    case SDLK_F11:
                        ui.fullscreen_canvas = !ui.fullscreen_canvas;
                        ui.dock_visible = !ui.fullscreen_canvas;
                        break;
                    case SDLK_1: ui.theme = HUD_THEME_GT7;        ui.theme_banner_until_ms = now_ms + 1800; break;
                    case SDLK_2: ui.theme = HUD_THEME_DIGITACH;   ui.theme_banner_until_ms = now_ms + 1800; break;
                    case SDLK_3: ui.theme = HUD_THEME_EV_COCKPIT; ui.theme_banner_until_ms = now_ms + 1800; break;
                    case SDLK_4: ui.theme = HUD_THEME_F1;         ui.theme_banner_until_ms = now_ms + 1800; break;
                    case SDLK_5: ui.theme = HUD_THEME_GT3;        ui.theme_banner_until_ms = now_ms + 1800; break;
                    case SDLK_6: ui.theme = HUD_THEME_RALLY;      ui.theme_banner_until_ms = now_ms + 1800; break;
                    case SDLK_7: ui.theme = HUD_THEME_JDM;        ui.theme_banner_until_ms = now_ms + 1800; break;
                    case SDLK_8: ui.theme = HUD_THEME_TRACK;      ui.theme_banner_until_ms = now_ms + 1800; break;
                    case SDLK_e:
                        if (ui.theme == HUD_THEME_EV_COCKPIT) {
                            hud_cycle_ev_style(&ui, +1, now_ms);
                        } else {
                            ui.theme = HUD_THEME_EV_COCKPIT;
                            ui.theme_banner_until_ms = now_ms + 1800;
                        }
                        break;
                    case SDLK_LEFT:
                    case SDLK_l:
                        hud_cycle_theme(&ui, -1, now_ms);
                        break;
                    case SDLK_RIGHT:
                    case SDLK_r:
                    case SDLK_TAB:
                        hud_cycle_theme(&ui, +1, now_ms);
                        break;
                    case SDLK_d:
                    case SDLK_y:
                        state.demo_mode = !state.demo_mode;
                        break;
                    case SDLK_u:
                    case SDLK_a:
                        ui.use_mph = !ui.use_mph;
                        break;
                    case SDLK_i:
                    case SDLK_x:
                        if (client.auto_detect && strcmp(client.ps5_ip, "AUTO") == 0) {
                            snprintf(ui.edit_ip, sizeof(ui.edit_ip), "%s.", client.subnet_prefix);
                        } else {
                            snprintf(ui.edit_ip, sizeof(ui.edit_ip), "%s", client.ps5_ip);
                        }
                        ui.ip_cursor = (int)strlen(ui.edit_ip);
                        ui.show_ip_modal = !ui.show_ip_modal;
                        break;
                    case SDLK_BACKSPACE:
                        gt7_reset_session(&state);
                        break;
                    default:
                        break;
                }
            } else if (ev.type == SDL_CONTROLLERBUTTONDOWN) {
                switch (ev.cbutton.button) {
                    case SDL_CONTROLLER_BUTTON_START: /* '+' on Switch */
                        running = false;
                        break;
                    case SDL_CONTROLLER_BUTTON_BACK:  /* '-' on Switch toggles Fullscreen */
                        ui.fullscreen_canvas = !ui.fullscreen_canvas;
                        ui.dock_visible = !ui.fullscreen_canvas;
                        break;
                    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
                    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                        hud_cycle_theme(&ui, -1, now_ms);
                        break;
                    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
                    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                        hud_cycle_theme(&ui, +1, now_ms);
                        break;
                    case SDL_CONTROLLER_BUTTON_Y:
                        if (ui.theme == HUD_THEME_EV_COCKPIT) {
                            hud_cycle_ev_style(&ui, +1, now_ms);
                        } else {
                            state.demo_mode = !state.demo_mode;
                        }
                        break;
                    case SDL_CONTROLLER_BUTTON_A:
                        ui.use_mph = !ui.use_mph;
                        break;
                    case SDL_CONTROLLER_BUTTON_X:
                        if (client.auto_detect && strcmp(client.ps5_ip, "AUTO") == 0) {
                            snprintf(ui.edit_ip, sizeof(ui.edit_ip), "%s.", client.subnet_prefix);
                        } else {
                            snprintf(ui.edit_ip, sizeof(ui.edit_ip), "%s", client.ps5_ip);
                        }
                        ui.ip_cursor = (int)strlen(ui.edit_ip);
                        ui.show_ip_modal = !ui.show_ip_modal;
                        break;
                    case SDL_CONTROLLER_BUTTON_B:
                        if (ui.show_ip_modal) ui.show_ip_modal = false;
                        else if (ui.fullscreen_canvas) {
                            ui.fullscreen_canvas = false;
                            ui.dock_visible = true;
                        }
                        break;
                    default:
                        break;
                }
            } else if (ev.type == SDL_MOUSEBUTTONDOWN) {
                hud_handle_touch_down(&ui, &client, &state, ev.button.x, ev.button.y, now_ms);
            } else if (ev.type == SDL_MOUSEBUTTONUP) {
                hud_handle_touch_up(&ui, ev.button.x, ev.button.y, now_ms);
            } else if (ev.type == SDL_FINGERDOWN) {
                int tx = (int)(ev.tfinger.x * (float)SCREEN_W);
                int ty = (int)(ev.tfinger.y * (float)SCREEN_H);
                hud_handle_touch_down(&ui, &client, &state, tx, ty, now_ms);
            } else if (ev.type == SDL_FINGERUP) {
                int tx = (int)(ev.tfinger.x * (float)SCREEN_W);
                int ty = (int)(ev.tfinger.y * (float)SCREEN_H);
                hud_handle_touch_up(&ui, tx, ty, now_ms);
            }
        }

        if (ui.fullscreen_canvas != prev_fullscreen) {
            sync_window_fullscreen(window, ui.fullscreen_canvas);
            prev_fullscreen = ui.fullscreen_canvas;
        }

        if (strcmp(prev_ip, client.ps5_ip) != 0 || prev_auto != client.auto_detect) {
            save_ip_config(&client);
            strncpy(prev_ip, client.ps5_ip, sizeof(prev_ip));
            prev_auto = client.auto_detect;
        }

        client.active_theme_hint = (int)ui.theme;
        gt7_update(&client, &state, now_ms);

        if (!state.connected && state.demo_mode) {
            gt7_update_demo(&state, now_ms, (int)ui.theme);
        }

        hud_render(renderer, &state, &client, &ui, now_ms);
    }

    gt7_close(&client);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

#ifdef __SWITCH__
    appletSetMediaPlaybackState(false);
    socketExit();
#endif
    return 0;
}
