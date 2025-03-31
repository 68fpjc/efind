# efind - エセ find

## これはなに？

X68000 + Human68k 用の GNU find モドキです。 find は先人たちによる移植版がいくつか存在しますが、自分向けに作りました。

## サポートされているオプション

- `-maxdepth LEVELS` : 検索を指定された深さに制限
- `-type TYPE` : 検索するファイルタイプを指定 ( `f` : 通常ファイル / `d` : ディレクトリ)
- `-name PATTERN` `-iname PATTERN` : 指定されたパターンに一致するファイル名を検索
- `-o` : 条件を論理 OR 演算子で結合
- `--help` / `-help` : ヘルプメッセージを表示
- `--version` / `-version` : バージョン情報を表示

なお、 [(V)TwentyOne.sys](https://github.com/kg68k/twentyonesys) が組み込まれ、かつ `+C` が設定されている場合、 `-name` は大文字 / 小文字を区別します。

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
- 空のドライブを指定するとエラーになる
- その他、いろいろ

## ソースコードからのビルド

X68000 ではビルドできません。 [elf2x68k](https://github.com/yunkya2/elf2x68k) が必要です。 makefile のあるディレクトリで `make` してください。

## ライセンス

`libmb` ディレクトリについては [libmb/README.md](libmb/README.md) を参照。

## 連絡先

https://github.com/68fpjc/efind
