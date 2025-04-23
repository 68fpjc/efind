# efind - エセ find

## これはなに？

X68000 + Human68k 用の GNU find モドキです。 find は先人たちによる移植版がいくつか存在しますが、自分向けに作りました。

## サポートされているオプション

- `-maxdepth LEVELS` : 検索を指定された深さに制限
- `-type TYPE` : 検索するファイルタイプを指定 ( `f` : 通常ファイル / `d` : ディレクトリ / `l` : シンボリックリンク / `x` : 実行属性ファイル )
- `-name PATTERN` `-iname PATTERN` : 指定されたパターンに一致するファイル名を検索
- `-o` : 条件を論理 OR 演算子で結合
- `--help` / `-help` : ヘルプメッセージを表示
- `--version` / `-version` : バージョン情報を表示

なお、 [(V)TwentyOne.sys](https://github.com/kg68k/twentyonesys) が組み込まれ、かつ `+C` が設定されている場合、 `-name` は大文字 / 小文字を区別します。

シンボリックリンクの検索 ( `-type l` ) および実行属性ファイルの検索 ( `-type x` ) に仮対応しました。ですが、重いのであまり使わないほうがいいと思います。

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

- 空のドライブを指定するとエラーになる
- `-type f` を指定したときにシンボリックリンクもヒットする
- その他、いろいろ

## ソースコードからのビルド

X68000 ではビルドできません。 [elf2x68k](https://github.com/yunkya2/elf2x68k) が必要です。 makefile のあるディレクトリで `make` してください。

## 連絡先

https://github.com/68fpjc/efind
