# GT7 Switch Sim-DDU Pro (Nintendo Switch Sim Racing & EV Cockpit DDU)
**Version / 版本號: `_aw_1.10.29.0839`**

<p align="center">
  <b>🌐 Language Navigation / 語言切換 / 言語切替 / Sprachauswahl:</b><br><br>
  <a href="#-繁體中文-traditional-chinese">🇹🇼 繁體中文</a> &nbsp;|&nbsp;
  <a href="#-english">🇬🇧 English</a> &nbsp;|&nbsp;
  <a href="#-日本語-japanese">🇯🇵 日本語</a> &nbsp;|&nbsp;
  <a href="#-deutsch-german">🇩🇪 Deutsch</a>
</p>

<p align="center">
  <i>Dedicated Full Documentation Files:</i><br>
  <a href="README_ZH.md"><b>[繁體中文完整文件]</b></a> &nbsp;•&nbsp;
  <a href="README_EN.md"><b>[English Full Docs]</b></a> &nbsp;•&nbsp;
  <a href="README_JA.md"><b>[日本語フルドキュメント]</b></a> &nbsp;•&nbsp;
  <a href="README_DE.md"><b>[Deutsche Dokumentation]</b></a>
</p>

<p align="center">
  <img src="assets/screenshots/telemetry_demo.gif" alt="GT7 Switch Sim-DDU Live Telemetry" width="680">
</p>

---

## 🇹🇼 繁體中文 (Traditional Chinese)

專為 **Nintendo Switch (V1 / V2 / Switch OLED 大氣層 Atmosphère 破解系統)** 打造的原生 Homebrew 賽車儀表板 App (`.nro`)。透過 Wi-Fi 接收 **PS5 / PS5 Pro《Gran Turismo 7 (跑車浪漫旅 7)》** 60Hz UDP 遙測封包，將 Switch 拆下左右 Joy-Con 搖桿後，直接架設於賽車方向盤基座前方，化身為高精準度、低延遲的頂級 **Sim Racing DDU (Dashboard Display Unit)**。

### 內建 8 大賽車與純電儀表板模式
1. **Mode 1: Official GT7 Native HUD & MFD (`1: GT7`)**：忠實還原 Polyphony Digital 原廠弧形轉速條、紅區閃爍提示、車速/檔位/建議檔位，並整合 4 頁切換式 MFD。
2. **Mode 2: High-Res Digital Tachometer DDU (`2: DIGI`)**：頂部 32 階漸變轉速燈條、油門煞車踩踏峰值記憶（PEAK BRAKE）、獨立胎溫與 Delta 計時。
3. **Mode 3: Luxury EV Digital Cockpit (`3: EV`)**：四大頂級純電品牌儀表（**Mercedes-Benz MBUX Hyperscreen EQ**、**BMW i-Curved Display iDrive 9 M-EV**、**Audi Virtual Cockpit Plus RS e-tron GT**、**Volvo EX90 Scandinavian Minimalist**）。
4. **Mode 4: F1 Grand Prix Steering Wheel (`4: F1`)**：模擬 Mercedes W14 / Ferrari SF23 PCU-8D 方向盤儀表。
5. **Mode 5: GT3 Endurance MoTeC DDU (`5: GT3`)**：耐力賽標準 MoTeC C1212 寬幅掃描轉速錶、燃油圈數計算。
6. **Mode 6: WRC Rally Stage Cockpit (`6: WRC`)**：拉力賽防滾籠專用 HUD，整合車身傾角儀（Roll / Pitch Horizon）。
7. **Mode 7: JDM Defi Advance ZD Multi-Meter (`7: JDM`)**：日系改裝經典指針式渦輪增壓錶與轉速大錶，搭配 Advance ZD 液晶條。
8. **Mode 8: GPS Track Map & Race Engineer (`8: MAP`)**：大尺寸 2D GPS 賽道圖、熱力著色行車線、最佳分段與進站窗口策略計算器。

### 100% 零搖桿全觸控與 OLED 純黑校準
- **一鍵全螢幕 (`FULL`)**：導覽列自動收起，畫面 100% 滿版無黑邊；輕觸左下角即刻跳出全螢幕。
- **自動偵測 PS5 (`IP: AUTO`)**：開機自動發送 UDP 廣播搜尋 PS5，無需手動輸入 IP；亦可點擊 IP 按鈕手動輸入。
- **純黑 OLED 校準**：非發光區域全數校準為 `#000000`，在 Switch OLED 螢幕上享有極致對比度。
- 👉 **[點此閱讀繁體中文完整說明文件](README_ZH.md)**

---

## 🇬🇧 English

A native Homebrew Sim Racing Dashboard Display Unit (`.nro`) tailored specifically for **Nintendo Switch (V1, V2, and Switch OLED on Atmosphère Custom Firmware)**. Designed to receive real-time 60Hz UDP telemetry packets streamed directly from **PS5 / PS5 Pro running *Gran Turismo 7*** over local Wi-Fi. Detach the left and right Joy-Cons, mount your Nintendo Switch directly onto your wheel base, and enjoy a professional, ultra-low-latency **Sim Racing DDU**.

