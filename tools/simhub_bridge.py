#!/usr/bin/env python3
"""
PC SimHub -> Nintendo Switch / Host Telemetry UDP Bridge & Simulator
====================================================================
Supports 3 operating modes:
  1. Direct SimHub Custom UDP / JSON Template generator (`--template`)
  2. Live SimHub Web/HTTP API poller -> Switch UDP relay (`--simhub-host 127.0.0.1:8888 --switch-ip <SWITCH_IP>`)
  3. 60Hz Telemetry Simulator (`--simulate --switch-ip <SWITCH_IP> --format json|forza|codemasters`)

Usage Examples:
  # Print the NCalc / JSON string to paste into SimHub -> Custom UDP / Serial:
  python3 tools/simhub_bridge.py --template

  # Relay live PC SimHub telemetry (from SimHub Web Server port 8888) to Switch IP:
  python3 tools/simhub_bridge.py --switch-ip 192.168.1.50 --port 20777

  # Send 60Hz simulated PC SimHub telemetry (JSON, Forza 311B, or Codemasters 264B) to test HUD:
  python3 tools/simhub_bridge.py --simulate --switch-ip 127.0.0.1 --port 20777 --format json
"""

import argparse
import json
import math
import socket
import struct
import time
import urllib.request

SIMHUB_NCALC_TEMPLATE = """{
  "rpm": [Rpms],
  "max_rpm": [MaxRpm],
  "redline": [Redline],
  "speed_kmh": [SpeedKmh],
  "gear": "[Gear]",
  "sug_gear": [SuggestedGear],
  "throttle": [Throttle],
  "brake": [Brake],
  "clutch": [Clutch],
  "steer_deg": [SteerAngle],
  "fuel": [Fuel],
  "fuel_cap": [MaxFuel],
  "water_temp": [WaterTemperature],
  "oil_temp": [OilTemperature],
  "oil_press": [OilPressure],
  "boost": [Turbo],
  "lap": [CurrentLap],
  "total_laps": [TotalLaps],
  "pos": [Position],
  "cars": [OpponentsCount],
  "cur_lap_ms": "[CurrentLapTime]",
  "last_lap_ms": "[LastLapTime]",
  "best_lap_ms": "[BestLapTime]",
  "delta_s": [DeltaToSessionBest],
  "tyre_temp_fl": [TyreTemperatureFrontLeft],
  "tyre_temp_fr": [TyreTemperatureFrontRight],
  "tyre_temp_rl": [TyreTemperatureRearLeft],
  "tyre_temp_rr": [TyreTemperatureRearRight],
  "slip_fl": [TyreSlipFrontLeft],
  "slip_fr": [TyreSlipFrontRight],
  "slip_rl": [TyreSlipRearLeft],
  "slip_rr": [TyreSlipRearRight],
  "susp_fl": [SuspensionFrontLeft],
  "susp_fr": [SuspensionFrontRight],
  "susp_rl": [SuspensionRearLeft],
  "susp_rr": [SuspensionRearRight],
  "sway": [AccelerationSway],
  "heave": [AccelerationHeave],
  "surge": [AccelerationSurge],
  "pos_x": [PositionX],
  "pos_y": [PositionY],
  "pos_z": [PositionZ],
  "tc_active": [TCActive],
  "abs_active": [ABSActive],
  "ers_pct": [ERSPercent],
  "ers_power": [ERSRecovery]
}"""


