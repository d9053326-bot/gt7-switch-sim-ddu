#ifndef GT7_TELEMETRY_H
#define GT7_TELEMETRY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GT7_SEND_PORT 33739
#define GT7_RECV_PORT 33740
#define GT7_MAGIC_G7S0 0x47375330U

#define GT7_APP_VERSION "_aw_1.10.30.0005"

#define GT7_TRACE_LEN 160
#define GT7_TRACK_PTS 500
#define GT7_MAX_LAPS  16

typedef enum {
    GT7_CIRCUIT_SUZUKA      = 0, /* Suzuka GP Technical Circuit */
    GT7_CIRCUIT_NURBURGRING = 1, /* Nürburgring GP Endurance */
    GT7_CIRCUIT_LE_MANS     = 2, /* Circuit de la Sarthe High-Speed */
    GT7_CIRCUIT_COUNT       = 3
} gt7_circuit_preset_t;

/* Heartbeat Negotiation & Telemetry Protocol Modes */
typedef enum {
    GT7_HEARTBEAT_AUTO      = 0, /* Smart Adaptive: '~' for EV Cockpit, 'C' for Motorsport, fallback to 'A' */
    GT7_HEARTBEAT_PKT_C     = 1, /* Force Packet C (368 Bytes, Heartbeat 'C') - Surface type, Category, Lap MS */
    GT7_HEARTBEAT_PKT_TILDE = 2, /* Force Packet ~ (344 Bytes, Heartbeat '~') - EV Torque Vectors & Regen kW */
    GT7_HEARTBEAT_PKT_A     = 3, /* Force Packet A (296 Bytes, Heartbeat 'A') - Baseline GT7 Telemetry */
    GT7_HEARTBEAT_PKT_B     = 4, /* Force Packet B (316 Bytes, Heartbeat 'B') - 6-DoF Inertial Acceleration */
    GT7_HEARTBEAT_COUNT     = 5
} gt7_heartbeat_mode_t;

/* Simulator Interface Flags (Offset 0x8E) */
typedef enum {
    GT7_FLAG_IN_RACE       = (1 << 0),
    GT7_FLAG_PAUSED        = (1 << 1),
    GT7_FLAG_LOADING       = (1 << 2),
    GT7_FLAG_IN_GEAR       = (1 << 3),
    GT7_FLAG_HAS_TURBO     = (1 << 4),
    GT7_FLAG_REV_BLINK     = (1 << 5),
    GT7_FLAG_HANDBRAKE     = (1 << 6),
    GT7_FLAG_LIGHTS_ACTIVE = (1 << 7),
    GT7_FLAG_HIGH_BEAM     = (1 << 8),
    GT7_FLAG_LOW_BEAM      = (1 << 9),
    GT7_FLAG_ASM_ACTIVE    = (1 << 10),
    GT7_FLAG_TCS_ACTIVE    = (1 << 11)
} gt7_flags_t;

typedef struct {
    float x;
    float z;
    float speed_kmh;
    uint8_t pedal_state;   /* 0 = coast, 1 = throttle, 2 = brake */
    uint8_t segment_break; /* 1 if teleported/jumped from previous point */
} gt7_track_pt_t;

typedef struct {
    int16_t lap_num;
    int32_t time_ms;
    int32_t s1_ms;
    int32_t s2_ms;
    int32_t s3_ms;
    float fuel_used;
    float top_speed_kmh;
} gt7_lap_record_t;

