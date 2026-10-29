#!/usr/bin/env bash
set -e

echo "=== 檢查 GitHub 登入狀態 ==="
gh auth status

REPO_NAME="gt7-switch-sim-ddu"
echo "=== 正在為您建立 GitHub 公開儲存庫: $REPO_NAME ==="
gh repo create "$REPO_NAME" --public --source=. --remote=origin --push --description "PS5 Gran Turismo 7 Sim Racing DDU for Nintendo Switch (Atmosphère Homebrew .nro) with 8 motorsport modes & luxury EV cockpits" || true

echo "=== 正在發布 Release 與上傳 gt7_switch_hud.nro ==="
gh release create "v1.10.29.0950" dist/switch/gt7_hud/gt7_switch_hud.nro \
  --title "GT7 Switch Sim-DDU Pro _aw_1.10.29.0950" \
  --notes "## 🏁 GT7 Switch Sim-DDU Pro _aw_1.10.29.0950

### 🇹🇼 繁體中文
- **豪華電車儀表即時檔位連動與全動態遙測深化優化 (v1.10.29.0950)**：
  - **Mercedes-Benz MBUX EQ 經典雙環儀表**：
    - 右下角檔位直列矩陣動態高亮當前檔位（R 琥珀黃、N 白、D 白），大字型顯示 \`P\`、\`R\`、\`N\`、\`D<檔位>\`（如 \`D4\`）。
    - 油門深度 > 75% 時，右下角駕駛模式膠囊動態切換為紅色 \`[ S ]\`（Sport 運動模式），一般巡航自動切換為水藍色 \`[ C ]\`（Comfort 舒適模式）。
    - 中央「From Reset」旅程電腦全動態化：即時油量/電量（L）、真實與極速時速、當圈行駛里程、動態計時器、剩餘里程 \`RANGE <km/mi>\` 與賽事名次 \`POS <名次>/<總車數>\`。
    - 底部狀態列在車輛行駛中動態顯示綠色 \`READY LAP <圈數>\`，停車時自動顯示紅色 \`PARK [ P ]\`。
  - **BMW Curved Display i-Curved M 弧形儀表 (OS 8.5/9)**：
    - 右側 Chevron 內角凹折處大字型即時顯示 \`P\`、\`R\`、\`N\`、\`D<檔位>\`、\`S<檔位>\`（全油門時顯示熱血運動紅字，如 \`S5\`）。
    - 智慧行車模式膠囊動態連動：減速回充顯示綠色 \`REGEN\`、大腳油門顯示紅色 \`SPORT\`、停車顯示 \`PARK\`、巡航顯示水藍色 \`ADAPTIVE\`。
    - 頂部 Chevron 標籤與刻度數字垂直間距加大至 42px，徹底避免 \`KM/H\` 與 \`120\`、\`POWER %\` 與 \`100\` 接觸。
    - 底部油量與高壓電池即時連動 GT7 遙測數據與動態指示條。
  - **Audi Sport RS Virtual Cockpit (RS Runway 跑道儀表)**：
    - 跑道轉速錶下方大字型檔位動態顯示 \`P\`、\`R\`、\`N\`、\`D <檔位>\`、\`S <檔位>\`；並動態切換紅字 \`DYNAMIC\` 與一般 \`AUTO\`。
    - 頂部紅色油量膠囊 \`[D] <里程> km\` 與加油槍圖示動態連動，左側機油溫度連動即時 \`OIL <溫度>°C\` 與漸進條。
    - 底部里程表動態計算累計行駛總里程與當次單圈里程。
  - **Volvo EX90 / Google Built-in 全景式座艙**：
    - 右側浮空玻璃艙大字型檔位動態切換 \`P\`、\`R\`、\`N\`、\`D\`，右上角高亮顯示檔位數字（如 \`D^5\`）。
    - \`R N D\` 直列排檔柱即時高亮當前運作檔位，煞車回充時 \`- B +\` 燈號自動點亮青綠色。
    - 底部右側雙能源資訊採用專屬繪製電池圖示 \`[BAT] ── <里程> km\` 與油箱圖示 \`[GAS] ── <里程> km\`，完全消除亂碼或超出邊界。
- **全觸控操作與純 OLED 顯示**：支援 Switch 電容全觸控切換主題與手動設定 IP，極致純黑背景 (#000000)。
- **Switch ARMv8 原生執行檔**：提供 \`gt7_switch_hud.nro\`，放入 microSD 卡 \`switch/gt7_hud/\` 即可直接暢玩！

---

### 🇬🇧 English
- **Live Transmission Gear Telemetry & Full Dynamic Readout Optimization (v1.10.29.0950)**:
  - **Mercedes-Benz MBUX EQ Classic Dual-Dial Cockpit**:
    - Right vertical PRND selector matrix dynamically highlights active transmission state (R amber, N white, D white); prominent gear readout shows \`P\`, \`R\`, \`N\`, \`D<gear>\` (e.g. \`D4\`).
    - Dynamic drive mode pill shifts to red \`[ S ]\` (Sport mode) when throttle > 75%, returning to cyan \`[ C ]\` (Comfort mode) during cruising.
    - Central \"From Reset\" trip card displays real-time fuel volume (L), current & peak speed, trip distance, live lap timer, remaining dynamic range, and race standing \`POS <pos>/<cars>\`.
    - Bottom status row displays green \`READY LAP <lap>\` when in motion and shifts to red \`PARK [ P ]\` when vehicle is stationary.
  - **BMW Curved Display i-Curved M (OS 8.5/9)**:
    - Right chevron crook displays dynamic gear \`P\`, \`R\`, \`N\`, \`D<gear>\`, or \`S<gear>\` in BMW M motorsport red under heavy acceleration (e.g. \`S5\`).
    - Dynamic driving mode pill responds in real time: green \`REGEN\` under braking, red \`SPORT\` under full throttle, \`PARK\` when stopped, and cyan \`ADAPTIVE\` during cruise.
    - Chevron top header margins expanded to 42px vertical clearance to eliminate any collision between \`KM/H\` / \`120\` and \`POWER %\` / \`100\`.
    - Dynamic fuel & high-voltage battery range bars wired directly to GT7 live telemetry.
  - **Audi Sport RS Virtual Cockpit**:
    - Digital gear readout under RS Runway shows \`P\`, \`R\`, \`N\`, \`D <gear>\`, \`S <gear>\` with dynamic \`DYNAMIC\` (red) vs \`AUTO\` status.
    - Dynamic red fuel range badge \`[D] <range> km\` with custom fuel pump glyph; dynamic oil temperature bar tracking live GT7 engine oil temps.
    - Bottom odometer dynamically tracks cumulative distance and current lap trip distance.
  - **Volvo EX90 / Google Built-in Panoramic Cockpit**:
    - Right floating glass pod displays dynamic transmission gear \`P\`, \`R\`, \`N\`, \`D\`, with active cyan gear superscript (e.g. \`D^5\`).
    - \`R N D\` column dynamically highlights active gear; \`- B +\` one-pedal regen indicator illuminates vibrant green during braking.
    - Bottom dual energy status features custom-drawn vector battery icon \`[BAT] ── <range> km\` and fuel icon \`[GAS] ── <range> km\`.
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
    - BMWの時速数値をx=350へ配置（シェブロン枠との干渉を完全排除）。重複していたADAPTIVEピルを解消。
    - Audi RSランウェイの斜め目盛り数値を20px外側にオフセット。
    - Volvoのスピードメーター弧を再計算し、200から0までの全8目盛数値が画面左端（x=29〜123）に100%完全に収まるよう修正。
    - JDM Defiメーターの針軸と中央ギア数字の重なりを解消。
  - **画面境界の安全性確保**:
    - Mercedes-BenzのPRNDおよび[ C ]をメーター外側右下へ配置し、円周線との交差を解消。
- **Switch用 NRO バイナリ同梱**: Atmosphère環境で即起動可能。

---

### 🇩🇪 Deutsch
- **Staffelung von Zahlen & Zeigern sowie Behebung von Textüberläufen (v1.10.29.0839)**:
  - **Keine Zeigerkollisionen & saubere Trennung**:
    - Mercedes-Benz Skalenzahlen auf 184px Innenradius verlegt; Nadeln unter den Zahlen gerendert für perfekte Lesbarkeit bei Zeigerdurchgang.
    - BMW Geschwindigkeit auf x=350 zentriert mit 18px+ Abstand zum Chevron; doppelte ADAPTIVE-Box entfernt.
    - Audi RS Runway Zahlen mit 20px Sicherheitsabstand zur Diagonale angeordnet.
    - Volvo Tacho-Geometrie neuberechnet; alle 8 Zahlen (200 bis 0) liegen sauber innerhalb des Bildschirms (x=29~123).
    - JDM Defi Nadelachse kollidiert nicht mehr mit der Ganganzeige.
  - **Sichere Bildschirrgrenzen**:
    - Mercedes-Benz PRND und [ C ]-Modus in die untere rechte Ecke außerhalb der Rundinstrumente verlegt.
- **Vorkompilierte Switch-Binärdatei**: \`gt7_switch_hud.nro\` für Atmosphère CFW enthalten."

echo "=== 發布完成！ ==="