### 8 Motorsport & Luxury EV Cockpit Modes
1. **Mode 1: Official GT7 Native HUD & MFD (`1: GT7`)**: Curved horizon tachometer bar (46-step gradient), shift redline warning flashes, suggested gear indicators, and a 4-page interactive MFD.
2. **Mode 2: High-Res Digital Tachometer DDU (`2: DIGI`)**: 32-step graduated top shift light strip, numeric RPM counter, dual throttle/brake meters with peak brake force memory, and live delta time.
3. **Mode 3: Luxury EV Cockpits (`3: EV`)**: Four modern luxury EV clusters (**Mercedes-Benz MBUX Hyperscreen EQ**, **BMW i-Curved Display iDrive 9 M-EV**, **Audi Virtual Cockpit Plus RS e-tron GT**, **Volvo EX90 Scandinavian Minimalist**).
4. **Mode 4: F1 Steering Wheel (`4: F1`)**: Modeled after contemporary F1 PCU-8D steering wheel screens (Mercedes W14 / Ferrari SF23).
5. **Mode 5: GT3 MoTeC (`5: GT3`)**: MoTeC C1212 endurance dashboard with horizontal rev sweep and fuel lap calculator.
6. **Mode 6: WRC Rally (`6: WRC`)**: Stage rally cockpit HUD featuring chassis roll and pitch artificial horizon meters.
7. **Mode 7: JDM Defi (`7: JDM`)**: Classic JDM street-tuned analog turbo boost gauge and oversized tachometer with Advance ZD multi-display strip.
8. **Mode 8: GPS Track Map (`8: MAP`)**: Full-screen 2D GPS circuit layout, braking/throttle line heatmaps, optimal sector tracking, and pit window strategy.

### 100% Joy-Con Free Multi-Touch & Pure OLED Black
- **Edge-to-Edge Fullscreen (`FULL`)**: Menus tuck away completely for 100% screen utilization on Switch 720p display without black bars. Tap bottom-left corner to exit fullscreen.
- **Auto-Discovery (`IP: AUTO`)**: Automatically discovers PS5 / PS5 Pro via UDP broadcast without manual typing.
- **Pure OLED Black Calibration**: Non-active pixel backgrounds are set to absolute `#000000` for infinite contrast on Switch OLED screens.
- 👉 **[Read Full English Documentation](README_EN.md)**

---

## 🇯🇵 日本語 (Japanese)

**Nintendo Switch (初期型・バッテリー強化版・Switch OLED / Atmosphère 大気層 CFW 環境)** 向けに開発された、ネイティブ Homebrew レーシング＆EV デジタルメーターアプリ（`.nro`）です。Wi-Fi 経由で **PS5 / PS5 Pro の『グランツーリスモ7』(GT7)** からブロードキャストされる 60Hz UDP テレメトリーデータを受信します。Switch 本体の左右 Joy-Con を取り外し、ステアリングコントローラーの直前に設置することで、超低遅延・高精度な本格派 **Sim Racing DDU (Dashboard Display Unit)** として活用できます。

### 8 つの本格モータースポーツ＆高級 EV デジタルメーターモード
1. **Mode 1: GT7 公式純正 HUD & MFD (`1: GT7`)**: ポリフォニー・デジタル純正の美しい曲線ホライゾン・レブバー、シフト警告点滅、推奨ギア表示、4 ページ切替 MFD を完全再現。
2. **Mode 2: 高解像度デジタルタコメーター DDU (`2: DIGI`)**: 上部 32 段階シフトランプ、数値 RPM、最大ブレーキ踏力メモリー付きペダルゲージ、個別タイヤ温度、デルタタイム表示。
3. **Mode 3: 高級 EV デジタルコックピット (`3: EV`)**: 最新高級 EV 4 大ブランド（**Mercedes-Benz MBUX ハイパースクリーン EQ**、**BMW i-Curved ディスプレイ iDrive 9 M-EV**、**Audi バーチャル コックピット プラス RS e-tron GT**、**Volvo EX90 スカンジナビア・ミニマリズム**）。
4. **Mode 4: F1 ステアリング (`4: F1`)**: 現代 F1 マシン（Mercedes W14 / Ferrari SF23 PCU-8D）を模したステアリング液晶画面。
5. **Mode 5: GT3 MoTeC (`5: GT3`)**: 耐久レーシングカー用 MoTeC C1212 スイープタコメーター＆燃料残量ラップ計算。
6. **Mode 6: WRC ラリー (`6: WRC`)**: ラリー HUD、ロール＆ピッチ角を計測する車体傾斜儀を搭載。
7. **Mode 7: JDM Defi (`7: JDM`)**: アナログ針式ブースト計＆タコメーター、Advance ZD 有機 EL 風マルチメーター。
8. **Mode 8: GPS サーキットマップ (`8: MAP`)**: 2D GPS サーキットコース図、加減速ヒートマップ、最適セクタータイム比較、ピット戦略計算。

