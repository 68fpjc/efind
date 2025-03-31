#include "efind.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <mbctype.h>
#include <mbstring.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arch.h"

#define MAX_PATH 1024  // パスの最大長

/**
 * @brief ディレクトリエントリを保持する構造体
 *
 * ディレクトリ内のエントリ情報を格納するために使用するフィールドにはエントリ名や属性などが含まれる
 */
typedef struct {
  char name[256];  // ファイル名 (最大長を確保)
  int is_dir;      // ディレクトリかどうかのフラグ
} DirEntry;

/**
 * @brief 大文字小文字を考慮したマルチバイト対応パターンマッチングを行う関数
 *
 * 指定されたパターンと文字列を比較し、大文字小文字の区別設定に従って一致するかどうかを判定する
 * マルチバイト文字に対応しています
 *
 * @param[in] pattern 比較対象のパターン (ヌル終端文字列)
 * @param[in] string チェック対象の文字列 (ヌル終端文字列)
 * @param[in] ignore_case 大文字小文字を区別しない場合は 1、区別する場合は 0
 * @param[in] fs_ignore_case ファイルシステムが大文字小文字を区別しない場合は
 * 1、区別する場合は 0
 * @return 一致する場合は非ゼロ値、一致しない場合はゼロを返す
 */
static int match_pattern(const char *pattern, const char *string,
                         int ignore_case, int fs_ignore_case) {
  // ファイルシステムが大文字小文字を区別しない場合は、常に大文字小文字を区別しない処理を行う
  if (fs_ignore_case) {
    ignore_case = 1;
  }

  const unsigned char *p = (const unsigned char *)pattern;
  const unsigned char *s = (const unsigned char *)string;
  const unsigned char *p_backup = NULL;
  const unsigned char *s_backup = NULL;

  while (*s) {
    // マルチバイト文字の処理
    if (ismbblead(*s)) {
      // マルチバイト文字の場合
      if (ismbblead(*p) && *s && *(s + 1) && *p && *(p + 1)) {
        // パターンもマルチバイト文字の場合は厳密比較
        if ((*s == *p) && (*(s + 1) == *(p + 1))) {
          s += 2;
          p += 2;
        } else if (*p == '*') {
          // '*' の処理はシングルバイト文字と同様
          p_backup = ++p;
          s_backup = s;
          if (*p == '\0') return 1;
        } else if (p_backup) {
          p = p_backup;
          s = ++s_backup;
        } else {
          return 0;
        }
      } else if (*p == '*') {
        // パターンが '*' の場合
        p_backup = ++p;
        s_backup = s;
        if (*p == '\0') return 1;
      } else if (*p == '?') {
        // '?' はマルチバイト文字 1 文字にもマッチする
        s += 2;  // マルチバイト文字なので 2 バイト進める
        p++;     // パターンは 1 文字分進める
      } else if (p_backup) {
        p = p_backup;
        s = ++s_backup;
      } else {
        return 0;
      }
    } else {
      // シングルバイト文字の場合
      if (*p == '*') {
        p_backup = ++p;
        s_backup = s;
        if (*p == '\0') return 1;
      } else if (*p == '?' || (ignore_case ? tolower((unsigned char)*p) ==
                                                 tolower((unsigned char)*s)
                                           : *p == *s)) {
        p++;
        s++;
      } else if (p_backup) {
        p = p_backup;
        s = ++s_backup;
      } else {
        return 0;
      }
    }
  }

  // 残りのパターンが全て '*' なら成功
  while (*p == '*') p++;

  // パターンの終わりまで来たらマッチ
  return *p == '\0';
}

/**
 * @brief 指定された条件を評価する関数
 *
 * ディレクトリエントリに基づいて、指定された条件を評価し、条件を満たすかどうかを判定する
 *
 * @param[in] entry 評価対象のディレクトリエントリ
 * @param[in] opts 評価基準を含む Options 構造体へのポインタ
 * @param[in] fs_ignore_case ファイルシステムが大文字小文字を区別しない場合は
 * 1、区別する場合は 0
 * @return 条件を満たす場合は 1 を返し、満たさない場合は 0 を返す
 */