def build_simulated_frame(t: float) -> dict:
    lap_time_s = t % 84.5
    lap = int(t // 84.5) + 1
    speed_kmh = 165.0 + 115.0 * math.sin(t * 0.65)
    rpm = 4200.0 + 4400.0 * (0.5 + 0.5 * math.sin(t * 1.9))
    gear = max(1, min(7, int(speed_kmh // 42) + 1))
    throttle = max(0.0, min(100.0, 60.0 + 55.0 * math.sin(t * 1.3)))
    brake = max(0.0, min(100.0, -75.0 * math.sin(t * 1.3)))
    steer_deg = 125.0 * math.sin(t * 0.9)
    return {
        "rpm": round(rpm, 1),
        "max_rpm": 9000,
        "redline": 8200,
        "speed_kmh": round(speed_kmh, 2),
        "gear": str(gear),
        "sug_gear": max(1, min(6, gear - (1 if brake > 35 else 0))),
        "throttle": round(throttle, 1),
        "brake": round(brake, 1),
        "clutch": 0.0,
        "steer_deg": round(steer_deg, 1),
        "fuel": round(max(5.0, 68.0 - t * 0.04), 2),
        "fuel_cap": 100.0,
        "water_temp": 88.5,
        "oil_temp": 102.4,
        "oil_press": 5.2,
        "boost": round(0.8 * (throttle / 100.0), 2),
        "lap": lap,
        "total_laps": 15,
        "pos": 2,
        "cars": 20,
        "cur_lap_ms": int(lap_time_s * 1000),
        "last_lap_ms": 83942,
        "best_lap_ms": 83418,
        "tyre_temp_fl": round(86.0 + 6.0 * math.sin(t * 0.5), 1),
        "tyre_temp_fr": round(88.0 + 6.0 * math.cos(t * 0.5), 1),
        "tyre_temp_rl": round(84.0 + 5.0 * math.sin(t * 0.4), 1),
        "tyre_temp_rr": round(85.0 + 5.0 * math.cos(t * 0.4), 1),
        "sway": round(1.65 * math.sin(t * 0.9), 3),
        "heave": round(0.18 * math.sin(t * 3.1), 3),
        "surge": round((throttle - brake) * 0.014, 3),
        "pos_x": round(480.0 * math.cos(t * 0.12), 2),
        "pos_y": round(8.0 * math.sin(t * 0.24), 2),
        "pos_z": round(340.0 * math.sin(t * 0.12), 2),
        "ers_pct": round(72.0 + 20.0 * math.sin(t * 0.25), 1),
        "ers_power": round((throttle - brake) * 0.85, 2),
    }


def _encode_forza_gear(g_val) -> int:
    s = str(g_val).strip().upper()
    if s in ("R", "-1"):
        return 0
    if s in ("N", "P", "0", ""):
        return 11
    try:
        gi = int(float(s))
        if gi < 0:
            return 0
        if gi == 0 or gi >= 11:
            return 11
        return gi
    except ValueError:
        return 11


def _pack_forza_common(d: dict, total_len: int, dash_off: int) -> bytes:
    buf = bytearray(total_len)
    struct.pack_into("<i", buf, 0x000, 1)  # IsRaceOn = 1
    struct.pack_into("<I", buf, 0x004, int(time.time() * 1000) & 0xFFFFFFFF)
    struct.pack_into("<f", buf, 0x008, float(d["max_rpm"]))
    struct.pack_into("<f", buf, 0x00C, 850.0)
    struct.pack_into("<f", buf, 0x010, float(d["rpm"]))
    struct.pack_into("<f", buf, 0x014, float(d["sway"]) * 9.80665)
    struct.pack_into("<f", buf, 0x018, float(d["heave"]) * 9.80665)
    struct.pack_into("<f", buf, 0x01C, float(d["surge"]) * 9.80665)
    spd_ms = float(d["speed_kmh"]) / 3.6
    struct.pack_into("<f", buf, 0x028, spd_ms)

    # Sled Suspension (0x44), Slip Ratio (0x54), Wheel Speed rad/s (0x64), Combined Slip (0xB4)
    w_rad = spd_ms / 0.33 if spd_ms > 0.1 else 0.0
    for w, (key_s, key_h) in enumerate([
        ("slip_fl", "susp_fl"),
        ("slip_fr", "susp_fr"),
        ("slip_rl", "susp_rl"),
        ("slip_rr", "susp_rr"),
    ]):
        norm_susp = float(d.get(key_h, 0.55))
        if norm_susp > 1.5:
            norm_susp = max(0.05, min(1.0, (norm_susp - 25.0) / 65.0))
        slip = float(d.get(key_s, 0.04))
        struct.pack_into("<f", buf, 0x044 + w * 4, norm_susp)
        struct.pack_into("<f", buf, 0x054 + w * 4, slip)
        struct.pack_into("<f", buf, 0x064 + w * 4, w_rad * (1.0 + slip))
        struct.pack_into("<f", buf, 0x0B4 + w * 4, slip)

    struct.pack_into("<i", buf, 0x0D4, 1420)  # CarOrdinal
    struct.pack_into("<i", buf, 0x0D8, 5)     # CarClass: 5 = S2
    struct.pack_into("<i", buf, 0x0E0, 2)     # Drivetrain: 2 = AWD

    struct.pack_into("<f", buf, dash_off + 0x00, float(d["pos_x"]))
    struct.pack_into("<f", buf, dash_off + 0x04, float(d["pos_y"]))
    struct.pack_into("<f", buf, dash_off + 0x08, float(d["pos_z"]))
    struct.pack_into("<f", buf, dash_off + 0x0C, spd_ms)
    struct.pack_into("<f", buf, dash_off + 0x10, float(d["ers_power"]) * 1000.0)
    struct.pack_into("<f", buf, dash_off + 0x18, float(d["tyre_temp_fl"]) * 9.0 / 5.0 + 32.0)
    struct.pack_into("<f", buf, dash_off + 0x1C, float(d["tyre_temp_fr"]) * 9.0 / 5.0 + 32.0)
    struct.pack_into("<f", buf, dash_off + 0x20, float(d["tyre_temp_rl"]) * 9.0 / 5.0 + 32.0)
    struct.pack_into("<f", buf, dash_off + 0x24, float(d["tyre_temp_rr"]) * 9.0 / 5.0 + 32.0)
    struct.pack_into("<f", buf, dash_off + 0x28, float(d["boost"]) * 14.5038)
    struct.pack_into("<f", buf, dash_off + 0x2C, float(d["fuel"]) / 100.0)
    struct.pack_into("<f", buf, dash_off + 0x34, float(d["best_lap_ms"]) / 1000.0)
    struct.pack_into("<f", buf, dash_off + 0x38, float(d["last_lap_ms"]) / 1000.0)
    struct.pack_into("<f", buf, dash_off + 0x3C, float(d["cur_lap_ms"]) / 1000.0)
    struct.pack_into("<H", buf, dash_off + 0x44, max(0, int(d["lap"]) - 1))
    buf[dash_off + 0x46] = int(d["pos"])
    buf[dash_off + 0x47] = int(float(d["throttle"]) * 2.55)
    buf[dash_off + 0x48] = int(float(d["brake"]) * 2.55)
    buf[dash_off + 0x49] = 0
    buf[dash_off + 0x4B] = _encode_forza_gear(d["gear"])
    steer_i8 = max(-127, min(127, int(float(d["steer_deg"]) / 450.0 * 127.0)))
    struct.pack_into("<b", buf, dash_off + 0x4C, steer_i8)
    return bytes(buf)


def pack_forza_311(d: dict) -> bytes:
    return _pack_forza_common(d, 311, 232)


def pack_forza_324(d: dict) -> bytes:
    return _pack_forza_common(d, 324, 244)


def pack_codemasters_264(d: dict) -> bytes:
    buf = bytearray(264)
    spd_ms = float(d["speed_kmh"]) / 3.6
    struct.pack_into("<f", buf, 0x00, float(d["cur_lap_ms"]) / 1000.0 + 60.0)
    struct.pack_into("<f", buf, 0x04, float(d["cur_lap_ms"]) / 1000.0)
    struct.pack_into("<f", buf, 0x10, float(d["pos_x"]))
    struct.pack_into("<f", buf, 0x14, float(d["pos_y"]))
    struct.pack_into("<f", buf, 0x18, float(d["pos_z"]))
    struct.pack_into("<f", buf, 0x1C, spd_ms)
    struct.pack_into("<f", buf, 0x74, float(d["throttle"]) / 100.0)
    struct.pack_into("<f", buf, 0x78, float(d["steer_deg"]) / 450.0)
    struct.pack_into("<f", buf, 0x7C, float(d["brake"]) / 100.0)
    struct.pack_into("<f", buf, 0x80, 0.0)
    struct.pack_into("<f", buf, 0x84, float(d["gear"]))
    struct.pack_into("<f", buf, 0x88, float(d["sway"]))
    struct.pack_into("<f", buf, 0x8C, float(d["surge"]))
    struct.pack_into("<f", buf, 0x90, float(d["lap"] - 1))
    struct.pack_into("<f", buf, 0x94, float(d["rpm"]) / 10.0)
    struct.pack_into("<f", buf, 0x9C, float(d["pos"]))
    struct.pack_into("<f", buf, 0xB4, float(d["fuel"]))
    struct.pack_into("<f", buf, 0xB8, float(d["fuel_cap"]))
    struct.pack_into("<f", buf, 0xDC, float(d["tyre_temp_rl"]))
    struct.pack_into("<f", buf, 0xE0, float(d["tyre_temp_rr"]))
    struct.pack_into("<f", buf, 0xE4, float(d["tyre_temp_fl"]))
    struct.pack_into("<f", buf, 0xE8, float(d["tyre_temp_fr"]))
    struct.pack_into("<f", buf, 0xF8, float(d["last_lap_ms"]) / 1000.0)
    struct.pack_into("<f", buf, 0xFC, float(d["max_rpm"]) / 10.0)
    struct.pack_into("<f", buf, 0x104, float(d["total_laps"]))
    return bytes(buf)


def pack_outgauge_96(d: dict) -> bytes:
    buf = bytearray(96)
    struct.pack_into("<I", buf, 0x00, int(time.time() * 1000) & 0xFFFFFFFF)
    buf[0x04:0x08] = b"GT3\x00"
    struct.pack_into("<H", buf, 0x08, (1 << 13) | (1 << 14) | (1 << 15))
    buf[0x0A] = int(d["gear"]) + 1
    buf[0x0B] = 1
    struct.pack_into("<f", buf, 0x0C, float(d["speed_kmh"]) / 3.6)
    struct.pack_into("<f", buf, 0x10, float(d["rpm"]))
    struct.pack_into("<f", buf, 0x14, float(d["boost"]))
    struct.pack_into("<f", buf, 0x18, float(d["water_temp"]))
    struct.pack_into("<f", buf, 0x1C, float(d["fuel"]) / 100.0)
    struct.pack_into("<f", buf, 0x20, float(d["oil_press"]))
    struct.pack_into("<f", buf, 0x24, float(d["oil_temp"]))
    struct.pack_into("<I", buf, 0x28, 0xFFFFFFFF)
    struct.pack_into("<I", buf, 0x2C, (1 << 4) if float(d["throttle"]) > 85.0 else 0)
    struct.pack_into("<f", buf, 0x30, float(d["throttle"]) / 100.0)
    struct.pack_into("<f", buf, 0x34, float(d["brake"]) / 100.0)
    struct.pack_into("<f", buf, 0x38, float(d["clutch"]) / 100.0)
    return bytes(buf)


def main():
    parser = argparse.ArgumentParser(description="PC SimHub / Forza Horizon -> Switch Telemetry UDP Bridge & Simulator")
    parser.add_argument("--template", action="store_true", help="Print SimHub Custom UDP JSON template and exit")
    parser.add_argument("--simulate", action="store_true", help="Send 60Hz simulated telemetry packets")
    parser.add_argument("--listen-udp", type=int, default=0, help="Local UDP port to listen on (e.g. 8000 for Forza) and forward to Switch")
    parser.add_argument("--switch-ip", default="127.0.0.1", help="Target Nintendo Switch or Host IP address")
    parser.add_argument("--port", type=int, default=20777, help="Target UDP port (20777, 8000, or 33740)")
    parser.add_argument("--format", choices=["json", "forza", "forza324", "codemasters", "outgauge"], default="json", help="UDP packet format")
    parser.add_argument("--duration", type=float, default=0.0, help="Stop after N seconds (0 = infinite)")
    parser.add_argument("--simhub-http", default="http://127.0.0.1:8888/api/getgamedata", help="SimHub HTTP API URL")
    args = parser.parse_args()

    if args.template:
        print("=== Paste this template into PC SimHub -> Custom UDP / Serial Output ===")
        print(SIMHUB_NCALC_TEMPLATE)
        return

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    start_t = time.time()
    sent = 0

    if args.listen_udp > 0:
        rx_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        rx_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        rx_sock.bind(("0.0.0.0", args.listen_udp))
        rx_sock.settimeout(0.25)
        print(f"[SimHub Bridge] Listening on local UDP :{args.listen_udp} -> Forwarding to {args.switch_ip}:{args.port}...")
        try:
            while True:
                if args.duration > 0.0 and (time.time() - start_t) >= args.duration:
                    break
                try:
                    data, _ = rx_sock.recvfrom(4096)
                    if data:
                        sock.sendto(data, (args.switch_ip, args.port))
                        sent += 1
                except socket.timeout:
                    continue
        except KeyboardInterrupt:
            pass
        finally:
            rx_sock.close()
            sock.close()
            print(f"[SimHub Bridge] Done. Forwarded {sent} packets.")
        return

    print(f"[SimHub Bridge] Sending {args.format.upper()} telemetry to {args.switch_ip}:{args.port} at 60Hz...")
    last_warn_t = 0.0

    try:
        while True:
            now = time.time()
            elapsed = now - start_t
            if args.duration > 0.0 and elapsed >= args.duration:
                break

            if args.simulate:
                frame = build_simulated_frame(elapsed)
            else:
                try:
                    with urllib.request.urlopen(args.simhub_http, timeout=0.25) as resp:
                        raw = json.loads(resp.read().decode("utf-8", errors="ignore"))
                        # SimHub HTTP API uses lowercase "newData" (or "NewData" in older plugins)
                        props = raw.get("newData") or raw.get("NewData") or raw
                        raw_data = props.get("RawData") or {}

                        def _pick(keys, default=0.0):
                            for k in keys:
                                v = props.get(k)
                                if v is not None:
                                    return v
                                v = raw_data.get(k)
                                if v is not None:
                                    return v
                            return default

                        frame = {
                            "rpm": _pick(["Rpms", "CurrentEngineRpm", "EngineRpm"], 0.0),
                            "max_rpm": _pick(["MaxRpm", "EngineMaxRpm", "CarSettings_MaxRPM"], 8500.0),
                            "redline": _pick(["Redline", "CarSettings_CurrentGearRedLineRPM"], 7800.0),
                            "speed_kmh": _pick(["SpeedKmh"], 0.0),
                            "gear": str(_pick(["Gear", "EngineGear"], "N")),
                            "sug_gear": _pick(["SuggestedGear"], 0),
                            "throttle": _pick(["Throttle"], 0.0),
                            "brake": _pick(["Brake"], 0.0),
                            "clutch": _pick(["Clutch"], 0.0),
                            "steer_deg": _pick(["SteerAngle", "Steer"], 0.0),
                            "fuel": _pick(["FuelPercent", "Fuel"], 50.0),
                            "fuel_cap": _pick(["MaxFuel"], 100.0),
                            "water_temp": _pick(["WaterTemperature"], 85.0),
                            "oil_temp": _pick(["OilTemperature"], 95.0),
                            "oil_press": _pick(["OilPressure"], 4.5),
                            "boost": _pick(["Turbo", "Boost"], 0.0),
                            "lap": _pick(["CurrentLap", "LapNumber"], 1),
                            "total_laps": _pick(["TotalLaps"], 0),
                            "pos": _pick(["Position", "RacePosition"], 1),
                            "cars": _pick(["OpponentsCount"], 20),
                            "cur_lap_ms": _pick(["CurrentLapTime"], 0),
                            "last_lap_ms": _pick(["LastLapTime"], 0),
                            "best_lap_ms": _pick(["BestLapTime"], 0),
                            "tyre_temp_fl": _pick(["TyreTemperatureFrontLeft", "TireTempFrontLeft", "TireTempFl"], 80.0),
                            "tyre_temp_fr": _pick(["TyreTemperatureFrontRight", "TireTempFrontRight", "TireTempFr"], 80.0),
                            "tyre_temp_rl": _pick(["TyreTemperatureRearLeft", "TireTempRearLeft", "TireTempRl"], 80.0),
                            "tyre_temp_rr": _pick(["TyreTemperatureRearRight", "TireTempRearRight", "TireTempRr"], 80.0),
                            "slip_fl": _pick(["TyreSlipFrontLeft", "TireSlipRatioFrontLeft", "TireCombinedSlipFrontLeft"], 0.0),
                            "slip_fr": _pick(["TyreSlipFrontRight", "TireSlipRatioFrontRight", "TireCombinedSlipFrontRight"], 0.0),
                            "slip_rl": _pick(["TyreSlipRearLeft", "TireSlipRatioRearLeft", "TireCombinedSlipRearLeft"], 0.0),
                            "slip_rr": _pick(["TyreSlipRearRight", "TireSlipRatioRearRight", "TireCombinedSlipRearRight"], 0.0),
                            "susp_fl": _pick(["SuspensionFrontLeft", "NormalizedSuspensionTravelFrontLeft"], 0.55),
                            "susp_fr": _pick(["SuspensionFrontRight", "NormalizedSuspensionTravelFrontRight"], 0.55),
                            "susp_rl": _pick(["SuspensionRearLeft", "NormalizedSuspensionTravelRearLeft"], 0.55),
                            "susp_rr": _pick(["SuspensionRearRight", "NormalizedSuspensionTravelRearRight"], 0.55),
                            "sway": _pick(["AccelerationSway"], 0.0),
                            "heave": _pick(["AccelerationHeave"], 0.0),
                            "surge": _pick(["AccelerationSurge"], 0.0),
                            "pos_x": _pick(["PositionX"], 0.0),
                            "pos_y": _pick(["PositionY"], 0.0),
                            "pos_z": _pick(["PositionZ"], 0.0),
                            "ers_pct": _pick(["ERSPercent"], 78.0),
                            "ers_power": _pick(["ERSRecovery"], 0.0),
                        }
                except Exception as exc:
                    if now - last_warn_t >= 3.0:
                        print(f"[SimHub Bridge] Waiting for SimHub HTTP API ({args.simhub_http}): {exc}")
                        last_warn_t = now
                    time.sleep(0.25)
                    continue

            if args.format == "json":
                payload = json.dumps(frame).encode("utf-8")
            elif args.format == "forza":
                payload = pack_forza_311(frame)
            elif args.format == "forza324":
                payload = pack_forza_324(frame)
            elif args.format == "outgauge":
                payload = pack_outgauge_96(frame)
            else:
                payload = pack_codemasters_264(frame)

            sock.sendto(payload, (args.switch_ip, args.port))
            sent += 1
            time.sleep(1.0 / 60.0)
    except KeyboardInterrupt:
        pass
    finally:
        sock.close()
        print(f"[SimHub Bridge] Done. Sent {sent} packets.")


if __name__ == "__main__":
    main()
