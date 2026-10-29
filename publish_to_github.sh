#!/usr/bin/env bash
set -e

echo "=== 檢查 GitHub 登入狀態 ==="
gh auth status

REPO_NAME="gt7-switch-sim-ddu"
echo "=== 正在為您建立 GitHub 公開儲存庫: $REPO_NAME ==="
gh repo create "$REPO_NAME" --public --source=. --remote=origin --push --description "PS5 Gran Turismo 7 Sim Racing DDU for Nintendo Switch (Atmosphère Homebrew .nro) with 8 motorsport modes & luxury EV cockpits" || true

echo "=== 正在發布 Release 與上傳 gt7_switch_hud.nro ==="
gh release create "v1.10.29.0839" dist/switch/gt7_hud/gt7_switch_hud.nro \
  --title "GT7 Switch Sim-DDU Pro _aw_1.10.29.0839" \
  --notes "## 🏁 GT7 Switch Sim-DDU Pro _aw_1.10.29.0839

### 🇹🇼 繁體中文
- **極致擬真度與字體指針錯開深度優化 (v1.10.29.0839)**：
  - **數字與指針完美錯開 (零卡住、零重疊)**：
    - Mercedes-Benz 類比雙圓錶數字調整至 184px 半徑內圈，雷射指針與外圈動態弧線置於 216-230px 外圈刻度，並改為指針先畫、數字與刻度後畫，指針掃過時數字 100% 清晰可辨，絕不卡住指針或遮蔽數字。
    - BMW i-Curved 數位時速置中於 x=350（3位數完全脫離內側側翼線），與 Chevron 括弧及動能導光帶保持 18px 以上安全淨空；修復 ADAPTIVE 駕駛模式膠囊多層重疊問題。
    - Audi Sport RS Runway 轉速跑道 1-3 斜向數字增加至 20px 淨空距離；數位車速與檔位完美置中。
    - Volvo 全景 Google 儀表重構左側速度弧線（角度 -50° 至 +50°），8 組刻度數字（200 下至 0）全部 100% 完整收納於螢幕左邊界內（x=29~123），徹底解決數字出界超出版面的問題。
    - JDM Defi 模式轉速錶指針與中央檔位數字分層錯開，中心針軸不再壓住檔位數字下緣。
  - **版面邊界安全淨空保證**：
    - Mercedes-Benz 右側 PRND 與 [ C ] 模式完全移至轉速錶外側右下角，白色錶圈不再切過檔位文字，與右邊界保留 25px 安全距離。
    - 中央座椅佔位圖形縮短至 190px，與左右圓錶保留 64px 空間。
- **全觸控操作與純 OLED 顯示**：完全支援電容全觸控切換主題與手動設定 IP，極致純黑背景 (#000000)。
- **Switch ARMv8 原生執行檔**：提供 \`gt7_switch_hud.nro\`，放入 microSD 卡 \`switch/gt7_hud/\` 即可直接暢玩！

---

### 🇬🇧 English
- **Visual Staggering & Text Boundary Optimization (v1.10.29.0839)**:
  - **Numbers Staggered from Needle & Pointers (Zero Collisions)**:
    - Mercedes-Benz dial numbers offset to inner 184px radius; laser needles and outer trailing arcs placed on outer 216-230px perimeter. Needles rendered underneath dial labels so graduation numbers remain 100% legible during needle sweeps.
    - BMW digital speed moved to x=350 (digits 18px+ clear of chevron flank); removed duplicate ADAPTIVE pill overlap.
    - Audi RS Runway diagonal tick numbers given 20px clear margin; digital speed and gear centered with 3 digits.
    - Volvo speedometer arc geometry recalculated (-50° to +50°); all 8 scale numbers (200 down to 0) strictly contained within screen boundaries (x=29~123).
    - JDM Defi needle hub separated from gear readout with clean vertical margin.
  - **Canvas Edge Containment**:
    - Mercedes-Benz PRND and [ C ] gear pill moved outside the dial into the lower right corner with zero circle clipping.
    - Center occupancy panel narrowed with 64px clearance from dial circles.
- **100% Touchscreen & Pure OLED Contrast**: Full touch navigation, zero clipping, ready-to-run Switch Homebrew.

---

### 🇯🇵 日本語
- **指針と数値の段差配置＆文字はみ出し防止の最適化 (v1.10.29.0839)**:
  - **指針と数字の完全分離（重なり・引っかかり解消）**:
    - Mercedes-Benzのメーター数値を内径184pxへオフセット。指針を数値の下層に描画することで針通過時も数字が完全に視認可能。
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
