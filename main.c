/*
 * efind - simple implementation of GNU find subset
 * Supports: -maxdepth, -type, -name, -iname, -o, --help, -help, --version,
 * -version
 */

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define VERSION "0.1.0"
#define MAX_PATH 1024

typedef enum { OP_NONE, OP_AND, OP_OR } Operator;

typedef enum { TYPE_NONE, TYPE_FILE, TYPE_DIR } FileType;

typedef struct {
  char *pattern;
  FileType type;
  Operator op;
} Condition;

typedef struct {
  int maxdepth;
  int condition_count;
  Condition conditions[100];  // 単純化のため固定サイズ
} Options;

void print_help() {
  printf("Usage: efind [starting-point...] [expression]\n\n");
  printf("Options:\n");
  printf("  -maxdepth LEVELS   Maximum directory depth to search\n");
  printf(
      "  -type TYPE         File type to search for (f: file, d: directory)\n");
  printf(
      "  -name PATTERN      Search for files matching PATTERN (case "
      "insensitive)\n");
  printf("  -iname PATTERN     Same as -name, case insensitive\n");
  printf("  -o                 OR operator to combine conditions\n");
  printf("  --help, -help      Display this help message\n");
  printf("  --version, -version Display version information\n");
}

void print_version() { printf("efind version %s\n", VERSION); }

// 独自のパターンマッチング関数
int match_pattern(const char *pattern, const char *string) {
  const char *p = pattern;
  const char *s = string;
  const char *p_backup = NULL;
  const char *s_backup = NULL;

  while (*s) {
    if (*p == '*') {
      // '*'の場合、パターンを1つ進めて、現在位置をバックアップ
      p_backup = ++p;
      s_backup = s;

      // '*'が最後のパターンなら、常にマッチ
      if (*p == '\0') return 1;
    } else if (*p == '?' || *p == *s) {
      // '?'か文字が一致する場合は次の文字へ
      p++;
      s++;
    } else if (p_backup) {
      // 不一致でバックアップがある場合は、バックアップ位置から再開
      p = p_backup;
      s = ++s_backup;
    } else {
      // 不一致でバックアップがない場合はマッチしない
      return 0;
    }
  }

  // 残りのパターンが全て'*'なら成功
  while (*p == '*') p++;

  // パターンの終わりまで来たらマッチ
  return *p == '\0';
}

// 大文字小文字を無視したパターンマッチング関数
int match_pattern_case_insensitive(const char *pattern, const char *string) {
  const char *p = pattern;
  const char *s = string;
  const char *p_backup = NULL;
  const char *s_backup = NULL;

  while (*s) {
    if (*p == '*') {
      // '*'の場合、パターンを1つ進めて、現在位置をバックアップ
      p_backup = ++p;
      s_backup = s;

      // '*'が最後のパターンなら、常にマッチ
      if (*p == '\0') return 1;
    } else if (*p == '?' ||
               tolower((unsigned char)*p) == tolower((unsigned char)*s)) {
      // '?'か文字が一致する場合は次の文字へ (大文字小文字を区別しない)
      p++;
      s++;
    } else if (p_backup) {
      // 不一致でバックアップがある場合は、バックアップ位置から再開
      p = p_backup;
      s = ++s_backup;
    } else {
      // 不一致でバックアップがない場合はマッチしない
      return 0;
    }
  }

  // 残りのパターンが全て'*'なら成功
  while (*p == '*') p++;

  // パターンの終わりまで来たらマッチ
  return *p == '\0';
}

int evaluate_conditions(const char *path, struct stat *st, Options *opts) {
  if (opts->condition_count == 0) {
    return 1;  // No conditions means match everything
  }

  int result = 0;
  Operator current_op = OP_AND;

  for (int i = 0; i < opts->condition_count; i++) {
    int match = 1;
    Condition *cond = &opts->conditions[i];

    // Check file type
    if (cond->type != TYPE_NONE) {
      if (cond->type == TYPE_FILE && !S_ISREG(st->st_mode)) {
        match = 0;
      } else if (cond->type == TYPE_DIR && !S_ISDIR(st->st_mode)) {
        match = 0;
      }
    }

    // Check name pattern with custom pattern matching
    if (cond->pattern != NULL) {
      const char *basename = strrchr(path, '/');
      basename = basename ? basename + 1 : path;
      if (!match_pattern_case_insensitive(cond->pattern, basename)) {
        match = 0;
      }
    }

    // Apply operator logic
    if (i == 0) {
      result = match;
    } else if (current_op == OP_AND) {
      result = result && match;
    } else if (current_op == OP_OR) {
      result = result || match;
    }

    // Set operator for next condition
    current_op = cond->op;
  }

  return result;
}

/**
 * パスがルートディレクトリかどうかを判定する関数
 * Human68kでは n:\ のようなドライブ指定のルートや / や \ も対象
 */
int is_root_directory(const char *path) {
  // ケース1: "X:\" または "X:/" 形式（ドライブレター+ルート）
  if (strlen(path) == 3 && isalpha((unsigned char)path[0]) && path[1] == ':' &&
      (path[2] == '\\' || path[2] == '/')) {
    return 1;
  }

  // ケース2: "/" または "\" 形式（ルートディレクトリ）
  if (strcmp(path, "/") == 0 || strcmp(path, "\\") == 0) {
    return 1;
  }

  return 0;
}

/**
 * 2つのパスを連結する関数
 * スラッシュの重複を防止し、Human68k のパス形式を考慮
 */
