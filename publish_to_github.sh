#!/usr/bin/env bash
set -e

echo "=== 檢查 GitHub 登入狀態 ==="
gh auth status

REPO_NAME="gt7-switch-sim-ddu"
echo "=== 正在為您建立 GitHub 公開儲存庫: $REPO_NAME ==="
gh repo create "$REPO_NAME" --public --source=. --remote=origin --push --description "PS5 Gran Turismo 7 Sim Racing DDU for Nintendo Switch (Atmosphère Homebrew .nro) with 8 motorsport modes & luxury EV cockpits" || true

echo "=== 正在發布 Release 與上傳 gt7_switch_hud.nro ==="
gh release create "v1.10.29.1350" dist/switch/gt7_hud/gt7_switch_hud.nro \
  --title "GT7 Switch Sim-DDU Pro _aw_1.10.29.1350 (Luxury EV Cockpit High-Fidelity & Vector Graphics)" \
  --notes "## 🏁 GT7 Switch Sim-DDU Pro _aw_1.10.29.1350

### 🌟 實車極致還原：Benz, BMW, Audi, Volvo 四大純電/旗艦座艙細節大躍進
針對使用者提供的最新實車操作畫面，進行逐像素細節比對與三大輪次迭代優化：
1. **Mercedes-Benz MBUX EQ Mode**：
   - 經典超大雙圓儀表（直徑達 472px，幾乎充滿螢幕垂直高度），數字與指針完美分層，指針運行零遮擋。
   - 中央地平線湛藍氣氛燈光暈（Ambient Horizon Aura），極致還原賓士座艙夜間科技感。
   - 中央「From Reset」磨砂卡片精緻化：整合即時油耗/電耗、行駛里程、即時單圈時間，並在純電里程中央繪製綠色雙葉 Eco 向量圖示。
   - 新增下方乘員安全帶監測膠囊列（Seat Occupancy Pill），展示前後五座安全帶帶扣狀態。
   - 右側垂直排檔列（R / N / D）搭配加大檔位與藍光 \`[ C ]\` / 紅光 \`[ S ]\` 駕駛模式膠囊。
2. **BMW i-Curved M (iDrive 8.5/9)**：
   - 多層次水晶切面導光雙箭矢（Chevrons），外框湛藍、內襯深海藍漸層。
   - **核心排版校正**：將超大數位車速（\`222 KM/H\`）精準移至左箭矢右側彎角處；將超大檔位（\`D\` / \`S\` / \`P\`）與獨立檔位數字移至右箭矢左側彎角處，徹底解決 \`S5\` 誤讀為 \`55\` 的問題！
   - 中央 3D 城市街道導航網格：白藍相間的交叉路口、立體建築透視、前進導航三角箭頭與真實 GT7 賽道雷達無縫結合。
   - 頂部加入 BMW 經典駕駛專注度監控攝影機梯形凹槽（Sensor Cluster Notch）。
   - 底部狀態列全面升級向量圖示：油槍圖示＋燃油進度條、時鐘、歐盟紅圈速限 \`50\`、車頂俯視圖＋安全帶警示、室外溫度、高壓電池圖示＋電量進度條。
3. **Audi Sport RS Virtual Cockpit (e-tron GT RS)**：
   - 經典 RS 曲棍球道轉速計（Runway Tachometer），1-3 階斜向、4-8 階橫向，7-8 轉紅線區高亮，白光轉速即時充盈。
   - 梯形跑道中心：超大數位車速，下方清晰顯示大字號檔位（如 \`R\`、\`S 6\`、\`D 4\`）與 \`DYNAMIC\` / \`AUTO\` 模式，兩側環抱綠色車道維持輔助箭矢。
   - 右側雙圓形性能錶（Power % 與 Torque %）搭配下方水平 \`MIN ──── BOOST ──── MAX\` 渦輪增壓條。
   - 頂部狀態列整合綠色大燈向量圖示、紅光 \`(P)\` 電子手煞車、紅底油量徽章、電台廣播 \`((o)) 2GB 873\`。
4. **Volvo EX90 / Polestar Scandinavian Cockpit (Google Built-in)**：
   - 全幅滿版深藍 Google 3D 衛星街道地圖，包含住宅路網、彎道街區、藍色地標公園（Devonshire park）及前進導航箭頭。
   - 左右兩側弧形刻度錶（左側車速 0..200 km/h，右側動力/回充）。
   - 兩側懸浮玻璃磁貼：左側容納速限 \`20\` 標誌與大型車速；右側容納綠色 \`READY\`、直立式排檔（P/R/N/D 與回充單踏板 \`- B +\`）及水平指針。
   - 頂部與底部全面替換為高品質向量圖示（大燈、車道維持、安全帶、俯視車身、電池與油槍）。

---

### 🇹🇼 繁體中文
- **旗艦座艙細節迭代與向量圖形引擎全面升級 (v1.10.29.1350)**：
  - **全新向量圖標繪製模組**：徹底淘汰易造成亂碼的 UTF-8 多位元組字元，採用純 SDL 向量幾何繪製大燈、安全帶、方向盤輔助、雙葉節能標章、加油機、電池、雪花溫度與車道維持圖示，呈現 100% 銳利無瑕畫質。
  - **大錶面高視覺震撼**：左右類比指針表直徑擴展至接近螢幕高度（472px），留出中央放置精美資訊卡片與導航，指針與數字分層錯開，杜絕任何遮擋。
  - **完整相容 GT7 Packet C (368B) & Packet ~ (344B)**：
    - 即時回充與輸出功率（kW）動態顯示。
    - 4 輪獨立馬達扭力向量（AWD Split）。
    - 輪胎路面材質即時偵測（柏油/路緣石/泥地）與組別標籤。

---

### 🇬🇧 English
- **Luxury EV Cockpit High-Fidelity & Vector Graphics Overhaul (v1.10.29.1350)**:
  - **Ultra-Detailed Mimicking of 4 Modern Luxury Cockpits**:
    - **Mercedes-Benz MBUX**: Full-height 472px dials, ambient horizon cyan glow, 'From Reset' frosted card with eco leaf vector icon, 5-seat occupancy pill, and bottom-right PRND column with C/S mode badge.
    - **BMW i-Curved M (iDrive 8.5/9)**: Precision chevron redesign with crystal facets. Digital speed (`222 KM/H`) repositioned to left inner nook; gear (`D`, `S`, `P`) with independent gear superscript repositioned to right inner nook. 3D city street map with destination pin and driver-monitoring camera notch.
    - **Audi Sport RS Virtual Cockpit**: RS runway hockey-stick tachometer with redline fill, central speed and dynamic gear (`S 6 DYNAMIC`), dual Power/Torque dials, horizontal BOOST bar, and oil temp.
    - **Volvo EX90 / Google Built-in**: Full-bleed panoramic dark navy Google map with Devonshire Park polygon, curved speed/power arcs, and floating glass tiles.
  - **Vector Graphics Primitives**: 100% custom-drawn vector icons for headlights, seatbelts, steering wheel assist, eco leaves, fuel pumps, batteries, snowflakes, and ADAS lane keeping.
  - **Telemetry Matrix Compatibility**: Full support for Packet C (368B) and Packet ~ (344B).

---

### 🇯🇵 日本語
- **最新高級EVコクピットの超高精度再現＆ベクター描画エンジン搭載 (v1.10.29.1350)**:
  - **実車計器盤の極限再現（4大ブランド徹底強化）**:
    - **Mercedes-Benz MBUX**: 画面高さいっぱいの直径472pxデュアルメーター、水平ブルーアンビエント照明、エコツインリーフ付き「From Reset」カード、前後5席シートベルト着座ピル、右下PRNDシフト。
    - **BMW i-Curved M (iDrive 8.5/9)**: 水晶切面シェブロン。速度表示を左側くぼみ、ギア表示（大文字D/S/P＋添え字段数）を右側くぼみに最適配置（S5の誤読を完全解消）。3D都市街路図、ナビ目的地ピン、ドライバー監視カメラノッチを実装。
    - **Audi Sport RS Virtual Cockpit**: RSホッケースティック型回転計、中央デジタル速度とギア表示（\`S 6 DYNAMIC\`）、2連パワー／トルクメーター、水平BOOSTバー。
    - **Volvo EX90 / Google Built-in**: 全画面Google 3Dストリートマップ、デヴォンシャ・パーク池ポリゴン、両端カーブメーター、フローティングガラスタイル。
  - **完全ベクターアイコン化**: ヘッドライト、シートベルト、ステアリング、リーフ、給油機、バッテリー、雪花、レーンキープを高品質ベクター描画。

---

### 🇩🇪 Deutsch
- **High-Fidelity-Überarbeitung der Luxus-EV-Cockpits & Vektorgrafiken (v1.10.29.1350)**:
  - **Präzise Nachbildung der 4 modernen EV-Cockpits**:
    - **Mercedes-Benz MBUX**: Großformatige 472px-Rundinstrumente, horizontales blaues Ambientelicht, 'From Reset'-Karte mit Öko-Blatt-Vektorsymbol, 5-Sitz-Belegungsanzeige, vertikale PRND-Schaltgasse mit C/S-Fahrmodus.
    - **BMW i-Curved M (iDrive 8.5/9)**: Facettierte Chevrons mit Kristallschattierung. Geschwindigkeit links, Fahrstufe (`D`, `S`, `P` mit separater Gangzahl) rechts neben den Instrumenten. 3D-Stadtstraßenkarte mit Ziel-Pin und Fahreraufmerksamkeits-Kamerakerbe.
    - **Audi Sport RS Virtual Cockpit**: RS-Runway-Drehzahlmesser mit rotem Drehzahlbereich, zentrale Anzeige (`S 6 DYNAMIC`), runde Leistungs-/Drehmomentanzeigen und BOOST-Balken.
    - **Volvo EX90 / Google Built-in**: Panorama-Google-Navigationskarte, geschwungene Skalenbögen und schwebende Glas-Kacheln.
  - **Scharfe Vektorgrafik**: Native Vektorsymbole für Scheinwerfer, Sicherheitsgurte, Lenkassistent, Batterie und Reifendruck.
  - **Volle Packet-C- (368B) und Packet-~- (344B) Unterstützung**."
- **Nintendo Switch Native ARMv8 Binary**: Ready-to-use \`gt7_switch_hud.nro\` included for Atmosphère homebrew.

---

### 🇯🇵 日本語
- **EVコクピットのリアルタイムギア連動＆全テレメトリ動的最適化 (v1.10.29.0950)**:
  - **Mercedes-Benz MBUX EQ クラシック2眼メーター**:
    - 右下PRNDコラムが現在ギアを動的ハイライト（R琥珀色、N白、D白）、大文字で \`P\`、\`R\`、\`N\`、\`D<ギア段数>\`（例: \`D4\`）を表示。
    - スロットル75%超で右下モードピルが赤色 \`[ S ]\`（Sportモード）へ自動切り替え、通常走行時は水色 \`[ C ]\`（Comfortモード）。
    - 中央「From Reset」トリップカードがリアルタイム残燃料（L）、現在車速、ラップ走行距離、リアルタイムラップタイマー、航続可能距離、レース順位 \`POS <順位>/<台数>\` を全動的表示。
    - 走行時は \`READY LAP <周回数>\`、停車時は \`PARK [ P ]\` を表示。
  - **BMW Curved Display i-Curved M (OS 8.5/9)**:
    - 右側シェブロン内に \`P\`、\`R\`、\`N\`、\`D<段数>\`、\`S<段数>\`（急加速時はモータースポーツレッド表示、例: \`S5\`）を動的表示。
    - 回生ブレーキ時 \`REGEN\`（緑）、全開加速時 \`SPORT\`（赤）、停車時 \`PARK\`、巡航時 \`ADAPTIVE\`（シアン）に連動。
    - 頂部ラベルと目盛数字の間隔を42pxに拡大し、文字重なりを完全解消。
  - **Audi Sport RS Virtual Cockpit**:
    - RSランウェイ回転計下に \`P\`、\`R\`、\`N\`、\`D <段数>\`、\`S <段数>\` と \`DYNAMIC\` / \`AUTO\` を動的表示。
    - 上部レッドバッジ \`[D] <航続距離> km\`、動的油温バー（\`OIL <温度>°C\`）、オドメーター＆トリップ距離をGT7データと連動。
  - **Volvo EX90 / Google Built-in パノラマコクピット**:
    - 右側ポッド内に動的シフト \`P\`、\`R\`、\`N\`、\`D\` および添え字段数（例: \`D^5\`）を表示、\`R N D\` 列を白ハイライト。
    - 回生ブレーキ時に \`- B +\` が鮮やかな緑色に点灯。
    - 下部デュアル航続距離表示にベクター描画のバッテリー \`[BAT] ── <距離> km\` および燃料 \`[GAS] ── <距離> km\` を実装。
- **Nintendo Switch ARMv8 ネイティブ実行ファイル**: \`gt7_switch_hud.nro\` をmicroSDの \`switch/gt7_hud/\` に配置するだけで即座に起動可能。

---

### 🇩🇪 Deutsch
- **Echtzeit-Gangpositionen & voll dynamische Telemetrie für alle EV-Cockpits (v1.10.29.0950)**:
  - **Mercedes-Benz MBUX EQ Klassisches Doppelinstrument**:
    - Rechter PRND-Spaltenwahlschalter hebt den aktiven Zustand hervor (R bernsteinfarben, N weiß, D weiß); große Anzeige für \`P\`, \`R\`, \`N\`, \`D<Gang>\` (z. B. \`D4\`).
    - Dynamische Fahrmodus-Pille schaltet bei Gas > 75% auf rotes \`[ S ]\` (Sport), bei normaler Fahrt auf cyanfarbenes \`[ C ]\` (Comfort).
    - Mittlere „From Reset“-Karte zeigt Live-Kraftstoffmenge (L), Live- & Höchstgeschwindigkeit, Rundendistanz, Rundentimer, Reichweite und Rennposition \`POS <Pos>/<Fahrzeuge>\`.
    - Untere Statuszeile schaltet während der Fahrt auf grünes \`READY LAP <Runde>\` und im Stillstand auf rotes \`PARK [ P ]\`.
  - **BMW Curved Display i-Curved M (OS 8.5/9)**:
    - Rechter Chevron zeigt \`P\`, \`R\`, \`N\`, \`D<Gang>\` oder \`S<Gang>\` (bei Volllast in Motorsport-Rot, z. B. \`S5\`).
    - Fahrmodus-Pille schaltet dynamisch: \`REGEN\` (grün) beim Bremsen, \`SPORT\` (rot) beim Beschleunigen, \`PARK\` im Stillstand, \`ADAPTIVE\` (cyan) beim Cruisen.
    - Vertikaler Abstand an den Chevron-Köpfen auf 42px vergrößert – garantiert null Überlappung zwischen \`KM/H\` / \`120\` und \`POWER %\` / \`100\`.
  - **Audi Sport RS Virtual Cockpit**:
    - Gangposition unter dem RS Runway zeigt \`P\`, \`R\`, \`N\`, \`D <Gang>\`, \`S <Gang>\` mit dynamischem \`DYNAMIC\` (rot) bzw. \`AUTO\`.
    - Rote Reichweiten-Plakette \`[D] <km> km\`, dynamische Öltemperaturanzeige mit Fortschrittsbalken und Live-Kilometerzähler.
  - **Volvo EX90 / Google Built-in Panoramacockpit**:
    - Rechter Glas-Pod zeigt dynamischen Gang \`P\`, \`R\`, \`N\`, \`D\` mit hochgestellter Ganganzeige (z. B. \`D^5\`) und weiß hervorgehobenem \`R N D\`.
    - \`- B +\` Rekuperationsanzeige leuchtet beim Bremsen grün auf.
    - Vektorgezeichnete Icons für Batterie \`[BAT] ── <km>\` und Kraftstoff \`[GAS] ── <km>\`.
- **Nintendo Switch ARMv8 Binary**: \`gt7_switch_hud.nro\` einsatzbereit für Atmosphère Homebrew."

echo "=== 發布完成！ ==="
