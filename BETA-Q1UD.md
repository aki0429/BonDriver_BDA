# BonDriver_BDA 改 — Q1UD 安定化 β版 (beta/q1ud-stability)

> **β版です。** [radi-sh/BonDriver_BDA](https://github.com/radi-sh/BonDriver_BDA) をフォークし、受信バッファまわりを修正したものです。
> **合成テストと x64 ビルドまでは確認済みです。実機の PX-Q1UD / PX-S1UD（Windows）では、まだ動作を確認していません。**
> 自己責任で使い、結果を Issue で知らせてもらえると助かります。

## 対象

- Windows の BDA 互換チューナー（PX-Q1UD / PX-S1UD など、BDA ドライバで動くもの）。
- **Ubuntu などの Linux では使えません。** BDA は Windows（DirectShow）の仕組みです。Linux の Q1UD 対策は [siano-tsbuf](https://github.com/aki0429/siano-tsbuf) を使ってください。

## 変更点（upstream `c91e501` から）

| # | 問題 | 修正 |
|---|---|---|
| 1 | `CTsWriter::Write()` が **188 バイト未満のデータを黙って捨てていた**。DirectShow のサンプル境界は TS パケット境界と一致しないため、ドライバが細切れに渡すと TS の一部が消え、ドロップや同期ずれになる | 長さが正なら必ず受け取って後段に渡す（`tswriter.cpp`） |
| 2 | 受信キューが溢れたとき、古いデータを**記録なしに**捨てていた（ドロップの原因がアプリ側か分からない） | 捨てたバイト数を累積する `TS_BUFF::GetDroppedBytes()` を追加し、溢れ処理を1か所に集約 |
| 3 | ini で `MaxBuffCount=0` を指定すると、空のキューを削除しようとして**クラッシュ**する | 最低 1 に丸める |
| 4 | `TS_BUFF::Size()` がロックなしで `std::queue` を読んでいた（データ競合） | ロック内で読むように修正 |

いずれも **受信したバイトを失わない・溢れを観測できる** ようにする修正で、チューニングや ini の書式は変えていません。既存の ini をそのまま使えます。

## ドロップが多いときの推奨設定（ini の `[BonDriver]` セクション）

```ini
; 1 ブロックのサイズ (×188 バイト)。既定 1024
BuffSize=1024
; キューに溜められるブロック数。読み手（TVTest/EDCB）が一時的に止まる環境では増やす。既定 512 (約 98MB)
MaxBuffCount=1024
```

## CATV・スカパー!プレミアムサービス光（実験）

PX-S1UD で **スカパー!プレミアムサービス光**（J.83B 256QAM、NID 0x000A）を 580.75MHz で受信できたという報告がありました。
同じ Siano 系の PX-Q1UD でも試せるように、設定と小さな拡張を入れています。**Q1UD での受信は未確認です。**

- サンプル: [`doc/BonDriver_BDA-Q1UD-CATV-sample.ini`](doc/BonDriver_BDA-Q1UD-CATV-sample.ini)（Shift_JIS）
  - `UHF`: 地デジ 13〜52ch
  - `CATV`: パススルー C13〜C63ch（ISDB-T のまま。CATV 局が地デジをパススルーで流していれば、これだけで映るはず）
  - `光`: スカパー!プレミアムサービス光 H001〜H105（J.83B 256QAM、5.6Msps）
  - `トラモジ`: CATV トランスモジュレーション（J.83C 64QAM/256QAM）
- 新オプション `ChannelSettingsAutoOptions="HExt"`（作成空間 `OPTICAST-EXT`）
  - 従来の自動生成は H001〜H058（〜463.75MHz）まで。報告の 580.75MHz はその外（同じ 6.5MHz 間隔で H076）なので、H059〜H105（470.25〜769.25MHz）を追加で作れるようにしました。
  - 指定しなければ従来どおりで、既存の ini には影響しません。
- QAM でロックしないときは、サンプル内のコメントにある `DVBSystemType1="DVB-C"` + `DVBSystemTypeNumber=1` を試してください。

**Linux（Mirakurun + dvbv5）では、光とトラモジは使えません。** カーネルの `smsdvb` ドライバは ISDB-T / DVB-T しか扱わず、QAM の設定手段がありません。
CATV パススルー（C13〜C63ch の ISDB-T）は Linux でも、今の構成でチャンネル設定を足せば受信できます。

## テスト

`tests/stream_regression.cpp` は、実際の `CTsWriter` と `TS_BUFF` を合成データで動かすテストです（実機は不要）。

| テスト | upstream | β版 |
|---|---|---|
| `callback_fragment_exact`（187+1+376 バイトに分割して入力） | **FAIL**（564 バイト中 376 しか届かない） | PASS |
| `bounded_overflow_observable`（溢れ量を記録できるか） | **FAIL**（dropped=0） | PASS |
| `zero_capacity_safe`（`MaxBuffCount=0`） | **クラッシュ**（0xC0000005） | PASS |

実行方法: x64 Native Tools コマンドプロンプトで `tests\run-regression.cmd` を実行します。VS 2019 以外の場合は、先に `set PLATFORM_TOOLSET=v145` のように使うツールセットを指定してください。

## ビルド

`git submodule update --init` を実行してから、`BonDriver_BDA.sln` を `Release_Static|x64` でビルドします。
作者の環境では VS 18 / ツールセット v145 でビルドを確認しました。プロジェクトの既定は v142 なので、必要なら `/p:PlatformToolset=` で変更してください。

## 未検証・既知の限界

- 実機の Q1UD / S1UD（Windows）での録画テストはしていません。
- 電波・USB・チップ側の欠損（Siano の 13ch 問題など）は直せません。
- x86 / XP 向け構成はビルドしていません。