/* Decoded & derived telemetry state ready for HUD rendering */
typedef struct {
    bool connected;          /* True if packets received recently */
    bool demo_mode;          /* True if running synthetic demo telemetry */
    uint32_t last_rx_ms;     /* Timestamp of last valid packet */
    uint32_t packet_id;      /* Tick counter (0x70) */

    /* Engine & Speed */
    float engine_rpm;        /* Current RPM (0x3C) */
    uint16_t rpm_warning;    /* Shift light start RPM (0x88) */
    uint16_t rpm_limiter;    /* Rev limiter RPM (0x8A) */
    float speed_ms;          /* Speed in m/s (0x4C) */
    float speed_kmh;         /* Speed in km/h */
    float speed_mph;         /* Speed in mph */
    float session_top_speed_kmh;
    uint16_t max_speed_kmh;  /* Car estimated top speed (0x8C) */

    /* Official GT7 MFD Interactive Parameters */
    int mfd_page;            /* 0: Radar/Track, 1: Fuel Map, 2: Brake Balance, 3: TCS */
    int gt7_tcs_level;       /* 0 .. 5 (GT7 MFD TCS setting) */
    int gt7_brake_bal;       /* -5 (Front) .. +5 (Rear) */
    int gt7_fuel_map;        /* 1 (Power) .. 6 (Lean) */

    /* Transmission & Pedals */
    int current_gear;        /* 0 = R, 1..8 = 1st..8th, -1 = N */
    int suggested_gear;      /* 1..8 or 0/15 if no suggestion */
    float throttle_pct;      /* 0.0 - 100.0% (from 0x91) */
    float brake_pct;         /* 0.0 - 100.0% (from 0x92) */
    float clutch_pct;        /* 0.0 - 100.0% (from 0xF4) */
    float peak_brake_pct;    /* Recent peak brake pressure */

    /* Temperatures, Pressures & Fuel */
    float fuel_level;        /* Liters or % (0x44) */
    float fuel_capacity;     /* Max fuel capacity (0x48) */
    float fuel_pct;          /* 0.0 - 100.0% */
    float fuel_burn_per_lap; /* Estimated fuel per lap */
    float turbo_bar;         /* Relative boost pressure in Bar (from 0x50: val - 1.0) */
    float peak_turbo_bar;    /* Peak boost recorded */
    float oil_pressure;      /* Bar (0x54) */
    float water_temp;        /* Celsius (0x58) */
    float oil_temp;          /* Celsius (0x5C) */

    /* Tires (0: FL, 1: FR, 2: RL, 3: RR) */
    float tire_temp[4];      /* Surface temperature in Celsius (0x60..0x6C) */
    float wheel_angular_speed[4]; /* Wheel angular velocity in rad/s (0xA4..0xB0) */
    float tire_rps[4];       /* Backwards compatibility: wheel rev/sec (wheel_angular_speed / 2*pi) */
    float tire_radius[4];    /* Tire radius in meters (0xB4..0xC0) */
    float tire_slip_ratio[4];/* Calculated wheel slip ratio for traction visualization */
    float susp_height[4];    /* Suspension height in mm (0xC4..0xD0) */

    /* Laps, Sectors & Timing */
    int16_t current_lap;     /* Current lap (0x74) */
    int16_t total_laps;      /* Total race laps (0x76) */
    int32_t best_lap_ms;     /* Best lap in ms (0x78) */
    int32_t last_lap_ms;     /* Last lap in ms (0x7C) */
    int32_t current_lap_ms;  /* Estimated current lap elapsed ms */
    int32_t delta_ms;        /* Live delta vs best lap (ms) */
    int32_t optimal_lap_ms;  /* Theoretical best lap (sum of best sectors) */
    int32_t cur_sector_ms[3];/* Current lap S1, S2, S3 */
    int32_t best_sector_ms[3];/* Session best S1, S2, S3 */
    int current_sector;      /* 0 = S1, 1 = S2, 2 = S3 */
    int16_t race_position;   /* Current position (0x84) */
    int16_t total_cars;      /* Total cars in race (0x86) */

    /* Dynamics & World Coordinates */
    float pos_x, pos_y, pos_z;
    float vel_x, vel_y, vel_z;
    /* Raw Orientation Quaternion (0x01C..0x02B: x, y, z, w) & Angular Velocity (rad/s) */
    float orient_x, orient_y, orient_z, orient_w;
    float angular_vel[3];    /* 3-axis body angular velocity in rad/s (0x02C..0x037) */
    float pitch, yaw, roll;  /* Derived true Euler angles (rad) */
    float g_lat;             /* Lateral G-force (cornering) */
    float g_lon;             /* Longitudinal G-force (braking/accel) */
    float body_height;       /* Ride height in mm (0x38) */
    int32_t car_code;        /* GT7 internal car ID (0x124) */
    uint16_t flags;          /* Bitmask gt7_flags_t (0x8E) */

    /* Historical speed/pedal trace ring buffer for graphs */
    float trace_throttle[GT7_TRACE_LEN];
    float trace_brake[GT7_TRACE_LEN];
    float trace_speed[GT7_TRACE_LEN];
    int trace_head;

    /* 2D GPS Circuit Map Recording & Bounding Box */
    gt7_track_pt_t track_pts[GT7_TRACK_PTS];
    int track_pt_count;
    int track_pt_head;
    float track_min_x, track_max_x;
    float track_min_z, track_max_z;
    float lap_progress;      /* 0.0 .. 1.0 around circuit */
    bool track_is_closed;    /* True if lap completed or demo loop */
    bool track_live_cleared; /* True if demo points cleared upon live stream */
    int demo_circuit_id;     /* 0: Suzuka, 1: Nurburgring, 2: Le Mans */
    char circuit_name[48];
    float track_length_km;

    /* Extended GT7 Telemetry Channels (Packet B, Packet ~, Packet C) */
    size_t rx_packet_len;    /* Actual decrypted packet length (296, 316, 344, 368) */
    char rx_packet_type;     /* 'A', 'B', '~', 'C' */
    bool has_packet_b;       /* True if packet >= 316 bytes */
    bool has_packet_tilde;   /* True if packet == 344 bytes */
    bool has_packet_c;       /* True if packet == 368 bytes */

    /* Packet B (0x128..0x13B) - Native 6-DoF Inertial Acceleration & Rates */
    float native_sway;       /* Live lateral acceleration (0x130) */
    float native_heave;      /* Live vertical bump acceleration (0x134) */
    float native_surge;      /* Live longitudinal acceleration (0x138) */
    float steering_ang_vel;  /* Steering angular velocity in rad/s (0x12C) */
    float wheel_rotation;    /* Wheel rotation in rad (0x128) */

    /* Packet ~ (0x13C..0x157) - Advanced EV / Hybrid & Actuator Channels */
    float throttle_filtered; /* Filtered throttle % (0x13C, 0-255 -> 0-100%) */
    float brake_filtered;    /* Filtered brake % (0x13D, 0-255 -> 0-100%) */
    uint8_t powertrain_flags;/* Powertrain / Motor configuration flags (0x13E) */
    bool energy_recovery_avail; /* Energy recovery data availability flag (0x13F) */
    float tcs_cut_pct;       /* Active TCS engine throttle cut % */
    float abs_cut_pct;       /* Active ABS brake modulation cut % */
    float torque_vector_nm[4]; /* Per-wheel motor torque in Nm (0x140..0x14C: FL, FR, RL, RR) */
    float energy_recovery_kw;/* Live EV/Hybrid regenerative recharge power (0x150) */
    float engine_torque_norm;/* Normalized engine output torque (0x154) */
    float battery_soc_pct;   /* EV Battery State of Charge % */

    /* Packet C (0x13C..0x16F) - Motorsport Contact Patch, Steering & Category */
    uint8_t surface_type[4]; /* Tire contact surface type: 'T'=Tarmac, 'C'=Curb, 'D'=Dirt, 'G'=Grass, 'S'=Snow (0x13C..0x13F) */
    int32_t current_lap_time_ms; /* GT7 internal millisecond-exact lap timer (0x140) */
    float steer_angle_fl_deg;/* Front Left wheel steering angle in degrees (0x144, converted from rad) */
    float steer_angle_fr_deg;/* Front Right wheel steering angle in degrees (0x148, converted from rad) */
    float wheel_base_m;      /* Vehicle wheelbase in meters (0x14C) */
    char car_category[8];    /* GT7 Category string: e.g. "GR3", "GR4", "GR1", "GRB" (0x150..0x153) */
    uint8_t unknown_tail[28];/* Unconfirmed tail area (0x154..0x16F), preserved for verification */
    float downforce_front_kg;/* Front aero downforce in kg */
    float downforce_rear_kg; /* Rear aero downforce in kg */
    float trans_oil_temp;    /* Transmission oil temp in °C */

    /* Lap History Table for Track Info Mode */
    gt7_lap_record_t lap_history[GT7_MAX_LAPS];
    int lap_history_count;
} gt7_state_t;

