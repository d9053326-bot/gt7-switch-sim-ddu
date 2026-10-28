# GT7 Switch Sim-DDU Pro (日本語)
**バージョン: `_aw_1.10.28.2026`**

[繁體中文](README_ZH.md) | [English](README_EN.md) | [Deutsch](README_DE.md)

**Nintendo Switch (初期型・バッテリー強化版・Switch OLED / Atmosphère 大気層 CFW 環境)** 向けに開発された、ネイティブ Homebrew レーシング＆EV デジタルメーターアプリ（`.nro`）です。Wi-Fi 経由で **PS5 / PS5 Pro の『グランツーリスモ7』(GT7)** からブロードキャストされる 60Hz UDP テレメトリーデータを受信します。Switch 本体の左右 Joy-Con を取り外し、ステアリングコントローラー（ハンコン）のベース直前に設置することで、超低遅延・高精度な本格派 **Sim Racing DDU (Dashboard Display Unit)** として活用できます。

---

## 🏎️ リアルタイム・テレメトリー動態デモ

![リアルタイム・テレメトリー動態デモ](assets/screenshots/telemetry_demo.gif)

---

## 🏆 8 つの本格モータースポーツ＆高級 EV デジタルメーターモード

### 1. 最新高級 EV 4 大ブランド・コックピット (`3: EV`)
| Mercedes-Benz MBUX ハイパースクリーン EQ | BMW i-Curved ディスプレイ iDrive 9 M-EV |
| :---: | :---: |
| ![Benz MBUX EQ](assets/screenshots/03_mode_ev_benz_eq.png) | ![BMW i-Curved](assets/screenshots/04_mode_ev_bmw_i.png) |
| **Audi バーチャル コックピット プラス e-tron GT** | **Volvo EX90 スカンジナビア・ミニマリズム** |
| ![Audi e-tron GT](assets/screenshots/05_mode_ev_audi_etron.png) | ![Volvo EX90](assets/screenshots/06_mode_ev_volvo_ex.png) |

