# C++ AI を Python 経由で対戦させる手順

この手順は、`/mygame` で作成した C++ の AI を Python の起動コマンドとして使い、`viewer` で対戦を確認するためのものです。

## 目的

- C++ で作成した AI を Python の実行コマンドとして呼び出す
- 同じ C++ AI を 2 体起動して対戦させる
- `viewer` で盤面の変化を確認する

## 使うコマンド

- 1体目: `mygame_ai`
- 2体目: `mygame_ai_2`
- 対戦起動: `start_blocksduo`

## 事前準備

1. プロジェクトルートへ移動する

```bash
cd ~/42tokyo-blocks-2026-0527
```

2. Python の仮想環境を有効化する

```bash
uv venv
source ./.venv/bin/activate
```

`source ./.venv/bin/activate` でエラーになる場合は、先に `uv venv` を実行して `.venv` を作成してください。

3. `mygame` をインストールする

```bash
uv pip install ./mygame
```

4. viewer の依存関係を入れる

```bash
cd ~/42tokyo-blocks-2026-0527/viewer
npm install
```

## ビューアー起動

別ターミナルで viewer を起動する。

```bash
cd ~/42tokyo-blocks-2026-0527/viewer
npm run start:live
```

## 対戦の起動

プロジェクトルートへ戻って、2つの AI を指定して起動する。

```bash
cd ~/42tokyo-blocks-2026-0527
source ./.venv/bin/activate
start_blocksduo mygame_ai mygame_ai_2 3 view
```

## 補足

- `mygame_ai` は通常版の C++ バイナリを使う
- `mygame_ai_2` は alt 版の C++ バイナリを使う
- `view` を付けると viewer と接続して盤面を確認できる
- 牌譜の JSON は `log/` に保存される

## うまく動かない時

- `mygame_ai` と `mygame_ai_2` が見つからない場合は、`uv pip install ./mygame` をやり直す
- `viewer` が開かない場合は、`npm install` と `npm run start:live` の実行場所を確認する
- `blocksstate` が無い場合は、`mygame` ディレクトリで C++ ビルドが通っているか確認する
