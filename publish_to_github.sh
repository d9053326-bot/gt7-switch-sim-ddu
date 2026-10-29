#!/usr/bin/env bash
set -e

echo "=== 檢查 GitHub 登入狀態 ==="
gh auth status

REPO_NAME="gt7-switch-sim-ddu"
echo "=== 正在為您建立 GitHub 公開儲存庫: $REPO_NAME ==="
gh repo create "$REPO_NAME" --public --source=. --remote=origin --push --description "PS5 Gran Turismo 7 Sim Racing DDU for Nintendo Switch (Atmosphère Homebrew .nro) with 8 motorsport modes & luxury EV cockpits" || true

echo "=== 正在發布 Release 與上傳 gt7_switch_hud.nro ==="
gh release create "v1.10.29.0820" dist/switch/gt7_hud/gt7_switch_hud.nro \
  --title "GT7 Switch Sim-DDU Pro _aw_1.10.29.0820" \
  --notes "## 🏁 GT7 Switch Sim-DDU Pro _aw_1.10.29.0820

### 🇹🇼 繁體中文
- **豪華四大品牌最新實車儀表極致擬真重製 (模式 3)**：
  - **Mercedes-Benz MBUX Classic**：雙類比圓錶直徑擴大至 472px（貼近螢幕高度），外圈放射狀 60 條細緻雷射光芒、發光指針與動態光弧、中央頂部速限牌與數位車速、中央浮動玻璃資訊卡（From Reset / ENTIRE 油耗、里程、時間與節能葉片標籤）、右下 PRND 檔位與天藍色 \`[ C ]\` 模式框、底部獨立狀態列（01:58 pm、PARK、[ P ]、24°F），完美無文字重疊。
  - **BMW Curved Display OS 8.5/9**：全螢幕深藍透視 3D 城市街道導航網格底圖、經典垂直外側側翼三段式晶體切面括弧 Chevron（< 與 >）、左側嵌入大字速度與正下方置中 \`km/h\`、右側檔位與 \`ADAPTIVE\` 駕駛模式膠囊、底部即時資訊列（⛽ 698 km、10:49、[ 50 ]、🚗、+ 19.0 °C、⚡ 55 km）與賽道幾何軌跡。
  - **Audi Sport RS Virtual Cockpit**：經典中置 RS Runway（曲棍球棒彎折式）跑道轉速錶（1-3 斜向爬升、4-8 頂部水平展開，7-8 競技紅線）、跑道下方嵌入式數位車速與檔位（2 / AUTO）、左側精緻 Date & time（10:43 AM、Wednesday 11.3.2026、OIL 105 °C）、右側雙圓環即時監控錶（Power % 與 Torque %）搭配底部橫向 BOOST 增壓能量條、頂部燃油里程與電台徽章、底部雙里程與車外溫度。
  - **Volvo Google Built-in Panoramic Cockpit**：全螢幕全景 3D 導航地圖（Devonshire park 湖泊、街道網格、立體導航藍箭頭、Google 品牌標誌）、左側懸浮深色玻璃速度卡（速限 20 標誌、大字車速 255 km/h）搭配左外圈弧形圓環速度計、右側懸浮深色玻璃控制卡（綠色 READY 標籤、垂直 PRND、- B + 換檔、POWER / CHARGE）搭配右外圈能量弧線、底部雙動能電量/續航條（0 km / 540 km）。
- **3 次完整迭代驗證**：所有元素皆經截圖檢視，確認比例、層次、對比度與質感完全貼近實車。
- **純 OLED 零破圖保證**：純黑 OLED 背景 (#000000)，高對比細節，完美呈現液晶儀表頂級質感。
- **完全支援 Switch 觸控**：無須 Joy-Con 即可單手點擊切換 8 大賽車模式與手動設定 IP。
- **Switch ARMv8 原生執行檔**：產出 \`gt7_switch_hud.nro\`，放入 microSD 卡 \`switch/gt7_hud/\` 即可直接暢玩！

---

### 🇬🇧 English
- **OEM Cluster Production Realism Overhaul (Mode 3)**:
  - **Mercedes-Benz MBUX Classic**: Enlarged 472px diameter dual dials (~screen height), 60 radial laser sunburst rays, glowing red needles with dynamic outer trail arcs, top limit badge and digital speed, center floating glass card ('From Reset' / 'ENTIRE' with mpg, mph, mi, time, eco leaf), bottom status row ('01:58 pm  PARK  [ P ]  ❄ 24°F'), and vertical gear PRND with cyan \`[ C ]\` mode box.
  - **BMW Curved Display OS 8.5/9**: Full-screen deep blue 3D perspective city/circuit grid map, symmetrical 3-segment crystalline faceted chevrons with vertical outer flank, speed digits with \`km/h\` centered cleanly below, gear & \`ADAPTIVE\` pill, bottom indicators (⛽ 698 km, 10:49, [ 50 ], 🚗, + 19.0 °C, ⚡ 55 km) and framed track telemetry.
  - **Audi Sport RS Virtual Cockpit**: Central iconic RS Runway (hockey stick) tachometer (slanted 1-3, horizontal 4-8 with redline on 7-8), under-runway digital speed and gear (2 / AUTO), left date/time/oil temp, right dual circular pods (Power % and Torque %) with horizontal BOOST bar, top fuel & radio info, bottom odometer & ambient temp.
  - **Volvo Google Built-in Panoramic Cockpit**: Full-screen 3D panoramic navigation map (lake, road grid, blue navigation arrowhead, Google logo), floating left dark glass pod ([ 20 ] limit, digital speed 255 km/h) with outer curved speedometer arc, floating right dark glass pod (READY green badge, vertical PRND, - B +, power/charge) with outer curved power/regen arc, bottom dual battery/fuel bars.
- **Triple-Iteration Quality Assurance**: Exhaustively tested and visually inspected via SDL2 render captures.
- **Pure OLED Contrast**: Zero clipping, deep black backgrounds (#000000), and crisp antialiased rendering.
- **100% Touchscreen Operation**: Change modes and configure IP directly via Switch touch screen.
- **Pre-compiled Binary**: Ready-to-run \`gt7_switch_hud.nro\` for Atmosphère CFW.

---

### 🇯🇵 日本語
- **最新実車メーターデザインの超高精度再現（モード3）**:
  - **Mercedes-Benz MBUX Classic**: 直径472pxの巨大2眼アナログメーター、60本の放射状レーザーサンバースト目盛り、発光レッド指針＆動的アウタートレイル、中央上部制限速度＆デジタルスピード、中央フローティングガラスカード（From Reset / ENTIRE 燃費・走行距離・時間・エコリーフ）、下部ステータスバー（01:58 pm、PARK、[ P ]、❄ 24°F）、右下PRNDとシアン\`[ C ]\`モード。
  - **BMW Curved Display OS 8.5/9**: フルスクリーン3Dパースペクティブ都市グリッドマップ、垂直アウターフランジ付き3分割クリスタルファセットシェブロン（< と >）、速度数値＆中央配置\`km/h\`、ギア＆\`ADAPTIVE\`ピル、下部ステータス（⛽ 698 km、10:49、[ 50 ]、🚗、+ 19.0 °C、⚡ 55 km）。
  - **Audi Sport RS Virtual Cockpit**: 伝統の中置RSランウェイ（ホッケースティック型）タコメーター（1-3斜め、4-8水平バー、7-8レッドゾーン）、ランウェイ下部デジタル速度＆ギア（2 / AUTO）、左側日時＆油温表示（OIL 105 °C）、右側デュアル丸型ポッド（Power % & Torque %）＆BOOSTバー、上部燃料＆ラジオ情報、下部オドメーター。
  - **Volvo Google Built-in Panoramic Cockpit**: フルスクリーンパノラマ3Dナビゲーションマップ（Devonshire park、道路グリッド、青色ナビ矢印、Googleロゴ）、左側フローティングガラス速度ポッド（制限20標識、255 km/h）＆外周スピードメーター弧、右側フローティングガラス制御ポッド（READY緑バッジ、PRND、- B +、power/charge）＆外周エネルギー弧、下部デュアルバッテリー/航続バー。
- **3段階の反復検証完了**: 視覚キャプチャにより実車写真と細部まで照合・最適化済み。
- **純粋なOLED黒表示**: #000000の完全黒背景で最高峰のコントラストを実現。
- **完全タッチスクリーン対応**: Switch画面のタッチのみで全機能・全モードを快適に操作可能。
- **Switch用 NRO バイナリ同梱**: Atmosphère環境で \`switch/gt7_hud/\` に配置するだけで即起動可能。

---

### 🇩🇪 Deutsch
- **Hochpräzise Nachbildung aktueller Serienfahrzeug-Cockpits (Modus 3)**:
  - **Mercedes-Benz MBUX Classic**: Gewaltige 472-px-Rundinstrumente (~Bildschirmhöhe), 60 radiale Laser-Sonnenstrahlen-Skalenstriche, leuchtende rote Nadeln mit Lichtschweif-Bögen, mittige Geschwindigkeits- und Tempolimit-Anzeige, schwebende Glaskarte ('From Reset' / 'ENTIRE' mit mpg, mph, mi, Zeit, Eco-Blatt), untere Statusleiste ('01:58 pm  PARK  [ P ]  ❄ 24°F') und Gangwahl PRND mit cyanfarbenem \`[ C ]\`-Modus.
  - **BMW Curved Display OS 8.5/9**: Vollbild-3D-Perspektiven-Stadtplan, symmetrische 3-teilige kristalline Facetten-Chevrons mit vertikalen Außenflanken, digitale Geschwindigkeitsanzeige mit zentriertem \`km/h\`, Gang & \`ADAPTIVE\`-Pille, untere Statusanzeigen (⛽ 698 km, 10:49, [ 50 ], 🚗, + 19.0 °C, ⚡ 55 km) und telemetrische Streckengrafik.
  - **Audi Sport RS Virtual Cockpit**: Zentraler ikonischer RS-Runway-Drehzahlmesser (Hockeyschläger-Form: 1-3 schräg ansteigend, 4-8 horizontal mit rotem Drehzahlbereich bei 7-8), integrierte Digitalgeschwindigkeit & Gang (2 / AUTO) unter dem Runway, Datum/Uhrzeit/Öltemperatur links, doppelte RS-Rundinstrumente rechts (Power % und Torque %) mit BOOST-Leiste, obere Kraftstoff- & Radioanzeige, unterer Kilometerzähler & Außentemperatur.
  - **Volvo Google Built-in Panoramic Cockpit**: Vollbild-3D-Panoramakarte (See 'Devonshire park', Straßennetz, blauer Navigationspfeil, Google-Schriftzug), linkes schwebendes Dunkelglas-Pod (Tempolimit 20, 255 km/h) mit äußerem Tacho-Bogen, rechtes schwebendes Dunkelglas-Pod (READY, PRND, - B +, power/charge) mit äußerem Leistungs-/Rekuperationsbogen, doppelte Batterie-/Reichweitenanzeige unten.
- **Dreistufige iterative Verifikation**: Umfangreich via SDL2-Rendering getestet und mit den Originalfotos abgeglichen.
- **Reines OLED-Schwarz**: Maximaler Kontrast (#000000) ohne Textüberschneidungen.
- **100% Touchscreen-Unterstützung**: Alle Modi und IP-Einstellungen direkt per Touch auf der Nintendo Switch bedienbar.
- **Vorkompilierte Switch-Binärdatei**: \`gt7_switch_hud.nro\` für Atmosphère CFW enthalten."

echo "=== 發布完成！ ==="