- **Mercedes-Benz MBUX ハイパースクリーン EQ**: 浮遊感のあるデュアル円形メーター、シアンネオンリング、0〜100% の電力出力および回生ブレーキ（Recuperation）、PRND シフトインジケーター、中央 MBUX レーダースクリーン。
- **BMW i-Curved ディスプレイ (iDrive 9 ///M Electric)**: シャープな多角形幾何学デザイン、BMW M エレクトリックブルー、全開加速時の `>> eBOOST <<` フラッシュ演出、リアルタイム G-Force ベクトルメーター。
- **Audi バーチャル コックピット プラス (e-tron GT RS)**: クラシックな Audi Sport 赤色針スケール、0〜100% ブースト＆回生サークル、MMI レースサーキットマップと理論最速セクター解析。
- **Volvo EX90 / Polestar ミニマリズム**: 北欧スカンジナビアの洗練されたミニマルラグジュアリーデザイン、中央の高コントラスト大型速度表示、縦型リボン式エネルギーフローメーター。

---

### 2. GT7 公式純正 HUD＆高解像度デジタルタコメーター
| Mode 1: GT7 公式純正 HUD & MFD | Mode 2: 高解像度デジタルタコメーター DDU |
| :---: | :---: |
| ![GT7 Official HUD](assets/screenshots/01_mode_gt7_official.png) | ![Digital Tachometer](assets/screenshots/02_mode_digitach.png) |

- **Mode 1: GT7 公式純正 HUD & MFD (`1: GT7`)**: ポリフォニー・デジタル純正 GT7 の美しい曲線ホライゾン・レブバー（46 段階グラデーション）、シフトアップ警告点滅、推奨ギア表示、タッチで切り替え可能な 4 ページ MFD を完全再現。
- **Mode 2: 高解像度デジタルタコメーター DDU (`2: DIGI`)**: 上部 32 段階の滑らかなシフトランプ、数値 RPM、最大ブレーキ踏力メモリー付きペダルゲージ、個別タイヤ温度、リアルタイムデルタタイム表示。

---

### 3. プロフェッショナル・モータースポーツモード
| Mode 4: F1 ステアリング・ディスプレイ | Mode 5: GT3 MoTeC 耐久レース DDU |
| :---: | :---: |
| ![F1 Steering Wheel](assets/screenshots/07_mode_f1_wheel.png) | ![GT3 MoTeC](assets/screenshots/08_mode_gt3_motec.png) |
| **Mode 6: WRC ラリーステージ・コックピット** | **Mode 7: JDM Defi Advance ZD 追加メーター** |
| ![WRC Rally](assets/screenshots/09_mode_rally_wrc.png) | ![JDM Defi](assets/screenshots/10_mode_jdm_defi.png) |

- **Mode 4: F1 ステアリング (`4: F1`)**: 現代 F1 マシン（Mercedes W14 / Ferrari SF23 PCU-8D）を忠実に模したステアリング液晶画面。
- **Mode 5: GT3 MoTeC (`5: GT3`)**: 耐久レーシングカーに標準採用される MoTeC C1212 スイープタコメーター＆燃料残量ラップ計算。
- **Mode 6: WRC ラリー (`6: WRC`)**: ロールケージ越しの視認性に特化したラリー HUD、ロール＆ピッチ角を計測する車体傾斜儀を搭載。
- **Mode 7: JDM Defi (`7: JDM`)**: 日本のチューニングカーでおなじみのアナログ針式ブースト計＆タコメーター、Advance ZD 有機 EL 風マルチメーター。
- **Mode 8: GPS サーキットマップ (`8: MAP`)**: 2D GPS サーキットコース図、加減速ヒートマップ、最適セクタータイム比較、ピットストップ戦略計算。

---

## 👆 完全ジョイコン不要・マルチタッチ＆全画面キャンバス

| PS5 IP タッチテンキーモーダル | フルスクリーン・没入キャンバス |
| :---: | :---: |
| ![PS5 IP Modal](assets/screenshots/12_ip_touch_modal.png) | ![Fullscreen Canvas](assets/screenshots/13_fullscreen_canvas.png) |

- **全画面フルスクリーン (`FULL`)**: 上下のバーが瞬時に格納され、Switch の 720p 画面を余白なく 100% フル活用。左下タッチでいつでも通常表示に復帰可能。
- **デフォルト自動探索 (`IP: AUTO`)**: 起動時に UDP ブロードキャストで PS5 / PS5 Pro を自動検知。手動入力不要ですが、タップして大画面テンキーから直接入力することも可能。
- **ピュア OLED ブラック校準**: 発光部以外の背景はすべて `#000000` に設定されており、Switch OLED ディスプレイの無限のコントラスト比を最大限に引き出します。

---

## 🎮 コントロール・ショートカット一覧

| 機能 | Switch タッチ操作（Joy-Con 不要） | Switch 物理ボタン | PC / Mac シミュレーター |
| :--- | :--- | :--- | :--- |
| **8 大モード切り替え** | 画面下部 Dock タップ / 左右スワイプ | L / R、十字キー左右 | `1`〜`8`、`L` / `R` キー |
| **EV ブランド切り替え** | 画面上部 4 大ブランドタグをタップ | Y ボタン（EV モード時） | `E` キー |
| **全画面切り替え** | 右上 `FULLSCREEN` / 下部 `FULL` | - ボタン (Minus) | `F` または `F11` キー |
| **PS5 IP 設定** | 下部 `IP: AUTO` タップでテンキー表示 | X ボタン | `I` または `X` キー |
| **Demo / 実機切り替え** | 下部 `DEMO ON` タップ | Y ボタン（非 EV モード時） | `D` または `Y` キー |
| **時速単位 (KM/H / MPH)** | 下部 `KM/H` タップ | A ボタン | `U` または `A` キー |
| **アプリ終了** | 右上赤色の `EXIT APP` タップ | + ボタン (Plus) | `Q` または `ESC` キー |
| **サーキット切替 (Demo)** | サーキットマップ領域をタップ | — | `C` キー |
| **ラップタイムリセット** | 上部 `RESET LAPS` タップ | — | `Backspace` キー |

---

## 📦 インストール方法 (Nintendo Switch Atmosphère)

1. [GitHub Releases](https://github.com/d9053326-bot/gt7-switch-sim-ddu/releases) から最新の `gt7_switch_hud.nro` をダウンロードします。
2. Switch の microSD カードを PC に接続し、以下のパスにコピーします：
   ```
   /switch/gt7_hud/gt7_switch_hud.nro
   ```
   *(または `/switch/gt7_switch_hud.nro` の直下でも可)*
3. microSD カードを Switch に戻し、Atmosphère（大気層）を起動します。
4. アルバム（Homebrew Menu）を開き、**GT7 Sim-DDU** アイコンをタップして起動します。
5. Switch と PS5 が同じ Wi-Fi ネットワークに接続されていることを確認し、GT7 でコースに出ると自動的に 60Hz リアルタイムデータを受信開始します！
