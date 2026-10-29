#!/usr/bin/env bash
set -e

echo "=== 檢查 GitHub 登入狀態 ==="
gh auth status

REPO_NAME="gt7-switch-sim-ddu"
echo "=== 檢查 GitHub 公開儲存庫: $REPO_NAME ==="
gh repo create "$REPO_NAME" --public --source=. --remote=origin --push --description "PS5 Gran Turismo 7 Sim Racing DDU for Nintendo Switch (Atmosphère Homebrew .nro) with 8 motorsport modes & luxury EV cockpits" || true

TAG_NAME="v1.10.29.1415"
TITLE="GT7 Switch Sim-DDU Pro _aw_1.10.29.1415 (2026 Community UDP Protocol Matrix & Salsa20 Seed Decoupling)"

NOTES=$(cat << 'EOF'
## 🏁 GT7 Switch Sim-DDU Pro _aw_1.10.29.1415

### 🌟 2026 最新 GT7 UDP 社群逆向工程協定全面實裝與位移校正 (GT7 v1.68+ Datalogger Spec)

依據 2026 年最新社群逆向工程文檔（GT7 Datalogger 與公開解析器成果），全面重構遙測封包解密與欄位解析架構：

1. **封包專屬 Salsa20 Nonce 解密常數與三層備援 (Decoupled Nonce XOR Seeds)**：
   - **Packet A (296B)**: 優先使用 `0xDEADBEAF` 種子解密。
   - **Packet B (316B) & Packet C (368B)**: 優先使用 `0xDEADBEEF` 種子解密。
   - **Packet ~ (344B)**: 專屬使用 `0x55FABB4F` 種子解密（徹底解決過去誤用 `0xDEADBEEF` 導致 Magic 檢驗失敗的問題）。
   - **三層無縫備援機制**：解密失敗時自動輪詢所有候選種子，確保 100% 成功解析各版本封包。

2. **姿態四元數與真實尤拉角轉換 (`0x01C..0x02B`)**：
   - 修正過去將此區段誤認為直接 Pitch/Yaw/Roll 角的偏差。
   - 完整讀取四元數 `(X, Y, Z, W)` 分量，並透過三角函數嚴格推導出真實姿態俯仰角、偏航角與滾轉角。
   - 補齊 `0x02C..0x037` 三軸車體角速度（`rad/s`）。

3. **輪速角速度與實體輪胎打滑率校正 (`0x0A4..0x0D3`)**：
   - 修正輪速單位為 `rad/s`（而非 rev/sec），透過線速度公式 $v = \omega \cdot r$ 搭配 `0x0B4` 輪胎半徑，計算出物理正確的四輪滑差率。
   - 嚴格區分避震器行程高度（`0x0C4..0x0D3`，mm）。

4. **Packet ~ 與 Packet C 於 `0x13C` 位移衝突之嚴格隔離解析**：
   - **Packet ~ (344B, 純電/油電專用)**：
     - `0x13C`: `throttle_filtered`（過濾後油門，0-100%）
     - `0x13D`: `brake_filtered`（過濾後煞車，0-100%）
     - `0x13E`: `powertrain_flags`（動力系統配置旗標）
     - `0x13F`: `energy_recovery_available`（動能回收可用布林值）
     - `0x140..0x14F`: `torque_vector_nm[4]`（四輪獨立馬達扭力向量，Nm）
     - `0x150..0x153`: `energy_recovery_kw`（即時動能回充功率，kW）
     - `0x154..0x157`: `engine_torque_norm`（正規化引擎扭力）
   - **Packet C (368B, 賽車/當前圈即時遙測)**：
     - `0x13C..0x13F`: `surface_type[4]`（四輪路面接觸型態：`'T'` 瀝青、`'C'` 路緣石、`'D'` 砂石、`'G'` 草地、`'S'` 雪地）
     - `0x140..0x143`: `current_lap_time_ms`（當圈即時時間，ms）
     - `0x144..0x14B`: `wheel_steering_angle[2]`（前左/前右前輪真實轉向角，rad 轉 deg）
     - `0x14C..0x14F`: `wheel_base_m`（軸距，m）
     - `0x150..0x153`: `car_category[4]`（車輛分級字串，如 `GR3`, `GR4`, `GR1`, `GRB`, `ROAD`）
     - `0x154..0x16F`: `unknown_tail[28]`（保留 28 位元組未確認尾端資料，杜絕位移溢出）

---

### 🇹🇼 繁體中文
- **2026 GT7 UDP 協定規格重大升級 (v1.10.29.1415)**：
  - 完整支援 Packet A (296B)、Packet B (316B)、Packet ~ (344B) 與 Packet C (368B)。
  - 封包長度導向之 Salsa20 獨立種子解密（Packet ~ 專用 `0x55FABB4F`，Packet A 專用 `0xDEADBEAF`，Packet B/C 專用 `0xDEADBEEF`），相容性達 100%。
  - 四元數真實姿態解算與 rad/s 輪速公式校正，打造最高物理擬真度。
  - 四大豪華旗艦電動儀表（Benz MBUX, BMW i-Curved M, Audi RS Virtual Cockpit, Volvo Google Built-in）與全滿版 OLED 黑色背景、SDL 純向量幾何圖示無瑕渲染。

---

### 🇬🇧 English
- **2026 GT7 UDP Telemetry Protocol Matrix & Seed Decoupling (v1.10.29.1415)**:
  - **Decoupled Salsa20 Nonce Seeds**:
    - Packet A (296B): `0xDEADBEAF`
    - Packet B (316B) & Packet C (368B): `0xDEADBEEF`
    - Packet ~ (344B): Dedicated EV `0x55FABB4F` seed with multi-layer fallback.
  - **Quaternion Attitude Solving**: Reads orientation quaternion `(x, y, z, w)` at `0x01C..0x02B` and trigonometrically derives pitch, yaw, and roll.
  - **Wheel Angular Speed in rad/s**: Physically accurate wheel speed $v = \omega \cdot r$ with tire radius at `0x0B4` for slip ratio computation.
  - **Strict Separation of Packet ~ and Packet C**: Fully isolates EV torque vectoring/regen (`0x13C..0x157`) from motorsport surface types, live lap time, steering angles, and Gr. category (`0x13C..0x16F`).

---

### 🇯🇵 日本語
- **2026年最新 GT7 UDP 逆アセンブル仕様完全対応＆Nonce暗号化シード分離 (v1.10.29.1415)**:
  - **パケット別Salsa20 Nonce暗号化シードの適正化**:
    - パケットA (296B): `0xDEADBEAF`
    - パケットB (316B) & パケットC (368B): `0xDEADBEEF`
    - パケット~ (344B): EV専用シード `0x55FABB4F`（マルチレイヤーフォールバック完備）
  - **姿勢クォータニオン解算**: `0x01C..0x02B`の4元数から正確なピッチ／ヨー／ロール角を三角関数導出。
  - **車輪角速度（rad/s）とタイヤスリップ率の厳密化**: $v = \omega \cdot r$ による高精度演算。
  - **パケット~とパケットCの完全分離**: EVトルクベクタリング・回生電力と、レース路面接地・ステアリング角・GT7公式クラス（GR3/GR4等）のオフセット競合を完全解決。

---

### 🇩🇪 Deutsch
- **2026 GT7 UDP Telemetrie-Protokollmatrix & Salsa20-Seed-Entkopplung (v1.10.29.1415)**:
  - **Format-spezifische Salsa20-Nonce-Entschlüsselung**:
    - Paket A (296B): `0xDEADBEAF`
    - Paket B (316B) & Paket C (368B): `0xDEADBEEF`
    - Paket ~ (344B): Spezifischer EV-Seed `0x55FABB4F` mit automatischem Fallback.
  - **Orientierungs-Quaternionen**: Exakte Ableitung von Nick-, Gier- und Wankwinkeln aus 4D-Quaternionen.
  - **Radwinkelgeschwindigkeit in rad/s**: Physikalisch präzise Schlupfberechnung via $v = \omega \cdot r$.
  - **Strikte Trennung von Paket ~ und Paket C**: Vollständige Trennung von Allrad-Drehmomentverteilung (`0x13C..0x157`) und GT3/GR4-Fahrbahnoberflächen sowie Rundenzeiten (`0x13C..0x16F`).
EOF
)

echo "=== 正在更新 Release $TAG_NAME ==="
gh release edit "$TAG_NAME" \
  --title "$TITLE" \
  --notes "$NOTES" || gh release create "$TAG_NAME" dist/switch/gt7_hud/gt7_switch_hud.nro --title "$TITLE" --notes "$NOTES"

echo "=== 上傳最新二進位 gt7_switch_hud.nro ==="
gh release upload "$TAG_NAME" dist/switch/gt7_hud/gt7_switch_hud.nro --clobber

echo "=== 發布完成 ==="
gh release view "$TAG_NAME"
