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
 * @brief 検索パスのリストを保持する構造体
 */
typedef struct {
  char **paths;  // パスの配列
  int count;     // パスの数
  int capacity;  // 配列の容量
} PathList;

/**
 * @brief パスリストを初期化する関数
 *
 * @param[out] list 初期化するパスリスト
 * @return 成功時は 1、失敗時は 0
 */
static int initialize_path_list(PathList *list) {
  list->capacity = 8;  // 初期容量
  list->count = 0;
  list->paths = (char **)malloc(sizeof(char *) * list->capacity);

  if (list->paths == NULL) {
    fprintf(stderr, "Memory allocation error\n");
    return 0;
  }

  return 1;
}

/**
 * @brief パスリストにパスを追加する関数
 *
 * @param[in,out] list パスを追加するリスト
 * @param[in] path 追加するパス
 * @return 成功時は 1、失敗時は 0
 */
static int add_path(PathList *list, const char *path) {
  // 容量が不足している場合は拡張
  if (list->count >= list->capacity) {
    list->capacity *= 2;
    char **new_paths =
        (char **)realloc(list->paths, sizeof(char *) * list->capacity);

    if (new_paths == NULL) {
      fprintf(stderr, "Memory allocation error\n");
      return 0;
    }

    list->paths = new_paths;
  }

  // パスを追加
  list->paths[list->count++] = strdup(path);

  if (list->paths[list->count - 1] == NULL) {
    fprintf(stderr, "Memory allocation error\n");
    return 0;
  }

  return 1;
}

/**
 * @brief パスリストを解放する関数
 *
 * @param[in,out] list 解放するパスリスト
 */
static void free_path_list(PathList *list) {
  if (list->paths) {
    for (int i = 0; i < list->count; i++) {
      free(list->paths[i]);
    }
    free(list->paths);
  }
  list->paths = NULL;
  list->count = list->capacity = 0;
}

/**
 * @brief コマンドライン引数を解析する関数
 *
 * @param[in] argc コマンドライン引数の数
 * @param[in] argv コマンドライン引数の配列
 * @param[out] opts 解析結果を格納するためのオプション構造体へのポインタ
 * @param[out] paths 検索パスのリストへのポインタ
 * @return 成功時は 1 、エラー時は 0 を返す
 */
static int parse_args(int argc, char *argv[], Options *opts, PathList *paths) {
  // デフォルト値の設定
  opts->maxdepth =
      -1;  // 最大深さのデフォルト値を設定 (-1 は制限なしを意味する)
  opts->condition_count = 0;  // 条件の数を初期化

  // コマンドライン引数がない場合はカレントディレクトリを検索パスに設定
  if (argc < 2) {
    add_path(paths, ".");
    return 1;  // 引数なしでも成功として扱う
  }

  int found_search_path = 0;  // 検索パスが見つかったかどうかのフラグ

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
      // オプションでない引数は検索パスとして扱う
      if (!add_path(paths, argv[i])) {
        return 0;
      }
      found_search_path = 1;
    }
  }

  // 検索パスが見つからなかった場合はカレントディレクトリを設定
  if (!found_search_path) {
    add_path(paths, ".");
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
  PathList paths;
  int status = 0;

  // パスリストを初期化
  if (!initialize_path_list(&paths)) {
    return 1;
  }

  if (!parse_args(argc, argv, &opts, &paths)) {
    free_path_list(&paths);
    return 1;
  }

  // 複数の検索パスを処理
  for (int i = 0; i < paths.count; i++) {
    int result = search_directory(paths.paths[i], 0, &opts);
    if (result != 0) {
      status = result;
    }
  }

  // パスリストを解放
  free_path_list(&paths);

  return status;
}