static int evaluate_conditions(const DirEntry *entry, Options *opts,
                               int fs_ignore_case) {
  // 条件が指定されていない場合はすべて一致とみなす
  if (opts->condition_count == 0) {
    return 1;  // 条件がない場合はすべて一致
  }

  int result = 0;
  Operator current_op = OP_AND;

  // 各条件を順に評価
  for (int i = 0; i < opts->condition_count; i++) {
    int match = 1;
    const Condition *cond = &opts->conditions[i];

    // ファイルタイプのチェック
    if (cond->type != TYPE_NONE) {
      if (cond->type == TYPE_FILE && entry->is_dir) {
        match = 0;
      } else if (cond->type == TYPE_DIR && !entry->is_dir) {
        match = 0;
      }
    }

    // 名前パターンのチェック
    if (cond->pattern != NULL) {
      if (!match_pattern(cond->pattern, entry->name, cond->ignore_case,
                         fs_ignore_case)) {
        match = 0;
      }
    }

    // 演算子ロジックを適用
    if (i == 0) {
      result = match;
    } else if (current_op == OP_AND) {
      result = result && match;
    } else if (current_op == OP_OR) {
      result = result || match;
    }

    // 次の条件のために演算子を設定
    current_op = cond->op;
  }

  return result;
}

/**
 * @brief パスがルートディレクトリかどうかを判定する関数
 *
 * 指定されたパスがルートディレクトリであるかを判定する
 *
 * Human68k 環境では以下のようなパスがルートディレクトリとみなされる :
 *
 *   - ドライブ指定のルート (例: n:\)
 *
 *   - スラッシュやバックスラッシュのみのパス (例: / または \)
 *
 * @param[in] path 判定対象のパス文字列
 * @return int ルートディレクトリの場合は非ゼロ値、それ以外の場合は 0 を返す
 */
static int is_root_directory(const char *path) {
  // ケース 1: "X:\" または "X:/" 形式 (ドライブレター + ルート)
  if (strlen(path) == 3 && isalpha((unsigned char)path[0]) && path[1] == ':' &&
      (path[2] == '\\' || path[2] == '/')) {
    return 1;
  }

  // ケース 2: "/" または "\" 形式 (ルートディレクトリ)
  if (strcmp(path, "/") == 0 || strcmp(path, "\\") == 0) {
    return 1;
  }

  return 0;
}

/**
 * @brief 安全な snprintf 関数のラッパー
 *
 * バッファが不足する場合でも必ず NULL 終端する
 *
 * @param[out] dest 出力先バッファ
 * @param[in] dest_size 出力先バッファのサイズ
 * @param[in] format 書式指定文字列
 */
static void safe_snprintf(char *dest, size_t dest_size, const char *format,
                          ...) {
  va_list args;
  va_start(args, format);
  int result = vsnprintf(dest, dest_size, format, args);
  va_end(args);

  if (result >= (int)dest_size && dest_size > 0) {
    dest[dest_size - 1] = '\0';  // 切り詰めが発生した場合でも終端を保証
  }
}

/**
 * @brief 2 つのパスを連結する関数
 *
 * スラッシュの重複を防止し、Human68k のパス形式を考慮する
 * マルチバイト文字に対応しています
 *
 * @param[out] dest      結合結果を格納するバッファ
 * @param[in]  dest_size バッファのサイズ
 * @param[in]  dir       ディレクトリパス
 * @param[in]  file      ファイル名またはサブディレクトリ名
 */
