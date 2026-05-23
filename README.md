# C Algorithms Practice

## 📌 プロジェクト概要

このリポジトリは、C 言語で実装したアルゴリズム・データ構造の演習コードを収録しています。

主に以下の内容を含みます。

- 汎用データ構造実装 (`src/utils`)
- ソート・検索・配列操作などの基本アルゴリズム
- テストおよびベンチマーク用コード

---

## 📁 ディレクトリ構成

- `src/`
  - `array/` - 配列関連のアルゴリズム
  - `sort/` - 基本ソート実装
  - `tree/` - 木構造実装（AVL など）
  - `utils/` - 汎用データ構造ライブラリ
  - `tests/` - テストおよびベンチマークコード

- `benchmark-bin/` - ベンチマーク実行ファイル出力先
- `build/` - CMake ビルド成果物

---

## 🔧 `src/utils` の主な機能

- `dynarray` - 汎用動的配列
- `hashtable` - ハッシュテーブル
- `linked_list` - 単方向 / 双方向 / 循環リスト
- `stack` - 型安全スタック
- `queue_array` - 配列ベースの循環キュー
- `queue_list` - リストベースのキュー
- `skiplist` - スコア付きスキップリスト
- `zset` - Redis 風のソート済み集合 (ZSet)

---

## ⚙️ ビルドと実行

### Makefile を使う場合

```sh
make          # main と test_zset をビルド
make run      # main を実行
make test     # test_zset を実行
make benchmark # benchmark-bin をビルドして実行
make clean    # 生成物を削除
```

### デバッグビルド

```sh
make debug
```

### リリースビルド

```sh
make release
```

---

## 📌 追加情報

- `test_zset` では `src/utils` の ZSet 実装を検証します。
- `benchmark-bin` はパフォーマンス計測用のバイナリです。
- `CMakeLists.txt` も用意されていて、CMake でのビルドも可能です。