void join_paths(char *dest, size_t dest_size, const char *dir,
                const char *file) {
  // ベースディレクトリが空か終端がスラッシュかバックスラッシュ
  const size_t dir_len = strlen(dir);
  if (dir_len == 0) {
    strncpy(dest, file, dest_size - 1);
    dest[dest_size - 1] = '\0';
    return;
  }

  // ルートディレクトリの場合は特別処理
  if (is_root_directory(dir)) {
    // ドライブ指定のルートの場合（例: "n:\"）
    if (dir_len == 3 && isalpha((unsigned char)dir[0]) && dir[1] == ':') {
      snprintf(dest, dest_size, "%c:%c%s", dir[0], dir[2], file);
    } else {
      // UNIXスタイルのルート
      snprintf(dest, dest_size, "/%s", file);
    }
    return;
  }

  // 通常のディレクトリの場合
  char last_char = dir[dir_len - 1];
  if (last_char == '/' || last_char == '\\') {
    snprintf(dest, dest_size, "%s%s", dir, file);
  } else {
    snprintf(dest, dest_size, "%s/%s", dir, file);
  }
}

/**
 * ディレクトリエントリを保持する構造体
 */
typedef struct {
  char name[256];  // ファイル名（最大長を確保）
  int is_dir;      // ディレクトリかどうかのフラグ
} DirEntry;

void search_directory(const char *base_dir, int current_depth, Options *opts) {
  DIR *dir;
  struct dirent *entry;
  char path[MAX_PATH];
  struct stat statbuf;
  DirEntry *entries = NULL;
  int entry_count = 0;
  int entry_capacity = 0;

  // 開始ディレクトリ（current_depth == 0）のときの特別処理
  if (current_depth == 0) {
    int should_print_base_dir = 0;

    // -type d が指定されているかチェック
    for (int i = 0; i < opts->condition_count; i++) {
      if (opts->conditions[i].type == TYPE_DIR) {
        should_print_base_dir = 1;
        break;
      }
    }

    // 条件を満たしていて、ルートディレクトリでなければ出力
    if (should_print_base_dir && !is_root_directory(base_dir)) {
      // ここでは深い検査せず、単に出力する（簡易実装）
      printf("%s\n", base_dir);
    }
  }

  if (opts->maxdepth >= 0 && current_depth > opts->maxdepth - 1) {
    return;
  }

  // ディレクトリを開いてエントリを収集
  if ((dir = opendir(base_dir)) == NULL) {
    fprintf(stderr, "Cannot open directory '%s': %s\n", base_dir,
            strerror(errno));
    return;
  }

  // 収集するエントリ用の初期メモリを確保
  entry_capacity = 32;  // 初期容量
  entries = (DirEntry *)malloc(sizeof(DirEntry) * entry_capacity);
  if (entries == NULL) {
    fprintf(stderr, "Memory allocation error\n");
    closedir(dir);
    return;
  }

  // ディレクトリエントリを読み込む（"."と".."を除く）
  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    // エントリリストの拡張が必要な場合
    if (entry_count >= entry_capacity) {
      entry_capacity *= 2;
      DirEntry *new_entries =
          (DirEntry *)realloc(entries, sizeof(DirEntry) * entry_capacity);
      if (new_entries == NULL) {
        fprintf(stderr, "Memory allocation error during expansion\n");
        break;
      }
      entries = new_entries;
    }

    // エントリ名をコピー
    strncpy(entries[entry_count].name, entry->d_name,
            sizeof(entries[entry_count].name) - 1);
    entries[entry_count].name[sizeof(entries[entry_count].name) - 1] = '\0';

    // パスを結合
    join_paths(path, sizeof(path), base_dir, entry->d_name);

    // ディレクトリかどうか確認
    if (stat(path, &statbuf) != -1 && S_ISDIR(statbuf.st_mode)) {
      entries[entry_count].is_dir = 1;
    } else {
      entries[entry_count].is_dir = 0;
    }

    entry_count++;
  }

  // ディレクトリハンドルはもう必要ないので閉じる
  closedir(dir);

  // 収集したエントリを処理
  for (int i = 0; i < entry_count; i++) {
    // パスを結合
    join_paths(path, sizeof(path), base_dir, entries[i].name);

    // ファイルの情報を取得
    if (stat(path, &statbuf) == -1) {
      fprintf(stderr, "Cannot stat '%s': %s\n", path, strerror(errno));
      continue;
    }

    // 条件を評価して、マッチすれば出力
    if (evaluate_conditions(path, &statbuf, opts)) {
      printf("%s\n", path);
    }

    // ディレクトリなら再帰的に処理
    if (entries[i].is_dir) {
      search_directory(path, current_depth + 1, opts);
    }
  }

  // メモリを解放
  free(entries);
}

int parse_args(int argc, char *argv[], Options *opts, char **start_dir) {
  if (argc < 2) {
    print_help();
    return 0;
  }

  opts->maxdepth = -1;  // No limit by default
  opts->condition_count = 0;
  *start_dir = ".";  // Default start directory

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
        Condition *cond = &opts->conditions[opts->condition_count++];
        cond->pattern = NULL;
        cond->op = OP_AND;

        char type = argv[++i][0];
        switch (type) {
          case 'f':
            cond->type = TYPE_FILE;
            break;
          case 'd':
            cond->type = TYPE_DIR;
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
        Condition *cond = &opts->conditions[opts->condition_count++];
        cond->pattern = argv[++i];
        cond->type = TYPE_NONE;
        cond->op = OP_AND;
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
      // Assume this is the start directory
      *start_dir = argv[i];
    }
  }

  return 1;
}

int main(int argc, char *argv[]) {
  Options opts;
  char *start_dir;

  if (!parse_args(argc, argv, &opts, &start_dir)) {
    return 1;
  }

  search_directory(start_dir, 0, &opts);

  return 0;
}
