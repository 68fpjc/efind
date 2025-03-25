# efind - エセ find

## これはなに？

X68000 + Human68k 用の GNU find モドキです。 find は先人たちによる移植版がいくつか存在しますが、自分向けに作りました。

GitHub Copilot + Claude 3.7 Sonnet で雑に作ったものです。

## サポートされているオプション

- `-maxdepth LEVELS` : 検索を指定された深さに制限
- `-type TYPE` : 検索するファイルタイプを指定 ( `f` : 通常ファイル / `d` : ディレクトリ)
- `-name PATTERN` `-iname PATTERN` : 指定されたパターンに一致するファイル名を検索
- `-o` : 条件を論理 OR 演算子で結合
- `--help` / `-help` : ヘルプメッセージを表示
- `--version` / `-version` : バージョン情報を表示

## 使用例

```bash
# カレントディレクトリ以下のすべてのファイル / ディレクトリを検索
efind . -type d

# カレントディレクトリ以下の .txt を検索
efind . -name '*.txt'

# カレントディレクトリ以下の .c または .h を検索
efind . -name '*.c' -o -name '*.h'

# 深さ2までのディレクトリで .txt を検索
efind . -maxdepth 2 -name '*.txt'
```

## issues

気が向いたら直します。

- 遅い
- 2 バイト文字を使用したファイル名 / ディレクトリ名で誤動作する
- パターンマッチング処理が雑
- 実は `-name` と `-iname` は同じ実装。 TwentyOne +C 環境では期待通りに動かない
- シンボリックリンクに対応していない
- 仮想ディレクトリでエラーになる
- その他、いろいろ

## ソースコードからのビルド

X68000 ではビルドできません。 [elf2x68k](https://github.com/yunkya2/elf2x68k) が必要です。 makefile のあるディレクトリで `make` してください。

## 連絡先

https://github.com/68fpjc/efind
