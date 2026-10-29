#!/usr/bin/env bash
set -e

echo "=== 檢查 GitHub 登入狀態 ==="
gh auth status

REPO_NAME="gt7-switch-sim-ddu"
echo "=== 檢查 GitHub 公開儲存庫: $REPO_NAME ==="
gh repo create "$REPO_NAME" --public --source=. --remote=origin --push --description "PS5 Gran Turismo 7 Sim Racing DDU for Nintendo Switch (Atmosphère Homebrew .nro) with 8 motorsport modes & luxury EV cockpits" || true

TAG_NAME="v1.10.30.0005"
TITLE="GT7 Switch Sim-DDU Pro _aw_1.10.30.0005 (Full Telemetry Coverage & Zero-Blank Across All 8 Themes & EV Modes)"

NOTES=$(cat << 'EOF'
## 🏁 GT7 Switch Sim-DDU Pro _aw_1.10.30.0005

### 🌟 全風格面板零空白優化與 2026 UDP 協定全功能覆蓋 (Zero-Blank & 100% Protocol Matrix Coverage)

針對全系統 8 大賽車主題模式與 4 款旗艦豪華電動車儀表（Mercedes-Benz MBUX EQ、BMW i-Curved M OS 8.5/9、Audi Sport RS Virtual Cockpit、Volvo EX90 Google Built-in）進行地毯式檢視與全滿版 OLED 黑色空間優化，確保所有 2026 最新封包欄位（Packet A, B, ~, C）在各風格中皆有專屬對應資訊卡，且排版零重疊、文字零截斷、邊界零溢出：

1. **Mode 1: Official GT7 Native HUD & MFD**:
   - 整合 Packet C 車輛分級標籤 (`[GR3]`, `[GR4]`, `[GR1]`, `[GRB]`, `[ROAD]`)
   - 四輪路面接觸型態即時徽章 (`[T]` 瀝青, `[C]` 路緣石, `[D]` 砂石, `[G]` 草地, `[S]` 雪地) 與四輪打滑率 (Slip Ratio)
   - 交互式 4 頁切換 MFD 賽道雷達、即時 S1/S2/S3 分段區間時速與時間。

2. **Mode 2: High-Resolution DigiTach Motorsport DDU**:
   - 填補中柱踏板下方之空白區域，建立專屬「車輛動態與底盤遙測卡」
   - 納入 3 軸加速度與慣性數據（G-Load, Sway, Heave, Surge）、前輪真實轉向角（`STEER %+.1f DEG`）、車身離地高度（`RIDE %.0fMM`）以及動能回充／下壓力（`AERO / HYBRID`）與最速紀錄。

3. **Mode 3: Luxury EV Digital Cockpit (Benz, BMW, Audi, Volvo)**:
   - **Mercedes-Benz MBUX EQ**: 左右雙類比大圓（直徑佔滿螢幕高度）、"From Reset" 航程資訊、4MATIC AWD 扭力分配卡，並新增四輪 TPMS 胎溫與路面接觸型態即時監測。
   - **BMW i-Curved M Display**: 對稱式雙箭頭外側翼新增左右獨立 TPMS 胎溫卡（FL/RL 與 FR/RR）、M xDrive AWD 扭力分配及前輪轉向角。
   - **Audi Sport RS Virtual Cockpit**: 跑道式階梯轉速表下新增 RS Quattro e-tron AWD 功率與扭力矩陣、專屬 RS 四輪胎溫卡及路面接觸狀態。
   - **Volvo EX90 Google Built-in**: 3D 俯視導航地圖左下方新增 Trip 圈數名次與四輪胎溫及全時四驅分配。

4. **Mode 4: F1 W14 / Super Formula Wheel Display**:
   - 左側 Timing 框加入 S1/S2/S3 分段區間時間。
   - 中柱頂部顯示車輛分級與前輪轉向角，優化 RPM 字級徹底杜絕重疊。
   - 右欄擴增 F1 ERS / Hybrid Energy Deployment 充放電進度條與 MGU-K 即時功率。

5. **Mode 5: GT3 MoTeC C1212 Endurance Dash**:
   - 重構右欄「ENDURANCE FUEL & VITALS」為雙欄精準對稱排版：獨立顯示油量、剩餘圈數、渦輪增壓、水溫、機油油溫油壓、變速箱油溫、尾速、下壓力與車身側傾（SWAY），完美錯開下方 TC / ABS 警示按鈕。

6. **Mode 6: WRC Rally Gr.B Stage Cockpit**:
   - 左側渦輪錶加入 Peak 峰值增壓、ALS 偏時點火狀態與目前賽段路面類型標籤。
   - 中柱巨型檔位下方顯示前輪轉向角與 4WD 扭力分配（AWD 50:50）。
   - 右側傾角儀面板補齊 3 軸衝擊加速度（SURGE, HEAVE, SWAY）與轉向角速度。

7. **Mode 7: JDM Defi Analog Cluster**:
   - 左側 Defi 增壓錶補齊 Peak 峰值保持與負壓抽氣數值。
   - 中柱指針轉速表頂部配置前輪轉向角與 ROAD SPEC 標籤。
   - 右側 Advance ZD 多功能儀表加入即時單圈 Delta 與極速紀錄。

8. **Mode 8: Track Info & Race Engineer Mode**:
   - 頂部遙測摘要條精簡排版並納入前輪轉向角與車輛分級徽章。
   - 底部進站策略計算欄位整合平均胎溫、路面接觸與側傾加速度。

---

### 🇹🇼 繁體中文
- **全風格面板零空白與協定功能全面實裝 (v1.10.30.0005)**：
  - 8 大主題與 4 款旗艦電車儀表全數優化，徹底消除空白區域。
  - 純黑色 OLED (`#000000`) 高對比渲染，字型與介面元件無遮擋、無重疊。
  - 支援 Packet A, B, ~, C 之所有已知 2026 遙測欄位（四元數姿態、四輪胎面型態、前輪轉向角、動能回充、扭力分配、軸距與分級）。

---

### 🇬🇧 English
- **Full Telemetry Coverage & Zero-Blank Across All 8 Themes & EV Modes (v1.10.30.0005)**:
  - System-wide optimization across all 8 racing themes and 4 luxury EV cockpits (Benz, BMW, Audi, Volvo).
  - All blank areas populated with high-value telemetry: 3-axis acceleration (Sway, Heave, Surge), contact patch surfaces (`[T]`, `[C]`, `[D]`, `[G]`, `[S]`), steering angles, aero downforce, and hybrid kW.
  - Zero text overlap and pure OLED `#000000` background verified across every single screen.

---

### 🇯🇵 日本語
- **全8テーマ＆EVコクピットの空白ゼロ最適化とテレメトリ全機能実装 (v1.10.30.0005)**:
  - 全8種のレーシングテーマおよび4種の高級EVコクピット（Benz、BMW、Audi、Volvo）を徹底検証。
  - すべての余白スペースに最新パケット（A/B/~/C）の有益なテレメトリ（3軸加速度、路面接地タイプ、前輪舵角、回生電力、エアロダウンフォース）を配置。
  - 完全なOLEDブラック背景と重なりのない美麗なフォントレイアウトを実現。

---

### 🇩🇪 Deutsch
- **Vollständige Telemetrie-Abdeckung & Null-Leerraum-Optimierung (v1.10.30.0005)**:
  - Umfassende Optimierung aller 8 Rennthemen und 4 Luxus-EV-Cockpits (Benz, BMW, Audi, Volvo).
  - Beseitigung aller ungenutzten Bildschirmbereiche durch gezielte Anzeige von 3-Achsen-Beschleunigung, Fahrbahnoberfläche, Lenkwinkel, Allrad-Drehmoment und ERS/Hybrid-Leistung.
  - Reines OLED-Schwarz (`#000000`) ohne Textüberlappungen auf allen 13 Bildschirmmodi.
EOF
)
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