### 完全ジョイコン不要・マルチタッチ＆ピュア OLED ブラック
- **全画面フルスクリーン (`FULL`)**: 上下のバーが瞬時に格納され、Switch の 720p 画面を余白なく 100% フル活用。
- **自動探索 (`IP: AUTO`)**: 起動時に UDP ブロードキャストで PS5 / PS5 Pro を自動検知。
- **ピュア OLED ブラック校準**: 背景はすべて `#000000` に設定され、Switch OLED の無限のコントラスト比を最大化。
- 👉 **[日本語版フルドキュメントはこちら](README_JA.md)**

---

## 🇩🇪 Deutsch (German)

Ein natives Homebrew Sim-Racing Dashboard Display Unit (`.nro`) speziell für die **Nintendo Switch (V1, V2 und Switch OLED unter Atmosphère Custom Firmware)**. Entwickelt, um 60Hz UDP-Telemetriedaten in Echtzeit direkt von der **PS5 / PS5 Pro mit *Gran Turismo 7*** über lokales WLAN zu empfangen. Nehmen Sie die linken und rechten Joy-Cons ab, montieren Sie die Switch direkt vor Ihrer Lenkrad-Base und genießen Sie ein professionelles, extrem latenzarmes **Sim-Racing DDU**.

### 8 Motorsport- & Luxus-EV-Cockpit-Modi
1. **Mode 1: Offizielles GT7 HUD & MFD (`1: GT7`)**: Geschwungener Polyphony Digital Horizont-Drehzahlmesser (46 Stufen), Schaltblitz-Warnanzeige, Ganganzeige und 4-seitiges umschaltbares MFD.
2. **Mode 2: High-Res Digitaler Drehzahlmesser (`2: DIGI`)**: 32-stufiges LED-Schaltband, präzise numerische Drehzahlanzeige, Pedalwege mit Spitzendruck-Speicher (Peak Brake), Reifentemperaturen und Delta-Zeit.
3. **Mode 3: Luxus-EV-Cockpits (`3: EV`)**: Vier moderne Luxus-EV-Cockpits (**Mercedes-Benz MBUX Hyperscreen EQ**, **BMW i-Curved Display iDrive 9 M-EV**, **Audi Virtual Cockpit Plus RS e-tron GT**, **Volvo EX90 Skandinavischer Minimalismus**).
4. **Mode 4: F1 Lenkrad (`4: F1`)**: Nachempfunden modernen F1 PCU-8D Lenkraddisplays (Mercedes W14 / Ferrari SF23).
5. **Mode 5: GT3 MoTeC (`5: GT3`)**: MoTeC C1212 Langstrecken-DDU mit breitem Drehzahlband und Runden-Kraftstoffrechner.
6. **Mode 6: WRC Rallye (`6: WRC`)**: Rallye-Cockpit mit integrierter Neigungs- und Rollanzeige (Künstlicher Horizont).
7. **Mode 7: JDM Defi (`7: JDM`)**: Japanische Tuning-Rundinstrumente für Ladedruck und Drehzahl kombiniert mit Advance ZD Multi-Anzeige.
8. **Mode 8: GPS Streckenkarte (`8: MAP`)**: 2D GPS Rennstreckenlayout, Brems-/Gas-Heatmaps, Sektoren-Delta und Boxenstopp-Strategierechner.

### 100% Joy-Con-freie Touch-Bedienung & Pure OLED Schwarz
- **Randloses Vollbild (`FULL`)**: Menüleisten blenden sich für 100% Bildschirm-Ausnutzung auf dem 720p Switch-Display vollständig aus.
- **Automatische Erkennung (`IP: AUTO`)**: Findet die PS5 / PS5 Pro beim Start automatisch via UDP-Broadcast.
- **Pure OLED Schwarz Kalibrierung**: Alle inaktiven Displaybereiche sind auf reines `#000000` kalibriert für unendlichen Kontrast auf der Switch OLED.
- 👉 **[Deutsche Dokumentation lesen](README_DE.md)**

---

## 📦 Download & Release (`gt7_switch_hud.nro`)

- 🌐 **GitHub Releases**: [Download v1.10.29.0839](https://github.com/d9053326-bot/gt7-switch-sim-ddu/releases/tag/v1.10.29.0839)
- 📦 **Direct .NRO Binary**: [gt7_switch_hud.nro (6.52 MB)](https://github.com/d9053326-bot/gt7-switch-sim-ddu/releases/download/v1.10.29.0839/gt7_switch_hud.nro)
- 📂 **Install Path on SD Card**: `/switch/gt7_hud/gt7_switch_hud.nro`
