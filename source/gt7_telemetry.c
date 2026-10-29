#include "gt7_telemetry.h"
#include "salsa20.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static const char GT7_SALSA_KEY[] = "Simulator Interface Packet GT7 ver 0.0";

static uint32_t read_u32_le(const uint8_t *buf, size_t offset) {
    const uint8_t *p = buf + offset;
    return ((uint32_t)p[0]) |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static int32_t read_i32_le(const uint8_t *buf, size_t offset) {
    return (int32_t)read_u32_le(buf, offset);
}

static uint16_t read_u16_le(const uint8_t *buf, size_t offset) {
    const uint8_t *p = buf + offset;
    return (uint16_t)(((uint16_t)p[0]) | ((uint16_t)p[1] << 8));
}

static int16_t read_i16_le(const uint8_t *buf, size_t offset) {
    return (int16_t)read_u16_le(buf, offset);
}

static float read_f32_le(const uint8_t *buf, size_t offset) {
    uint32_t raw = read_u32_le(buf, offset);
    float val;
    memcpy(&val, &raw, sizeof(float));
    return val;
}

/* Pre-populate realistic circuit maps (Suzuka, Nürburgring GP, Le Mans) & lap history */
static void populate_circuit_by_id(gt7_state_t *state, int circuit_id) {
    state->demo_circuit_id = circuit_id % GT7_CIRCUIT_COUNT;
    state->track_min_x = 1e9f;
    state->track_max_x = -1e9f;
    state->track_min_z = 1e9f;
    state->track_max_z = -1e9f;

    if (state->demo_circuit_id == GT7_CIRCUIT_SUZUKA) {
        strncpy(state->circuit_name, "SUZUKA INTERNATIONAL GP", sizeof(state->circuit_name) - 1);
        state->track_length_km = 5.807f;
    } else if (state->demo_circuit_id == GT7_CIRCUIT_NURBURGRING) {
        strncpy(state->circuit_name, "NURBURGRING GP STRECKE", sizeof(state->circuit_name) - 1);
        state->track_length_km = 5.148f;
    } else {
        strncpy(state->circuit_name, "CIRCUIT DE LA SARTHE (24H)", sizeof(state->circuit_name) - 1);
        state->track_length_km = 13.626f;
    }

    for (int i = 0; i < GT7_TRACK_PTS; ++i) {
        float a = ((float)i / (float)GT7_TRACK_PTS) * 2.0f * (float)M_PI;
        float x = 0.0f, z = 0.0f, curvature = 0.0f;

        if (state->demo_circuit_id == GT7_CIRCUIT_SUZUKA) {
            /* Figure-8 inspired technical layout with S-Curves, Hairpin, Spoon & 130R */
            x = 420.0f * cosf(a) + 145.0f * sinf(2.0f * a) - 55.0f * cosf(3.0f * a);
            z = 240.0f * sinf(2.0f * a) + 95.0f * cosf(a) + 45.0f * sinf(4.0f * a);
            curvature = fabsf(sinf(2.0f * a) + 0.65f * cosf(3.0f * a));
        } else if (state->demo_circuit_id == GT7_CIRCUIT_NURBURGRING) {
            /* Nürburgring GP technical arena + Mercedes-Arena + Schumacher-S + Chicane */
            x = 390.0f * cosf(a) - 130.0f * cosf(2.0f * a) + 85.0f * sinf(3.0f * a);
            z = 265.0f * sinf(a) + 115.0f * sinf(2.0f * a) - 65.0f * cosf(4.0f * a);
            curvature = fabsf(cosf(2.0f * a) + 0.8f * sinf(4.0f * a));
        } else {
            /* Le Mans Mulsanne long straights + chicanes + Porsche Curves */
            x = 480.0f * cosf(a) + 60.0f * cosf(3.0f * a) + 25.0f * sinf(7.0f * a);
            z = 220.0f * sinf(a) - 70.0f * sinf(2.0f * a) + 35.0f * cosf(5.0f * a);
            curvature = fabsf(0.45f * sinf(3.0f * a) + 0.85f * sinf(7.0f * a));
        }

        state->track_pts[i].x = x;
        state->track_pts[i].z = z;
        state->track_pts[i].speed_kmh = 305.0f - curvature * 120.0f;
        if (curvature > 1.02f) state->track_pts[i].pedal_state = 2; /* Heavy Braking */
        else if (curvature > 0.62f) state->track_pts[i].pedal_state = 0; /* Technical Cornering */
        else state->track_pts[i].pedal_state = 1; /* Full Throttle */
        state->track_pts[i].segment_break = 0;

        if (x < state->track_min_x) state->track_min_x = x;
        if (x > state->track_max_x) state->track_max_x = x;
        if (z < state->track_min_z) state->track_min_z = z;
        if (z > state->track_max_z) state->track_max_z = z;
    }
    state->track_pt_count = GT7_TRACK_PTS;
    state->track_pt_head = 0;
    state->track_is_closed = true;
    state->track_live_cleared = false;
}

static void populate_default_circuit_and_laps(gt7_state_t *state) {
    populate_circuit_by_id(state, state->demo_circuit_id);

    /* Populate sample stint lap history */
    static const gt7_lap_record_t sample_laps[4] = {
        {1, 93140, 31420, 39810, 21910, 3.3f, 286.4f},
        {2, 92085, 30980, 39415, 21690, 3.2f, 289.1f},
        {3, 91428, 30712, 39120, 21596, 3.1f, 292.8f},
        {4, 91892, 30840, 39390, 21662, 3.2f, 290.5f}
    };
    for (int i = 0; i < 4; ++i) {
        state->lap_history[i] = sample_laps[i];
    }
    state->lap_history_count = 4;
    state->best_sector_ms[0] = 30712;
    state->best_sector_ms[1] = 39120;
    state->best_sector_ms[2] = 21510;
    state->optimal_lap_ms = state->best_sector_ms[0] + state->best_sector_ms[1] + state->best_sector_ms[2];
    state->cur_sector_ms[0] = 30685;
    state->cur_sector_ms[1] = 39210;
    state->cur_sector_ms[2] = 21615;
    state->session_top_speed_kmh = 292.8f;
    state->peak_turbo_bar = 1.48f;
    state->mfd_page = 0;
    state->gt7_tcs_level = 1;
    state->gt7_brake_bal = -1;
    state->gt7_fuel_map = 1;
}

void gt7_cycle_demo_circuit(gt7_state_t *state) {
    int next = (state->demo_circuit_id + 1) % GT7_CIRCUIT_COUNT;
    populate_circuit_by_id(state, next);
}

void gt7_reset_session(gt7_state_t *state) {
    bool was_demo = state->demo_mode;
    bool was_conn = state->connected;
    memset(state, 0, sizeof(*state));
    state->demo_mode = was_demo;
    state->connected = was_conn;
    populate_default_circuit_and_laps(state);
}

bool gt7_decrypt_packet(const uint8_t *raw, size_t len, uint8_t *decrypted_out) {
    if (len < 0x128) {
        return false;
    }

    uint32_t iv1 = read_u32_le(raw, 0x40);

    /* Format-specific Salsa20 nonce XOR constant:
       - Packet ~ (344B): 0x55FABB4FU (EV / Hybrid telemetry format)
       - Packet A (296B): 0xDEADBEAFU (Legacy baseline telemetry format)
       - Packet B (316B) & Packet C (368B): 0xDEADBEEFU (Dynamic 6-DoF & Motorsport format)
       Multi-layer fallback candidate seeds are prioritized based on packet length.
    */
    uint32_t candidates[3];
    if (len == 344) {
        candidates[0] = 0x55FABB4FU;
        candidates[1] = 0xDEADBEEFU;
        candidates[2] = 0xDEADBEAFU;
    } else if (len == 296) {
        candidates[0] = 0xDEADBEAFU;
        candidates[1] = 0xDEADBEEFU;
        candidates[2] = 0x55FABB4FU;
    } else {
        candidates[0] = 0xDEADBEEFU;
        candidates[1] = 0x55FABB4FU;
        candidates[2] = 0xDEADBEAFU;
    }

    for (int i = 0; i < 3; ++i) {
        uint32_t iv2 = iv1 ^ candidates[i];
        uint8_t nonce[8];
        nonce[0] = (uint8_t)(iv2 & 0xFF);
        nonce[1] = (uint8_t)((iv2 >> 8) & 0xFF);
        nonce[2] = (uint8_t)((iv2 >> 16) & 0xFF);
        nonce[3] = (uint8_t)((iv2 >> 24) & 0xFF);
        nonce[4] = (uint8_t)(iv1 & 0xFF);
        nonce[5] = (uint8_t)((iv1 >> 8) & 0xFF);
        nonce[6] = (uint8_t)((iv1 >> 16) & 0xFF);
        nonce[7] = (uint8_t)((iv1 >> 24) & 0xFF);

        salsa20_ctx_t ctx;
        salsa20_init(&ctx, (const uint8_t *)GT7_SALSA_KEY, nonce);
        salsa20_crypt(&ctx, raw, decrypted_out, len);

        uint32_t magic = read_u32_le(decrypted_out, 0x00);
        if (magic == GT7_MAGIC_G7S0) {
            return true;
        }
    }

    return false;
}

/* Detect local Wi-Fi IP & /24 subnet prefix using a non-transmitting UDP routing probe */
static void detect_local_subnet(gt7_client_t *client) {
    strncpy(client->local_ip, "192.168.1.2", sizeof(client->local_ip) - 1);
    strncpy(client->subnet_prefix, "192.168.1", sizeof(client->subnet_prefix) - 1);

    int probe = socket(AF_INET, SOCK_DGRAM, 0);
    if (probe < 0) return;

    struct sockaddr_in remote;
    memset(&remote, 0, sizeof(remote));
    remote.sin_family = AF_INET;
    remote.sin_port = htons(53);
    inet_pton(AF_INET, "8.8.8.8", &remote.sin_addr);

    if (connect(probe, (struct sockaddr *)&remote, sizeof(remote)) == 0) {
        struct sockaddr_in local;
        socklen_t len = sizeof(local);
        if (getsockname(probe, (struct sockaddr *)&local, &len) == 0) {
            if (inet_ntop(AF_INET, &local.sin_addr, client->local_ip, sizeof(client->local_ip))) {
                strncpy(client->subnet_prefix, client->local_ip, sizeof(client->subnet_prefix) - 1);
                client->subnet_prefix[sizeof(client->subnet_prefix) - 1] = '\0';
                char *last_dot = strrchr(client->subnet_prefix, '.');
                if (last_dot) {
                    *last_dot = '\0';
                }
            }
        }
    }
    close(probe);
}

bool gt7_init(gt7_client_t *client, gt7_state_t *state, const char *ps5_ip) {
    memset(client, 0, sizeof(*client));
    memset(state, 0, sizeof(*state));
    client->sock_fd = -1;
    state->demo_mode = true;
    client->tracked_lap = -1;
    client->scan_cursor = 1;
    client->heartbeat_mode = GT7_HEARTBEAT_AUTO;
    client->active_hb_char = 'C';

    populate_default_circuit_and_laps(state);
    detect_local_subnet(client);

    /* Default to AUTO-DETECT unless a specific manual IP was provided */
    if (!ps5_ip || ps5_ip[0] == '\0' || strcasecmp(ps5_ip, "AUTO") == 0) {
        client->auto_detect = true;
        strncpy(client->ps5_ip, "AUTO", sizeof(client->ps5_ip) - 1);
    } else {
        gt7_set_ip(client, ps5_ip);
    }

    client->sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (client->sock_fd < 0) {
        return false;
    }

    int reuse = 1;
    setsockopt(client->sock_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    int broadcast = 1;
    setsockopt(client->sock_fd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));

    int flags = fcntl(client->sock_fd, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(client->sock_fd, F_SETFL, flags | O_NONBLOCK);
    }

    struct sockaddr_in bind_addr;
    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_port = htons(GT7_RECV_PORT);
    bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(client->sock_fd, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0) {
        /* Socket remains usable for outbound heartbeats */
    }

    return true;
}

void gt7_set_ip(gt7_client_t *client, const char *ps5_ip) {
    if (!ps5_ip || ps5_ip[0] == '\0' || strcasecmp(ps5_ip, "AUTO") == 0) {
        client->auto_detect = true;
        strncpy(client->ps5_ip, "AUTO", sizeof(client->ps5_ip) - 1);
        return;
    }
    client->auto_detect = false;
    strncpy(client->ps5_ip, ps5_ip, sizeof(client->ps5_ip) - 1);
    client->ps5_ip[sizeof(client->ps5_ip) - 1] = '\0';
}

void gt7_set_auto_detect(gt7_client_t *client, gt7_state_t *state) {
    detect_local_subnet(client);
    client->auto_detect = true;
    client->scan_cursor = 1;
    strncpy(client->ps5_ip, "AUTO", sizeof(client->ps5_ip) - 1);
    if (state) {
        state->connected = false;
    }
}

void gt7_set_heartbeat_mode(gt7_client_t *client, gt7_heartbeat_mode_t mode) {
    if (!client) return;
    client->heartbeat_mode = mode % GT7_HEARTBEAT_COUNT;
}

void gt7_cycle_heartbeat_mode(gt7_client_t *client) {
    if (!client) return;
    client->heartbeat_mode = (gt7_heartbeat_mode_t)((client->heartbeat_mode + 1) % GT7_HEARTBEAT_COUNT);
}

const char *gt7_heartbeat_mode_name(gt7_heartbeat_mode_t mode) {
    switch (mode) {
        case GT7_HEARTBEAT_AUTO:      return "AUTO (ADAPTIVE)";
        case GT7_HEARTBEAT_PKT_C:     return "PKT C (368B)";
        case GT7_HEARTBEAT_PKT_TILDE: return "PKT ~ (344B)";
        case GT7_HEARTBEAT_PKT_A:     return "PKT A (296B)";
        case GT7_HEARTBEAT_PKT_B:     return "PKT B (316B)";
        default:                      return "AUTO";
    }
}

static void gt7_send_heartbeat(gt7_client_t *client, bool connected) {
    if (client->sock_fd < 0) return;

    /* Select heartbeat byte based on configured mode or active theme */
    char hb_char = 'C';
    switch (client->heartbeat_mode) {
        case GT7_HEARTBEAT_PKT_C:     hb_char = 'C'; break;
        case GT7_HEARTBEAT_PKT_TILDE: hb_char = '~'; break;
        case GT7_HEARTBEAT_PKT_A:     hb_char = 'A'; break;
        case GT7_HEARTBEAT_PKT_B:     hb_char = 'B'; break;
        case GT7_HEARTBEAT_AUTO:
        default:
            /* Auto-adaptive:
               - If disconnected and fail count > 8 (~2s): fallback to 'A' to probe older GT7 versions
               - If in Luxury EV Cockpit (active_theme_hint == 2): send '~' to get EV torque & regen kW
               - Else (Motorsport, Tach, F1, MoTeC, WRC, JDM, Track): send 'C' to get tire surface, category, steer & lap ms
            */
            if (!connected && client->hb_fail_count > 8) {
                hb_char = 'A';
            } else if (client->active_theme_hint == 2) {
                hb_char = '~';
            } else {
                hb_char = 'C';
            }
            break;
    }
    client->active_hb_char = hb_char;
    const char heartbeat_msg[2] = { hb_char, '\0' };

    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(GT7_SEND_PORT);

    /* 1. If we have a locked/manual IP, send directly to it */
    if (strcmp(client->ps5_ip, "AUTO") != 0 && inet_pton(AF_INET, client->ps5_ip, &dest.sin_addr) == 1) {
        sendto(client->sock_fd, heartbeat_msg, 1, 0,
               (struct sockaddr *)&dest, sizeof(dest));
    }

    /* 2. If in Auto-Detect mode and not yet locked on a live stream, sweep subnet + broadcasts */
    if (client->auto_detect && !connected) {
        /* Global broadcast 255.255.255.255 */
        dest.sin_addr.s_addr = htonl(INADDR_BROADCAST);
        sendto(client->sock_fd, heartbeat_msg, 1, 0,
               (struct sockaddr *)&dest, sizeof(dest));

        /* Directed subnet broadcast e.g. 192.168.1.255 */
        char bcast_ip[64];
        snprintf(bcast_ip, sizeof(bcast_ip), "%s.255", client->subnet_prefix);
        if (inet_pton(AF_INET, bcast_ip, &dest.sin_addr) == 1) {
            sendto(client->sock_fd, heartbeat_msg, 1, 0,
                   (struct sockaddr *)&dest, sizeof(dest));
        }

        /* Active unicast sweep across the /24 subnet (32 hosts per pulse -> full subnet in ~2s) */
        for (int i = 0; i < 32; ++i) {
            char target[64];
            snprintf(target, sizeof(target), "%s.%u", client->subnet_prefix, client->scan_cursor);
            client->scan_cursor++;
            if (client->scan_cursor == 0 || client->scan_cursor >= 255) {
                client->scan_cursor = 1;
            }
            if (inet_pton(AF_INET, target, &dest.sin_addr) == 1) {
                sendto(client->sock_fd, heartbeat_msg, 1, 0,
                       (struct sockaddr *)&dest, sizeof(dest));
            }
        }
    }
}

static void gt7_parse_decrypted(gt7_client_t *client, gt7_state_t *state, const uint8_t *d, size_t len, uint32_t now_ms) {
    (void)len;
    state->pos_x = read_f32_le(d, 0x04);
    state->pos_y = read_f32_le(d, 0x08);
    state->pos_z = read_f32_le(d, 0x0C);

    /* When live packets arrive for the first time, CLEAR demo points so real track can be mapped cleanly */
    if (!state->demo_mode && !state->track_live_cleared) {
        state->track_pt_count = 0;
        state->track_pt_head = 0;
        state->track_min_x = state->pos_x - 10.0f;
        state->track_max_x = state->pos_x + 10.0f;
        state->track_min_z = state->pos_z - 10.0f;
        state->track_max_z = state->pos_z + 10.0f;
        state->track_is_closed = false;
        state->track_live_cleared = true;
        client->has_sample_pos = false;
        strncpy(state->circuit_name, "LIVE GT7 CIRCUIT", sizeof(state->circuit_name) - 1);
    }

    state->vel_x = read_f32_le(d, 0x10);
    state->vel_y = read_f32_le(d, 0x14);
    state->vel_z = read_f32_le(d, 0x18);

    /* Orientation Quaternion (0x01C..0x02B: X, Y, Z, W) */
    state->orient_x = read_f32_le(d, 0x1C);
    state->orient_y = read_f32_le(d, 0x20);
    state->orient_z = read_f32_le(d, 0x24);
    state->orient_w = read_f32_le(d, 0x28);

    /* 3-axis Angular Velocity in rad/s (0x02C..0x037: X, Y, Z) */
    state->angular_vel[0] = read_f32_le(d, 0x2C);
    state->angular_vel[1] = read_f32_le(d, 0x30);
    state->angular_vel[2] = read_f32_le(d, 0x34);

    /* Derive true Euler angles (pitch, yaw, roll) from orientation quaternion */
    float qx = state->orient_x;
    float qy = state->orient_y;
    float qz = state->orient_z;
    float qw = state->orient_w;
    float q_norm = sqrtf(qx * qx + qy * qy + qz * qz + qw * qw);
    if (q_norm > 0.001f) {
        qx /= q_norm;
        qy /= q_norm;
        qz /= q_norm;
        qw /= q_norm;

        float sinr_cosp = 2.0f * (qw * qx + qy * qz);
        float cosr_cosp = 1.0f - 2.0f * (qx * qx + qy * qy);
        float roll_rad = atan2f(sinr_cosp, cosr_cosp);

        float sinp = 2.0f * (qw * qy - qz * qx);
        float pitch_rad;
        if (fabsf(sinp) >= 1.0f) {
            pitch_rad = copysignf((float)M_PI / 2.0f, sinp);
        } else {
            pitch_rad = asinf(sinp);
        }

        float siny_cosp = 2.0f * (qw * qz + qx * qy);
        float cosy_cosp = 1.0f - 2.0f * (qy * qy + qz * qz);
        float yaw_rad = atan2f(siny_cosp, cosy_cosp);

        state->yaw   = yaw_rad;
        state->pitch = pitch_rad * (180.0f / (float)M_PI);
        state->roll  = roll_rad * (180.0f / (float)M_PI);
    }

    state->body_height = read_f32_le(d, 0x38) * 1000.0f;
    state->engine_rpm  = read_f32_le(d, 0x3C);

    state->fuel_level    = read_f32_le(d, 0x44);
    state->fuel_capacity = read_f32_le(d, 0x48);
    if (state->fuel_capacity > 0.01f) {
        state->fuel_pct = (state->fuel_level / state->fuel_capacity) * 100.0f;
    } else {
        state->fuel_pct = 100.0f;
    }

    state->speed_ms  = read_f32_le(d, 0x4C);
    state->speed_kmh = state->speed_ms * 3.6f;
    state->speed_mph = state->speed_ms * 2.236936f;
    if (state->speed_kmh > state->session_top_speed_kmh) {
        state->session_top_speed_kmh = state->speed_kmh;
    }
    if (state->speed_kmh > client->lap_top_speed) {
        client->lap_top_speed = state->speed_kmh;
    }

    state->turbo_bar    = read_f32_le(d, 0x50) - 1.0f;
    if (state->turbo_bar > state->peak_turbo_bar) {
        state->peak_turbo_bar = state->turbo_bar;
    }
    state->oil_pressure = read_f32_le(d, 0x54);
    state->water_temp   = read_f32_le(d, 0x58);
    state->oil_temp     = read_f32_le(d, 0x5C);

    for (int i = 0; i < 4; ++i) {
        state->tire_temp[i]   = read_f32_le(d, 0x60 + i * 4);
        state->wheel_angular_speed[i] = read_f32_le(d, 0xA4 + i * 4); /* rad/s (0xA4..0xB3) */
        state->tire_rps[i]    = fabsf(state->wheel_angular_speed[i]) / (2.0f * (float)M_PI);
        state->tire_radius[i] = read_f32_le(d, 0xB4 + i * 4);        /* m (0xB4..0xC3) */
        state->susp_height[i] = read_f32_le(d, 0xC4 + i * 4) * 1000.0f; /* mm (0xC4..0xD3) */

        /* Linear wheel tangential speed v = omega * r (m/s) */
        float wheel_speed = fabsf(state->wheel_angular_speed[i]) * state->tire_radius[i];
        if (state->speed_ms > 2.0f) {
            float slip = (wheel_speed - state->speed_ms) / state->speed_ms;
            if (slip < -1.0f) slip = -1.0f;
            if (slip > 1.0f)  slip = 1.0f;
            state->tire_slip_ratio[i] = fabsf(slip);
        } else {
            state->tire_slip_ratio[i] = 0.0f;
        }
    }

    state->packet_id     = read_u32_le(d, 0x70);
    state->current_lap   = read_i16_le(d, 0x74);
    state->total_laps    = read_i16_le(d, 0x76);
    state->best_lap_ms   = read_i32_le(d, 0x78);
    state->last_lap_ms   = read_i32_le(d, 0x7C);
    state->race_position = read_i16_le(d, 0x84);
    state->total_cars    = read_i16_le(d, 0x86);
    state->rpm_warning   = read_u16_le(d, 0x88);
    state->rpm_limiter   = read_u16_le(d, 0x8A);
    state->max_speed_kmh = read_u16_le(d, 0x8C);
    state->flags         = read_u16_le(d, 0x8E);

    uint8_t gears = d[0x90];
    state->current_gear   = (int)(gears & 0x0F);
    state->suggested_gear = (int)((gears >> 4) & 0x0F);

    state->throttle_pct = ((float)d[0x91] / 255.0f) * 100.0f;
    state->brake_pct    = ((float)d[0x92] / 255.0f) * 100.0f;
    if (state->brake_pct > state->peak_brake_pct) {
        state->peak_brake_pct = state->brake_pct;
    } else {
        state->peak_brake_pct *= 0.996f;
    }
    state->clutch_pct   = read_f32_le(d, 0xF4) * 100.0f;
    state->car_code     = read_i32_le(d, 0x124);

    state->rx_packet_len = len;
    client->hb_fail_count = 0;

    /* Check for Packet B extensions (len >= 316 / 0x13C) */
    if (len >= 316) {
        state->has_packet_b = true;
        state->wheel_rotation   = read_f32_le(d, 0x128); /* rad */
        state->steering_ang_vel = read_f32_le(d, 0x12C); /* rad/s */
        state->native_sway      = read_f32_le(d, 0x130); /* lateral accel */
        state->native_heave     = read_f32_le(d, 0x134); /* vertical accel */
        state->native_surge     = read_f32_le(d, 0x138); /* longitudinal accel */

        /* Directly use high-fidelity native G-forces if valid */
        if (fabsf(state->native_sway) > 0.001f || fabsf(state->native_surge) > 0.001f) {
            state->g_lat = state->native_sway;
            state->g_lon = state->native_surge;
        }
    } else {
        state->has_packet_b = false;
        state->wheel_rotation = 0.0f;
        state->steering_ang_vel = 0.0f;
        state->native_sway = 0.0f;
        state->native_heave = 0.0f;
        state->native_surge = 0.0f;
    }

    /* Strict separation between Packet C (len >= 368) and Packet ~ (len == 344) */
    if (len >= 368) {
        state->rx_packet_type = 'C';
        state->has_packet_c = true;
        state->has_packet_tilde = false;

        /* 0x13C..0x13F: Four-wheel contact surface type ('T', 'C', 'D', 'G', 'S') */
        state->surface_type[0] = d[0x13C];
        state->surface_type[1] = d[0x13D];
        state->surface_type[2] = d[0x13E];
        state->surface_type[3] = d[0x13F];

        /* 0x140..0x143: Current live lap time in ms */
        state->current_lap_time_ms = read_i32_le(d, 0x140);
        if (state->current_lap_time_ms > 0) {
            state->current_lap_ms = state->current_lap_time_ms;
        }

        /* 0x144..0x14B: Front wheel steering angle (rad -> deg) */
        float rad_fl = read_f32_le(d, 0x144);
        float rad_fr = read_f32_le(d, 0x148);
        state->steer_angle_fl_deg = rad_fl * (180.0f / (float)M_PI);
        state->steer_angle_fr_deg = rad_fr * (180.0f / (float)M_PI);

        /* 0x14C..0x14F: Wheelbase in meters */
        state->wheel_base_m = read_f32_le(d, 0x14C);

        /* 0x150..0x153: Car category string (4 bytes, e.g. "GR3", "GR4") */
        memcpy(state->car_category, d + 0x150, 4);
        state->car_category[4] = '\0';
        for (int c = 0; c < 4; ++c) {
            if ((uint8_t)state->car_category[c] < 32 || (uint8_t)state->car_category[c] > 126) {
                state->car_category[c] = '\0';
                break;
            }
        }

        /* 0x154..0x16F: 28-byte unconfirmed tail region, preserved safely */
        memcpy(state->unknown_tail, d + 0x154, 28);

    } else if (len == 344) {
        /* Packet ~ (344 bytes) - Advanced EV / Hybrid & Torque Vectoring */
        state->rx_packet_type = '~';
        state->has_packet_tilde = true;
        state->has_packet_c = false;

        /* 0x13C..0x13D: Filtered throttle & brake outputs (0..255 -> 0..100%) */
        state->throttle_filtered = ((float)d[0x13C] / 255.0f) * 100.0f;
        state->brake_filtered    = ((float)d[0x13D] / 255.0f) * 100.0f;

        /* 0x13E: Powertrain / Motor configuration flags */
        state->powertrain_flags = d[0x13E];

        /* 0x13F: Energy recovery available flag */
        state->energy_recovery_avail = (d[0x13F] != 0);

        /* 0x140..0x14F: 4-wheel torque vectoring in Nm (FL, FR, RL, RR) */
        state->torque_vector_nm[0] = read_f32_le(d, 0x140);
        state->torque_vector_nm[1] = read_f32_le(d, 0x144);
        state->torque_vector_nm[2] = read_f32_le(d, 0x148);
        state->torque_vector_nm[3] = read_f32_le(d, 0x14C);

        /* 0x150..0x153: Live energy recovery power */
        state->energy_recovery_kw = read_f32_le(d, 0x150);

        /* 0x154..0x157: Normalized engine output torque */
        state->engine_torque_norm = read_f32_le(d, 0x154);

    } else {
        /* Baseline Packet A (296B) or Packet B (316B) */
        state->rx_packet_type = (len >= 316) ? 'B' : 'A';
        state->has_packet_c = false;
        state->has_packet_tilde = false;
    }

    /* Calculate G-forces from velocity delta */
    if (client->prev_packet_ms > 0 && now_ms > client->prev_packet_ms) {
        float dt = (float)(now_ms - client->prev_packet_ms) / 1000.0f;
        if (dt > 0.005f && dt < 0.25f) {
            float dvx = state->vel_x - client->prev_vel_x;
            float dvz = state->vel_z - client->prev_vel_z;
            float accel_total = sqrtf(dvx * dvx + dvz * dvz) / (dt * 9.80665f);
            float lon_sign = (state->throttle_pct > state->brake_pct) ? 1.0f : -1.0f;
            state->g_lon = state->g_lon * 0.75f + (accel_total * lon_sign * 0.4f) * 0.25f;

            float dyaw = state->yaw - client->prev_yaw;
            state->g_lat = state->g_lat * 0.75f + ((dyaw / dt) * state->speed_ms / 9.80665f) * 0.25f;
            if (state->g_lat > 3.0f) state->g_lat = 3.0f;
            if (state->g_lat < -3.0f) state->g_lat = -3.0f;
            if (state->g_lon > 3.0f) state->g_lon = 3.0f;
            if (state->g_lon < -3.0f) state->g_lon = -3.0f;
        }
    }
    client->prev_vel_x = state->vel_x;
    client->prev_vel_y = state->vel_y;
    client->prev_vel_z = state->vel_z;
    client->prev_yaw   = state->yaw;
    client->prev_packet_ms = now_ms;

    /* Track current lap elapsed time, sectors & lap history */
    if (state->current_lap != client->tracked_lap) {
        if (client->tracked_lap > 0 && state->last_lap_ms > 0) {
            float used = (client->lap_start_fuel > state->fuel_pct) ? (client->lap_start_fuel - state->fuel_pct) : 2.8f;
            state->fuel_burn_per_lap = used;

            int idx = state->lap_history_count < GT7_MAX_LAPS ? state->lap_history_count++ : (GT7_MAX_LAPS - 1);
            if (state->lap_history_count == GT7_MAX_LAPS) {
                memmove(&state->lap_history[0], &state->lap_history[1], sizeof(gt7_lap_record_t) * (GT7_MAX_LAPS - 1));
            }
            state->lap_history[idx].lap_num = client->tracked_lap;
            state->lap_history[idx].time_ms = state->last_lap_ms;
            state->lap_history[idx].s1_ms   = (int32_t)(state->last_lap_ms * 0.335f);
            state->lap_history[idx].s2_ms   = (int32_t)(state->last_lap_ms * 0.425f);
            state->lap_history[idx].s3_ms   = state->last_lap_ms - state->lap_history[idx].s1_ms - state->lap_history[idx].s2_ms;
            state->lap_history[idx].fuel_used = used;
            state->lap_history[idx].top_speed_kmh = client->lap_top_speed;
            state->track_is_closed = true; /* Full lap completed, close the circuit loop cleanly! */
        }
        client->tracked_lap = state->current_lap;
        client->lap_start_ms = now_ms;
        client->lap_start_fuel = state->fuel_pct;
        client->lap_top_speed = state->speed_kmh;
    }
    if (state->current_lap > 0 && !(state->flags & GT7_FLAG_PAUSED)) {
        state->current_lap_ms = (int32_t)(now_ms - client->lap_start_ms);
        if (state->best_lap_ms > 0) {
            state->lap_progress = (float)state->current_lap_ms / (float)state->best_lap_ms;
            if (state->lap_progress > 1.0f) state->lap_progress = 1.0f;
            state->current_sector = (state->lap_progress < 0.33f) ? 0 : ((state->lap_progress < 0.76f) ? 1 : 2);
        }
    }

    /* Record 2D GPS Circuit coordinate with distance-based threshold and teleport safety */
    if (!state->demo_mode && state->speed_kmh > 8.0f) {
        float dx = client->has_sample_pos ? (state->pos_x - client->last_sample_x) : 999.0f;
        float dz = client->has_sample_pos ? (state->pos_z - client->last_sample_z) : 999.0f;
        float dist_sq = dx * dx + dz * dz;

        /* Sample if car moved >= 15m (dist_sq >= 225.0f) or timeout > 400ms while moving */
        if (!client->has_sample_pos || dist_sq >= 225.0f || (now_ms - client->last_track_sample_ms >= 400 && dist_sq >= 16.0f)) {
            bool is_teleport = (client->has_sample_pos && dist_sq > 62500.0f); /* > 250m sudden jump */
            int h = state->track_pt_head;
            state->track_pts[h].x = state->pos_x;
            state->track_pts[h].z = state->pos_z;
            state->track_pts[h].speed_kmh = state->speed_kmh;
            state->track_pts[h].pedal_state = (state->brake_pct > 20.0f) ? 2 : ((state->throttle_pct > 50.0f) ? 1 : 0);
            state->track_pts[h].segment_break = is_teleport ? 1 : 0;

            state->track_pt_head = (h + 1) % GT7_TRACK_PTS;
            if (state->track_pt_count < GT7_TRACK_PTS) {
                state->track_pt_count++;
            }

            if (!is_teleport) {
                if (state->pos_x < state->track_min_x) state->track_min_x = state->pos_x;
                if (state->pos_x > state->track_max_x) state->track_max_x = state->pos_x;
                if (state->pos_z < state->track_min_z) state->track_min_z = state->pos_z;
                if (state->pos_z > state->track_max_z) state->track_max_z = state->pos_z;
            }

            client->last_sample_x = state->pos_x;
            client->last_sample_z = state->pos_z;
            client->has_sample_pos = true;
            client->last_track_sample_ms = now_ms;
        }
    }

    /* Update pedal & speed trace buffer */
    state->trace_throttle[state->trace_head] = state->throttle_pct;
    state->trace_brake[state->trace_head]    = state->brake_pct;
    state->trace_speed[state->trace_head]    = state->speed_kmh;
    state->trace_head = (state->trace_head + 1) % GT7_TRACE_LEN;
}

void gt7_update(gt7_client_t *client, gt7_state_t *state, uint32_t now_ms) {
    uint32_t hb_interval = (client->auto_detect && !state->connected) ? 250 : 1000;
    if (now_ms - client->last_heartbeat_ms >= hb_interval) {
        gt7_send_heartbeat(client, state->connected);
        client->last_heartbeat_ms = now_ms;
    }

    uint8_t raw_buf[512];
    uint8_t dec_buf[512];
    struct sockaddr_in sender;
    socklen_t sender_len = sizeof(sender);

    while (client->sock_fd >= 0) {
        ssize_t n = recvfrom(client->sock_fd, raw_buf, sizeof(raw_buf), 0,
                             (struct sockaddr *)&sender, &sender_len);
        if (n <= 0) {
            break;
        }

        if (gt7_decrypt_packet(raw_buf, (size_t)n, dec_buf)) {
            state->connected = true;
            state->demo_mode = false;
            state->last_rx_ms = now_ms;
            char detected_ip[INET_ADDRSTRLEN];
            if (inet_ntop(AF_INET, &sender.sin_addr, detected_ip, sizeof(detected_ip))) {
                /* Lock onto discovered PS5 IP while preserving auto_detect flag */
                strncpy(client->ps5_ip, detected_ip, sizeof(client->ps5_ip) - 1);
                client->ps5_ip[sizeof(client->ps5_ip) - 1] = '\0';
            }
            gt7_parse_decrypted(client, state, dec_buf, (size_t)n, now_ms);
        }
    }

    if (state->connected && (now_ms - state->last_rx_ms > 2500)) {
        state->connected = false;
    }
}

void gt7_update_demo(gt7_state_t *state, uint32_t now_ms, int theme_index) {
    float t = (float)now_ms / 1000.0f;

    float max_rpm = 9000.0f;
    float warn_rpm = 7800.0f;
    int max_gear = 6;
    float top_speed = 295.0f;

    if (theme_index == 0) { /* Official Gran Turismo 7 Gr.3 / Gr.4 Spec */
        max_rpm = 8600.0f;
        warn_rpm = 7600.0f;
        max_gear = 6;
        top_speed = 290.0f;
    } else if (theme_index == 1) { /* Digital Tachometer Motorsport Dash */
        max_rpm = 9500.0f;
        warn_rpm = 8500.0f;
        max_gear = 7;
        top_speed = 315.0f;
    } else if (theme_index == 2) { /* Modern Luxury EV Digital Cockpit */
        max_rpm = 16000.0f;
        warn_rpm = 14500.0f;
        max_gear = 6; /* Multi-speed transmission for luxury EV/hybrid GT7 telemetry */
        top_speed = 310.0f;
    } else if (theme_index == 3) { /* F1 W14 / Super Formula */
        max_rpm = 13500.0f;
        warn_rpm = 11800.0f;
        max_gear = 8;
        top_speed = 342.0f;
    } else if (theme_index == 4) { /* GT3 Endurance */
        max_rpm = 8800.0f;
        warn_rpm = 7900.0f;
        max_gear = 6;
        top_speed = 288.0f;
    } else if (theme_index == 5) { /* WRC Rally Gr.B */
        max_rpm = 7800.0f;
        warn_rpm = 6800.0f;
        max_gear = 6;
        top_speed = 225.0f;
    } else if (theme_index == 6) { /* JDM Street Tuned */
        max_rpm = 8500.0f;
        warn_rpm = 7400.0f;
        max_gear = 6;
        top_speed = 278.0f;
    } else { /* Track / Engineer Mode */
        max_rpm = 9200.0f;
        warn_rpm = 8200.0f;
        max_gear = 7;
        top_speed = 305.0f;
    }

    state->rpm_warning = (uint16_t)warn_rpm;
    state->rpm_limiter = (uint16_t)max_rpm;

    /* Move live car position smoothly around the 360-point circuit map */
    float lap_duration = 45.0f; /* 45s compressed visual circuit loop */
    state->lap_progress = fmodf(t, lap_duration) / lap_duration;
    int pt_idx = (int)(state->lap_progress * GT7_TRACK_PTS) % GT7_TRACK_PTS;
    state->pos_x = state->track_pts[pt_idx].x;
    state->pos_z = state->track_pts[pt_idx].z;
    state->current_sector = (state->lap_progress < 0.33f) ? 0 : ((state->lap_progress < 0.72f) ? 1 : 2);

    float cycle = fmodf(t, 18.0f);
    float speed_norm = 0.0f;
    float throttle = 0.0f;
    float brake = 0.0f;
    int suggested = 15;

    if (cycle < 11.5f) {
        float prog = cycle / 11.5f;
        speed_norm = 0.24f + 0.76f * powf(prog, 0.72f);
        throttle = 98.0f + 2.0f * sinf(t * 5.0f);
        brake = 0.0f;
    } else if (cycle < 14.2f) {
        float prog = (cycle - 11.5f) / 2.7f;
        speed_norm = 1.0f - 0.74f * powf(prog, 0.85f);
        throttle = (prog > 0.4f && fmodf(prog * 10.0f, 2.0f) < 0.35f) ? 38.0f : 0.0f;
        brake = (prog < 0.6f) ? 97.0f : (97.0f * (1.0f - (prog - 0.6f) / 0.4f));
        suggested = 2;
    } else {
        float prog = (cycle - 14.2f) / 3.8f;
        speed_norm = 0.26f + 0.25f * prog;
        throttle = 45.0f + 55.0f * prog;
        brake = (prog < 0.2f) ? 20.0f * (1.0f - prog * 5.0f) : 0.0f;
        suggested = (prog < 0.5f) ? 2 : 3;
    }

    state->speed_kmh = speed_norm * top_speed;
    state->speed_ms  = state->speed_kmh / 3.6f;
    state->speed_mph = state->speed_ms * 2.236936f;
    if (state->speed_kmh > state->session_top_speed_kmh) {
        state->session_top_speed_kmh = state->speed_kmh;
    }

    float gear_float = 1.0f + speed_norm * (float)(max_gear - 0.15f);
    int gear = (int)gear_float;
    if (gear < 1) gear = 1;
    if (gear > max_gear) gear = max_gear;
    state->current_gear = gear;
    state->suggested_gear = suggested;

    float in_gear_frac = gear_float - (float)gear;
    float base_rpm = max_rpm * (0.52f + 0.46f * in_gear_frac);
    if (cycle >= 11.5f && cycle < 14.2f) {
        base_rpm = max_rpm * (0.60f + 0.28f * (1.0f - in_gear_frac));
    }
    state->engine_rpm = base_rpm + 45.0f * sinf(t * 32.0f);

    state->throttle_pct = throttle;
    state->brake_pct    = brake;
    state->clutch_pct   = 0.0f;
    if (brake > state->peak_brake_pct) state->peak_brake_pct = brake;

    state->turbo_bar = (throttle > 40.0f)
        ? (-0.2f + 1.68f * (throttle / 100.0f) * (state->engine_rpm / max_rpm))
        : (-0.65f + 0.2f * sinf(t * 4.0f));
    if (state->turbo_bar > state->peak_turbo_bar) state->peak_turbo_bar = state->turbo_bar;

    state->oil_pressure = 4.2f + 2.4f * (state->engine_rpm / max_rpm);
    state->water_temp   = 86.5f + 2.5f * sinf(t * 0.15f);
    state->oil_temp     = 102.0f + 4.0f * sinf(t * 0.12f);

    state->fuel_capacity = 100.0f;
    state->fuel_pct      = 68.4f - fmodf(t * 0.08f, 45.0f);
    state->fuel_level    = state->fuel_pct;
    state->fuel_burn_per_lap = 3.2f;

    state->g_lon = (throttle - brake * 1.45f) / 68.0f;
    state->g_lat = (cycle > 12.8f) ? (1.85f * sinf((cycle - 12.8f) * 1.2f)) : (0.25f * sinf(t * 1.7f));
    state->roll  = state->g_lat * 2.5f;
    state->pitch = -state->g_lon * 1.8f;
    state->body_height = 58.0f - state->g_lon * 6.0f;

    /* Synthesize 3D Orientation Quaternion & 3-Axis Angular Velocity */
    float r_rad = state->roll * ((float)M_PI / 180.0f);
    float p_rad = state->pitch * ((float)M_PI / 180.0f);
    float y_rad = state->yaw;
    float cr = cosf(r_rad * 0.5f), sr = sinf(r_rad * 0.5f);
    float cp = cosf(p_rad * 0.5f), sp = sinf(p_rad * 0.5f);
    float cy = cosf(y_rad * 0.5f), sy = sinf(y_rad * 0.5f);
    state->orient_w = cr * cp * cy + sr * sp * sy;
    state->orient_x = sr * cp * cy - cr * sp * sy;
    state->orient_y = cr * sp * cy + sr * cp * sy;
    state->orient_z = cr * cp * sy - sr * sp * cy;

    state->angular_vel[0] = -state->g_lon * 0.22f; /* rad/s pitch rate */
    state->angular_vel[1] = state->g_lat * 0.88f;  /* rad/s yaw rate */
    state->angular_vel[2] = state->g_lat * 0.35f;  /* rad/s roll rate */

    for (int i = 0; i < 4; ++i) {
        float load_heat = (i == 0 || i == 2) ? (state->g_lat * 6.5f) : (-state->g_lat * 6.5f);
        float brake_heat = (i < 2) ? (brake * 0.12f) : (brake * 0.06f);
        state->tire_temp[i] = 81.0f + load_heat + brake_heat + 3.0f * sinf(t * 0.5f + (float)i);
        state->tire_slip_ratio[i] = (brake > 80.0f || (throttle > 85.0f && gear <= 2))
            ? (0.18f + 0.12f * fabsf(sinf(t * 15.0f + (float)i)))
            : (0.02f + 0.03f * fabsf(state->g_lat));
        state->susp_height[i] = 62.0f + ((i < 2) ? (state->g_lon * 8.0f) : (-state->g_lon * 8.0f));

        state->tire_radius[i] = 0.33f;
        state->wheel_angular_speed[i] = (state->speed_ms / state->tire_radius[i]) * (1.0f + state->tire_slip_ratio[i]);
        state->tire_rps[i] = fabsf(state->wheel_angular_speed[i]) / (2.0f * (float)M_PI);
    }

    state->current_lap   = 5;
    state->total_laps    = 15;
    state->best_lap_ms   = 91428;
    state->last_lap_ms   = 91892;
    state->current_lap_ms = (int32_t)(state->lap_progress * 91428.0f);
    state->delta_ms      = (int32_t)(-240.0f * sinf(t * 0.35f) - 85.0f);
    state->race_position = 2;
    state->total_cars    = 20;

    uint16_t flags = GT7_FLAG_IN_RACE | GT7_FLAG_IN_GEAR | GT7_FLAG_HAS_TURBO | GT7_FLAG_LIGHTS_ACTIVE | GT7_FLAG_LOW_BEAM;
    if (state->engine_rpm >= warn_rpm) {
        flags |= GT7_FLAG_REV_BLINK;
    }
    if (throttle > 85.0f && gear <= 3 && fabsf(state->g_lat) > 0.6f) {
        flags |= GT7_FLAG_TCS_ACTIVE;
    }
    if (fabsf(state->g_lat) > 1.45f) {
        flags |= GT7_FLAG_ASM_ACTIVE;
    }
    if (theme_index == 4 && cycle > 13.5f && cycle < 14.6f) {
        flags |= GT7_FLAG_HANDBRAKE;
    }
    state->flags = flags;

    static uint32_t last_trace_ms = 0;
    if (now_ms - last_trace_ms >= 32) {
        state->trace_throttle[state->trace_head] = state->throttle_pct;
        state->trace_brake[state->trace_head]    = state->brake_pct;
        state->trace_speed[state->trace_head]    = state->speed_kmh;
        state->trace_head = (state->trace_head + 1) % GT7_TRACE_LEN;
        last_trace_ms = now_ms;
    }

    /* Synthesize Extended GT7 Telemetry Channels (Packet B, Packet ~, Packet C) */
    state->has_packet_b = true;
    state->native_sway  = state->g_lat;
    state->native_surge = state->g_lon;
    state->native_heave = 0.98f + 0.12f * sinf(t * 18.0f);
    state->steering_ang_vel = state->g_lat * 2.4f;
    state->wheel_rotation = state->speed_ms / 0.33f;

    if (theme_index == 2) {
        /* Mode 3: Modern Luxury EV Digital Cockpit -> Packet ~ (344 bytes) */
        state->rx_packet_len = 344;
        state->rx_packet_type = '~';
        state->has_packet_tilde = true;
        state->has_packet_c = false;

        state->battery_soc_pct = 78.4f - fmodf(t * 0.05f, 25.0f);
        state->throttle_filtered = state->throttle_pct;
        state->brake_filtered    = state->brake_pct;
        state->powertrain_flags  = 0x03; /* AWD Dual Motor Configuration */
        state->energy_recovery_avail = true;
        state->engine_torque_norm = (throttle > 5.0f) ? (throttle / 100.0f) : (brake > 5.0f ? -brake / 100.0f : 0.0f);

        if (brake > 5.0f) {
            /* Active Regenerative Braking up to -285 kW */
            state->energy_recovery_kw = -(brake * 2.7f + 35.0f * (state->speed_kmh / 300.0f));
            state->abs_cut_pct = (brake > 82.0f) ? (18.0f + 22.0f * fabsf(sinf(t * 16.0f))) : 0.0f;
            state->tcs_cut_pct = 0.0f;
        } else {
            /* Live Electric Drive Output up to +490 kW */
            state->energy_recovery_kw = (throttle > 5.0f) ? (throttle * 4.4f + 45.0f) : 0.0f;
            state->abs_cut_pct = 0.0f;
            state->tcs_cut_pct = (throttle > 85.0f && gear <= 2) ? (24.0f + 30.0f * fabsf(sinf(t * 14.0f))) : 0.0f;
        }

        /* 4-Wheel Torque Vectoring in Nm */
        float base_nm = (throttle > 5.0f) ? (throttle * 7.2f) : (brake > 5.0f ? -brake * 3.8f : 0.0f);
        float yaw_bias = state->g_lat * 0.28f;
        state->torque_vector_nm[0] = base_nm * (0.46f - yaw_bias);
        state->torque_vector_nm[1] = base_nm * (0.46f + yaw_bias);
        state->torque_vector_nm[2] = base_nm * (0.54f - yaw_bias);
        state->torque_vector_nm[3] = base_nm * (0.54f + yaw_bias);
    } else {
        /* Motorsport and Track DDU Modes -> Packet C (368 bytes) */
        state->rx_packet_len = 368;
        state->rx_packet_type = 'C';
        state->has_packet_c = true;
        state->has_packet_tilde = false;

        /* Official GT7 Car Category tag (4-char null-terminated) */
        if (theme_index == 0) strncpy(state->car_category, "GR3", sizeof(state->car_category));
        else if (theme_index == 1) strncpy(state->car_category, "GR2", sizeof(state->car_category));
        else if (theme_index == 3) strncpy(state->car_category, "GR1", sizeof(state->car_category));
        else if (theme_index == 4) strncpy(state->car_category, "GR3", sizeof(state->car_category));
        else if (theme_index == 5) strncpy(state->car_category, "GRB", sizeof(state->car_category));
        else if (theme_index == 6) strncpy(state->car_category, "ROAD", sizeof(state->car_category));
        else strncpy(state->car_category, "SPEC", sizeof(state->car_category));
        state->car_category[4] = '\0';

        memset(state->unknown_tail, 0, sizeof(state->unknown_tail));

        /* Dynamic Contact Patch Surface */
        state->surface_type[0] = 'T';
        state->surface_type[1] = 'T';
        state->surface_type[2] = 'T';
        state->surface_type[3] = 'T';

        if (cycle >= 12.2f && cycle <= 14.0f) {
            /* Apex curb rumble contact on inside tires */
            state->surface_type[0] = 'C';
            state->surface_type[2] = 'C';
        }

        /* Front steering angle in degrees */
        state->steer_angle_fl_deg = state->g_lat * 11.5f;
        state->steer_angle_fr_deg = state->g_lat * 11.1f;
        state->wheel_base_m = 2.71f;

        /* Downforce based on speed squared */
        float spd_factor = (state->speed_kmh / 300.0f);
        state->downforce_front_kg = 215.0f * spd_factor * spd_factor;
        state->downforce_rear_kg  = 385.0f * spd_factor * spd_factor;
        state->trans_oil_temp     = 95.0f + 3.0f * sinf(t * 0.12f);

        state->tcs_cut_pct = (throttle > 85.0f && gear <= 2 && fabsf(state->g_lat) > 0.5f) ? (32.0f + 25.0f * sinf(t * 14.0f)) : 0.0f;
        state->abs_cut_pct = (brake > 82.0f) ? (28.0f + 20.0f * sinf(t * 16.0f)) : 0.0f;
        state->current_lap_time_ms = state->current_lap_ms;
    }
}

void gt7_close(gt7_client_t *client) {
    if (client->sock_fd >= 0) {
        close(client->sock_fd);
        client->sock_fd = -1;
    }
}
