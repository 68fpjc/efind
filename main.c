#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "efind.h"

/**
 * @brief ヘルプメッセージを出力する関数
 */
static void print_help(void) {
  printf(
      "Usage: efind [starting-point...] [expression]\n\n"
      "Options:\n"
      "  -maxdepth LEVELS   Maximum directory depth to search\n"
      "  -type TYPE         File type to search for\n"
      "                     (f: file, d: directory, l: symbolic link, x: "
      "executable)\n"
      "  -name PATTERN      Search for files matching PATTERN (case "
      "insensitive)\n"
      "  -iname PATTERN     Same as -name, case insensitive\n"
      "  -o                 OR operator to combine conditions\n"
      "  --help, -help      Display this help message\n"
      "  --version, -version Display version information\n");
}

/**
 * @brief バージョン情報を出力する関数
 */
static void print_version(void) {
  printf("efind version " VERSION " https://github.com/68fpjc/efind\n");
}

/**
 * @brief コマンドライン引数を解析する関数
 *
 * @param[in] argc コマンドライン引数の数
 * @param[in] argv コマンドライン引数の配列
 * @param[out] opts 解析結果を格納するためのオプション構造体へのポインタ
 * @param[out] start_dir 開始ディレクトリを格納するためのポインタ
 * @return 成功時は 1 、エラー時は 0 を返す
 */
static int parse_args(int argc, char *argv[], Options *opts, char **start_dir) {
  // デフォルト値の設定
  opts->maxdepth =
      -1;  // 最大深さのデフォルト値を設定 (-1 は制限なしを意味する)
  opts->condition_count = 0;  // 条件の数を初期化
  *start_dir = ".";           // 開始ディレクトリのデフォルト値を設定

  // コマンドライン引数がない場合はデフォルト設定で実行する
  if (argc < 2) {
    return 1;  // 引数なしでも成功として扱う
  }

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-help") == 0) {
      print_help();
      return 0;
    } else if (strcmp(argv[i], "--version") == 0 ||
               strcmp(argv[i], "-version") == 0) {
      print_version();
      return 0;
    } else if (strcmp(argv[i], "-maxdepth") == 0) {
      if (i + 1 < argc) {
        opts->maxdepth = atoi(argv[++i]);
      } else {
        fprintf(stderr, "Error: -maxdepth requires an argument\n");
        return 0;
      }
    } else if (strcmp(argv[i], "-type") == 0) {
      if (i + 1 < argc) {
        // 条件数のチェックを追加
        if (opts->condition_count >= MAX_CONDITIONS) {
          fprintf(stderr, "Error: Too many conditions (maximum is %d)\n",
                  MAX_CONDITIONS);
          return 0;
        }

        Condition *cond = &opts->conditions[opts->condition_count++];
        cond->pattern = NULL;
        cond->op = OP_AND;
        cond->ignore_case = 0;  // デフォルトは大文字小文字を区別する

        char type = argv[++i][0];
        switch (type) {
          case 'f':
            cond->type = TYPE_FILE;
            break;
          case 'd':
            cond->type = TYPE_DIR;
            break;
          case 'l':
            cond->type = TYPE_SYMLINK;
            break;
          case 'x':
            cond->type = TYPE_EXECUTABLE;
            break;
          default:
            fprintf(stderr, "Error: invalid type '%c'\n", type);
            return 0;
        }
      } else {
        fprintf(stderr, "Error: -type requires an argument\n");
        return 0;
      }
    } else if (strcmp(argv[i], "-name") == 0 ||
               strcmp(argv[i], "-iname") == 0) {
      if (i + 1 < argc) {
        // 条件数のチェックを追加
        if (opts->condition_count >= MAX_CONDITIONS) {
          fprintf(stderr, "Error: Too many conditions (maximum is %d)\n",
                  MAX_CONDITIONS);
          return 0;
        }

        Condition *cond = &opts->conditions[opts->condition_count++];
        cond->pattern = argv[++i];
        cond->type = TYPE_NONE;
        cond->op = OP_AND;

        // -name と -iname で大文字小文字の区別フラグを設定
        cond->ignore_case = (strcmp(argv[i - 1], "-iname") == 0) ? 1 : 0;
      } else {
        fprintf(stderr, "Error: %s requires an argument\n", argv[i]);
        return 0;
      }
    } else if (strcmp(argv[i], "-o") == 0) {
      if (opts->condition_count > 0) {
        opts->conditions[opts->condition_count - 1].op = OP_OR;
      } else {
        fprintf(stderr, "Error: -o cannot be the first condition\n");
        return 0;
      }
    } else if (argv[i][0] != '-') {
      *start_dir = argv[i];  // 開始ディレクトリと仮定する
    }
  }

  return 1;
}

/**
 * @brief プログラムのエントリーポイント
 *
 * @param[in] argc コマンドライン引数の個数
 * @param[in] argv コマンドライン引数の配列
 * @return int プログラムの終了ステータス
 */
int main(int argc, char *argv[]) {
  Options opts;
  char *start_dir;

  if (!parse_args(argc, argv, &opts, &start_dir)) {
    return 1;
  }

  return search_directory(start_dir, 0, &opts);
}