typedef struct {
    int sock_fd;
    bool auto_detect;        /* True = Auto-detect PS5 via subnet scan + broadcast (default) */
    char ps5_ip[64];         /* Locked or manual PS5 IP */
    char local_ip[64];       /* Detected local Wi-Fi IP of Switch/Host */
    char subnet_prefix[48];  /* e.g. "192.168.1" */
    uint8_t scan_cursor;     /* Current host octet being swept during auto-discovery */
    uint32_t last_heartbeat_ms;
    gt7_heartbeat_mode_t heartbeat_mode; /* AUTO, PKT_C, PKT_TILDE, PKT_A, PKT_B */
    char active_hb_char;     /* 'A', 'B', '~', 'C' currently being sent */
    int active_theme_hint;   /* UI theme hint (e.g. 2 for EV) */
    uint32_t hb_fail_count;  /* Count of heartbeats without reply, for auto-fallback */
    float prev_vel_x, prev_vel_y, prev_vel_z;
    float prev_yaw;
    uint32_t prev_packet_ms;
    int16_t tracked_lap;
    uint32_t lap_start_ms;
    float lap_start_fuel;
    float lap_top_speed;
    uint32_t last_track_sample_ms;
    float last_sample_x, last_sample_z;
    bool has_sample_pos;
} gt7_client_t;

bool gt7_init(gt7_client_t *client, gt7_state_t *state, const char *ps5_ip);
void gt7_set_ip(gt7_client_t *client, const char *ps5_ip);
void gt7_set_auto_detect(gt7_client_t *client, gt7_state_t *state);
void gt7_set_heartbeat_mode(gt7_client_t *client, gt7_heartbeat_mode_t mode);
void gt7_cycle_heartbeat_mode(gt7_client_t *client);
const char *gt7_heartbeat_mode_name(gt7_heartbeat_mode_t mode);
void gt7_reset_session(gt7_state_t *state);
void gt7_cycle_demo_circuit(gt7_state_t *state);
void gt7_update(gt7_client_t *client, gt7_state_t *state, uint32_t now_ms);
void gt7_update_demo(gt7_state_t *state, uint32_t now_ms, int theme_index);
bool gt7_decrypt_packet(const uint8_t *raw, size_t len, uint8_t *decrypted_out);
void gt7_close(gt7_client_t *client);

#ifdef __cplusplus
}
#endif

#endif /* GT7_TELEMETRY_H */
