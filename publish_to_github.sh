#!/usr/bin/env bash
set -e

echo "=== 檢查 GitHub 登入狀態 ==="
gh auth status

REPO_NAME="gt7-switch-sim-ddu"
echo "=== 檢查 GitHub 公開儲存庫: $REPO_NAME ==="
gh repo create "$REPO_NAME" --public --source=. --remote=origin --push --description "PS5 Gran Turismo 7 Sim Racing DDU for Nintendo Switch (Atmosphère Homebrew .nro) with 8 motorsport modes & luxury EV cockpits" || true

TAG_NAME="v1.10.30.1350"
TITLE="GT7 Switch Sim-DDU Pro _aw_1.10.30.1350 (Authentic Full-Screen & Modern Vector Typography Overhaul for Benz, BMW, Audi, Volvo)"

NOTES=$(cat << 'EOF'
## 🏁 GT7 Switch Sim-DDU Pro _aw_1.10.30.1350

### 🌟 豪華旗艦電車儀表 100% 全滿版與現代原廠向量字體極致重構 (Authentic Full-Screen & OEM Vector Typography Overhaul)

針對使用者回饋與最新 Mercedes-Benz MBUX EQ、BMW i-Curved M (OS 8.5/9)、Audi Sport RS Virtual Cockpit、Volvo EX90 Google Built-in 實車儀表照片，進行全面視覺重構與細節對齊：

1. **全新原廠現代幾何向量字型引擎 (Modern Luxury Automotive Vector Typography)**：
   - 徹底捨棄復古 7 段 LED 數位傾斜字體，自主實作 `draw_modern_char` 與 `draw_modern_number_centered` 原廠級無襯線向量字型。
   - 筆畫粗細均勻（6~8px）、幾何圓角與倒角過渡平滑，完美還原各車廠最新數位儀表之高品質質感。

2. **Mercedes-Benz MBUX EQ (雙 492px 巨型類比圓錶 + 電幻地平線浪湧)**：
   - 雙圓直徑擴大至 492px，幾近貼合螢幕垂直高度，內部配置 72 道星芒放射線與高精度刻度。
   - 中間全面鋪設平滑漸層電幻地平線藍光（Electric Blue Horizon Wave）。
   - 頂部現代向量時速與 ADAS 駕駛輔助陣列（車距雷達、車道維持、雙手方向盤、限速 50）。
   - "From Reset" 航程卡精緻半透明磨砂深藍玻璃，呈現油量、均速、單圈時間、里程與綠色 ECO 綠葉圖示。
   - 右錶外側配置真實 MBUX 直列式 R/N/D 檔位柱、大型白色 P/D 檔位與 `[ C ]` / `[ S ]` 動態駕駛模式膠囊。
   - 下方配置 3 人座安全帶就座圖形卡與 4MATIC AWD 四驅扭力比、四輪胎溫及路面接地徽章。

3. **BMW i-Curved M Display (100% 全螢幕 3D 街區導航地圖 + 對稱式幾何雙箭頭)**：
   - 導航地圖全面擴展至 1280px 全螢幕邊緣（消除兩側黑邊與突兀黑框），支援透視路網、交錯市區大道與路徑航點。
   - 左右雙箭頭覆蓋半透明深藍結晶磨砂背板，左箭頭鑲嵌現代向量時速與 km/h 標籤；右箭頭鑲嵌現代大字檔位（P / D / R）、READY 0、即時功率（+481 kW）與傾斜 ADAPTIVE 駕駛模式。
   - 底部完整對齊原廠狀態列：總里程 465 km、時間 10:49、歐規圓形紅框速限 `(50)`、俯視車輛底盤圖標、車外溫度 `+ 19.0 °C` 與高壓電池電量條。

4. **Audi Sport RS Virtual Cockpit (RS 階梯跑道轉速表 + 雙圓形 Power/Torque 儀表)**：
   - 階梯跑道式轉速表（Runway Hockey-stick）尺寸大幅擴展，精準繪製斜坡 READY 1 2 3 與水平 4 5 6 7 8 轉速刻度及 7-8k 紅線區。
   - 彎角核心配置現代巨型向量時速，下方配置 RS 簽名檔位（R AUTO / D AUTO / S 6 DYNAMIC）。
   - 右側雙獨立圓形儀表：Power % 與 Torque % 具備外圈青綠飾環、內圈刻度與底部紅色指針，下方配置橫向分段 `MIN BOOST MAX` 增壓條與 RS QUATTRO e-tron AWD 性能卡。
   - 左側完整配置日期時間（`10:43 AM Wednesday 11.3.2026`）、機油溫度條與專屬 RS 四輪 TPMS 胎面卡。

5. **Volvo EX90 / Polestar (Google Built-in 100% 邊緣到邊緣全景導航 + 雙弧儀表)**：
   - 3D Google Maps 透視地圖橫跨 0 至 1280px，標註真實街道名稱（Granville St、Oak St、W 49th Ave、W 57th Ave）、青藍色 "Devonshire park" 公園多邊形與 Google 識別商標。
   - 左右緊貼螢幕外緣配置外拱形 0~200 時速弧與 Power/Charge 動能弧。
   - 雙懸浮圓角磨砂玻璃艙：左艙配置白底黑字方框速限 `[20]` 與現代向量時速；右艙配置綠色 READY、大型檔位 D、直列 R/N/D/-B+ 檔位柱與 power/charge 刻度。
   - 底部配置雙動態續航條（電池 264 km、燃油 356 km）與 Volvo Trip 遙測摘要。

---

### 🇹🇼 繁體中文
- **旗艦電車儀表 100% 全滿版與現代原廠向量字體重構 (v1.10.30.1350)**：
  - 4 款旗艦電車儀表全數消除兩側黑邊，BMW 與 Volvo 具備滿版 1280px 3D 導航透視地圖。
  - 導入專屬現代原廠無襯線向量數字（draw_modern_char），終結復古 LED 粗糙感。
  - Benz MBUX、BMW iDrive、Audi Virtual Cockpit、Volvo EX90 之檔位、功率、胎面、四驅分配完全契合實車邏輯。
  - 純黑色 OLED (`#000000`) 零文字重疊與全滿版沉浸式渲染。

---

### 🇬🇧 English
- **Authentic Full-Screen & Modern Vector Typography Overhaul for Benz, BMW, Audi, Volvo (v1.10.30.1350)**:
  - 100% full-bleed canvas utilization across all 4 luxury EV cockpits: edge-to-edge 1280px 3D navigation maps for BMW and Volvo, enlarged 492px dials with horizon glow for Benz, and expanded runway with dual circular pods for Audi.
  - Custom OEM modern automotive vector typography replacing 7-segment digital font with clean, proportional luxury aesthetics.
  - Authentic transmission selectors, live hybrid recovery/power kW, AWD torque splits, and surface contact badges (`[T]`, `[C]`, `[D]`, `[G]`, `[S]`).

---

### 🇯🇵 日本語
- **Benz、BMW、Audi、Volvoの全画面100%表示と最新OEMベクターフォント刷新 (v1.10.30.1350)**:
  - BMWおよびVolvoに横幅1280pxの全画面3Dナビゲーションマップを実装し、余白ブラックアウトを完全解消。
  - レトロな7セグメントLEDを廃止し、最新の高級車専用モダンベクターフォントを新設。
  - Benz MBUXの大径492pxメーター、Audi SportのRS滑走路タコメーター＆Power/Torque2連ポッド、VolvoのGoogle Built-inフロートガラスポッドを実車に忠実に再現。

---

### 🇩🇪 Deutsch
- **Authentische Vollbild- & moderne Vektor-Typografie-Überarbeitung für Benz, BMW, Audi, Volvo (v1.10.30.1350)**:
  - Vollständige 100% Bildschirmausnutzung (1280px) für BMW i-Curved und Volvo EX90 mit randlosem 3D-Kartennetzwerk.
  - Elegante moderne OEM-Vektor-Typografie anstelle veralteter 7-Segment-LED-Ziffern.
  - Präzise Abbildung der echten Getriebewahlhebel, Allrad-Drehmomentverteilung (4MATIC, xDrive, Quattro) und 4-Rad-TPMS-Reifenoberflächen.
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
