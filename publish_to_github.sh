#!/usr/bin/env bash
set -e

echo "=== 檢查 GitHub 登入狀態 ==="
gh auth status

REPO_NAME="gt7-switch-sim-ddu"
echo "=== 正在為您建立 GitHub 公開儲存庫: $REPO_NAME ==="
gh repo create "$REPO_NAME" --public --source=. --remote=origin --push --description "PS5 Gran Turismo 7 Sim Racing DDU for Nintendo Switch (Atmosphère Homebrew .nro) with 8 motorsport modes & luxury EV cockpits" || true

echo "=== 正在發布 Release 與上傳 gt7_switch_hud.nro ==="
gh release create "_aw_1.10.28.2026" dist/switch/gt7_hud/gt7_switch_hud.nro \
  --title "GT7 Switch Sim-DDU Pro _aw_1.10.28.2026" \
  --notes "### GT7 Switch Sim-DDU Pro (版本號: _aw_1.10.28.2026)
- 8 大專業賽車與純電儀表板模式 (GT7 官方原版、高解析數位轉速表、EV 純電座艙、F1 方向盤、GT3 MoTeC、WRC 拉力、JDM 改裝雙錶、GPS 賽道雷達)
- 4 款豪華純電車數位儀表 (Mercedes MBUX Hyperscreen EQ、BMW i-Curved M、Audi Virtual Cockpit Plus RS e-tron GT、Volvo EX90)
- 純黑 OLED 校準 (#000000)，在 Switch OLED 螢幕上享有極致對比度
- 100% 零搖桿全觸控操作與全螢幕沉浸體驗
- 附帶可直接於 Switch 大氣層環境執行的 gt7_switch_hud.nro！"

echo "=== 發布完成！ ==="