static void join_paths(char *dest, size_t dest_size, const char *dir,
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
    // ドライブ指定のルートの場合 (例: "n:\")
    if (dir_len == 3 && isalpha((unsigned char)dir[0]) && dir[1] == ':') {
      safe_snprintf(dest, dest_size, "%c:%c%s", dir[0], dir[2], file);
    } else {
      // UNIX スタイルのルート
      safe_snprintf(dest, dest_size, "/%s", file);
    }
    return;
  }

  // 通常のディレクトリの場合
  int needs_separator = 1;  // デフォルトでセパレータが必要

  if (dir_len > 0) {
    // mbsdec()を使用して、マルチバイト文字列の末尾の文字を正しく取得
    unsigned char *dir_end = (unsigned char *)dir + dir_len;
    unsigned char *last_char_pos =
        (unsigned char *)mbsdec((const unsigned char *)dir, dir_end);

    // 最後の文字のバイト数を計算 (マルチバイト文字の場合は 2 バイト)
    size_t last_char_size = dir_end - last_char_pos;

    // 最後の文字がスラッシュまたはバックスラッシュならセパレータは不要
    if (last_char_size == 1 &&
        (*last_char_pos == '/' || *last_char_pos == '\\')) {
      needs_separator = 0;
    }
  }

  // 区切り記号の有無に基づいてパスを結合
  if (needs_separator) {
    safe_snprintf(dest, dest_size, "%s/%s", dir, file);
  } else {
    safe_snprintf(dest, dest_size, "%s%s", dir, file);
  }
}

int search_directory(const char *base_dir, int current_depth, Options *opts) {
  DIR *dir;
  struct dirent *entry;
  char path[MAX_PATH];
  DirEntry *entries = NULL;
  int entry_count = 0;
  int entry_capacity = 0;
  int return_status = 0;

  // ファイルシステムの大文字小文字の区別を検索開始時に1回だけチェック
  static int fs_case_checked = 0;
  static int fs_ignore_case = 0;

  if (!fs_case_checked) {
    fs_ignore_case = is_filesystem_ignore_case();
    fs_case_checked = 1;
  }

  // 開始ディレクトリ (current_depth == 0) のときの特別処理
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
      // ここでは深い検査せず、単に出力する (簡易実装)
      printf("%s\n", base_dir);
    }
  }

  if (opts->maxdepth >= 0 && current_depth > opts->maxdepth - 1) {
    return 0;
  }

  // ディレクトリを開いてエントリを収集
  if ((dir = opendir(base_dir)) == NULL) {
    fprintf(stderr, "Cannot open directory '%s': %s\n", base_dir,
            strerror(errno));
    // 最初の呼び出し (current_depth == 0) でエラーの場合のみエラーコードを返す
    return (current_depth == 0) ? 1 : 0;
  }

  // 収集するエントリ用の初期メモリを確保
  entry_capacity = 32;  // 初期容量
  entries = (DirEntry *)malloc(sizeof(DirEntry) * entry_capacity);
  if (entries == NULL) {
    fprintf(stderr, "Memory allocation error\n");
    closedir(dir);
    return 1;
  }

  // ディレクトリエントリを読み込む ("." と ".." を除く)
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
        free(entries);
        closedir(dir);
        return 1;
      }
      entries = new_entries;
    }

    // エントリ名をコピー (snprintf を安全な関数に置き換え)
    safe_snprintf(entries[entry_count].name, sizeof(entries[entry_count].name),
                  "%s", entry->d_name);

    // ディレクトリかどうかを d_type から直接判断
    entries[entry_count].is_dir = (entry->d_type == DT_DIR);

    entry_count++;
  }

  // ディレクトリハンドルはもう必要ないので閉じる
  closedir(dir);

  // 収集したエントリを処理
  for (int i = 0; i < entry_count; i++) {
    // パスを結合
    join_paths(path, sizeof(path), base_dir, entries[i].name);

    // 条件を評価して、マッチすれば出力
    if (evaluate_conditions(&entries[i], opts, fs_ignore_case)) {
      printf("%s\n", path);
    }

    // ディレクトリなら再帰的に処理
    if (entries[i].is_dir) {
      search_directory(path, current_depth + 1, opts);
    }
  }

  // メモリを解放
  free(entries);

  return return_status;
}
