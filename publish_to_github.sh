#!/usr/bin/env bash
set -e

echo "=== 檢查 GitHub 登入狀態 ==="
gh auth status

REPO_NAME="gt7-switch-sim-ddu"
echo "=== 正在為您建立 GitHub 公開儲存庫: $REPO_NAME ==="
gh repo create "$REPO_NAME" --public --source=. --remote=origin --push --description "PS5 Gran Turismo 7 Sim Racing DDU for Nintendo Switch (Atmosphère Homebrew .nro) with 8 motorsport modes & luxury EV cockpits" || true

echo "=== 正在發布 Release 與上傳 gt7_switch_hud.nro ==="
gh release create "v1.10.29.0746" dist/switch/gt7_hud/gt7_switch_hud.nro \
  --title "GT7 Switch Sim-DDU Pro _aw_1.10.29.0746" \
  --notes "## 🏁 GT7 Switch Sim-DDU Pro _aw_1.10.29.0746

### 🇹🇼 繁體中文
- **豪華 EV 純電座艙深度優化 (模式 3)**：
  - **Mercedes-Benz MBUX EQ**：左右類比圓錶直徑擴大至 472px（貼近螢幕高度），外圈放射狀刻度、發光雷射指針、7段數位速度置中、PRND 檔位指示、右側 EQ 輸出/回生錶與 SoC 電量條、中央 MBUX 導航地圖。
  - **BMW i-Curved Display OS 8.5/9**：經典幾何多邊形對稱雙 Chevron 括弧線條、///M 專屬三色條紋、前傾刻度線、動態 ePOWER 輸出與回生導光帶、全油門時閃爍 \`>> eBOOST <<\`、中央 524px BMW Live Cockpit M 賽道與 G-Force。
  - **Audi Virtual Cockpit Plus RS e-tron GT**：直徑 472px 雙圓錶、RS 紅色外圈刻度、紅色運動指針、右側 RS Power 輸出與螢光綠回生充能區、中央 MMI 賽道圖與 S1/S2/S3 分段計時卡片。
  - **Volvo EX90 / Polestar**：斯堪地那維亞純黑極簡風格（純 OLED #000000）、大字速限與行車視角引導、Pilot Assist 狀態、左右對稱純電續航卡片與垂直 PWR/REGEN 能量流 Ribbon。
- **純 OLED 零破圖保證**：所有數值均有獨立底框與安全間距，經 3 次迭代截圖驗證，零文字重疊、零數值溢出。
- **全觸控操作**：支援 100% 電容觸控切換主題、手動輸入 IP、全螢幕切換。
- ** Switch 執行檔**：提供 ARMv8 原生 \`gt7_switch_hud.nro\`，放入 microSD 卡 \`switch/gt7_hud/\` 即可直接啟動！

---

### 🇬🇧 English
- **Luxury EV Cockpit High-Fidelity Refinements (Mode 3)**:
  - **Mercedes-Benz MBUX EQ**: Dial diameter enlarged to 472px (~screen height), radial graduation ticks, luminous laser needles, 7-segment digital speed, PRND gear pill, right power/charge meter, center MBUX navigation radar.
  - **BMW i-Curved Display OS 8.5/9**: Symmetrical geometric faceted chevron brackets, ///M tri-color accents, forward-angled ticks, dynamic ePOWER/charge fill ribbons, flashing \`>> eBOOST <<\` banner, 524px BMW Live Cockpit M center view.
  - **Audi Virtual Cockpit Plus RS e-tron GT**: Dual massive 472px circular dials, RS Red graduation rings, red sport needle, right RS Power/Boost dial with neon green recuperation zone, center MMI sector timing (S1/S2/S3).
  - **Volvo EX90 / Polestar**: Scandinavian panoramic minimalism on pure OLED black (#000000), centered speed, road view guide, Pilot Assist status, vertical PWR/REGEN energy flow ribbon.
- **Pure OLED Contrast & Zero Clipping**: Rigorously verified through 3 iteration cycles with high-resolution visual capture.
- **Direct Switch Homebrew**: Includes release \`gt7_switch_hud.nro\` (ARMv8 devkitA64) for Atmosphère.

---

### 🇯🇵 日本語
- **ラグジュアリーEVコックピット実車再現の徹底強化（モード3）**:
  - **Mercedes-Benz MBUX EQ**: 直径472pxの巨大アナログメーター、レーザー発光指針、7セグデジタル速度、PRNDシフトインジケーター、中央MBUXナビゲーション。
  - **BMW i-Curved Display OS 8.5/9**: 左右対称の幾何学的シェブロンブラケット、///M 3色ストライプ、前傾目盛り、\`>> eBOOST <<\` 点滅表示、中央 BMW Live Cockpit M。
  - **Audi Virtual Cockpit Plus RS e-tron GT**: 直径472pxのRSレッドメーター、高輝度スポーツ指針、RSパワーメーター＆回生グリーンゾーン、MMIセクタータイム表示。
  - **Volvo EX90 / Polestar**: スカンジナビア・ミニマリズム（完全OLED黒 #000000）、大型センター速度、Pilot Assist表示、垂直エネルギーリボン。
- **完全タッチスクリーン対応**: Joy-Con不要、フルタッチでモード切替・IP設定・全画面表示が可能。
- **Switch用 NRO バイナリ同梱**: Atmosphère環境で即起動可能。

---

### 🇩🇪 Deutsch
- **Luxus-EV-Cockpits mit maximaler Detailtreue (Modus 3)**:
  - **Mercedes-Benz MBUX EQ**: Durchmesser auf 472px vergrößert, Laser-Zeiger, 7-Segment-Tacho, PRND-Schaltanzeige, MBUX-Navigationsradar.
  - **BMW i-Curved Display OS 8.5/9**: Symmetrische Facetten-Chevron-Klammern, ///M-Dreifarben-Streifen, geneigte Skalenstriche, dynamisches ePOWER-Band, blinkendes \`>> eBOOST <<\`, BMW Live Cockpit M.
  - **Audi Virtual Cockpit Plus RS e-tron GT**: 472px RS-Doppelinstrumente, RS-Rote Skalierung, rote Sportnadel, RS-Power- & Rekuperationsanzeige, MMI-Sektorenzeiten.
  - **Volvo EX90 / Polestar**: Skandinavischer Minimalismus auf reinem OLED-Schwarz (#000000), zentrierte Geschwindigkeit, Pilot Assist, vertikales Energieband.
- **100% Touch-Bedienung**: Vollständige Bedienung ohne Joy-Cons auf der Nintendo Switch.
- **Nintendo Switch NRO**: Direkt ausführbares ARMv8 Homebrew für Atmosphère."

echo "=== 發布完成！ ==="
