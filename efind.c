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
 * @brief パターンマッチングを行う
 *
 * 指定されたパターンと文字列を比較し、大文字小文字の区別設定に従って一致するかどうかを判定する
 *
 * @param[in] pattern 比較対象のパターン (ヌル終端文字列)
 * @param[in] string チェック対象の文字列 (ヌル終端文字列)
 * @param[in] ignore_case 大文字小文字を区別しない場合は 1、区別する場合は 0
 * @param[in] fs_ignore_case ファイルシステムが大文字小文字を区別しない場合は
 * 1、区別する場合は 0
 * @return 一致する場合は非ゼロ値、一致しない場合は 0
 */
static int match_pattern(const char *pattern, const char *string,
                         int ignore_case, const int fs_ignore_case) {
  // ファイルシステムが大文字小文字を区別しない場合は、常に大文字小文字を区別しない処理を行う
  if (fs_ignore_case) {
    ignore_case = 1;
  }

  unsigned char *p = (unsigned char *)pattern;
  unsigned char *s = (unsigned char *)string;
  unsigned char *p_backup = NULL;
  unsigned char *s_backup = NULL;

  while (*s) {
    if (*p == '*') {
      // '*' の場合: バックアップポインタを更新して次のパターン文字へ
      p_backup = mbsinc(p);
      s_backup = s;
      p = p_backup;
      if (*p == '\0') return 1;  // パターンの終わりなら一致
    } else if (*p == '?') {
      // '?' の場合: 任意の 1 文字にマッチ (マルチバイト文字も含む)
      s = mbsinc(s);
      p = mbsinc(p);
    } else if (ismbblead(*p) && ismbblead(*s)) {
      // 両方がマルチバイト文字の先頭の場合
      if ((*p == *s) && (*(p + 1) == *(s + 1))) {
        // マルチバイト文字が一致
        s = mbsinc(s);
        p = mbsinc(p);
      } else if (p_backup) {
        // バックトラック
        p = p_backup;
        s = mbsinc(s_backup);
        s_backup = s;
      } else {
        return 0;  // 不一致
      }
    } else if (ismbblead(*s)) {
      // 文字列側だけがマルチバイト文字の場合
      if (p_backup) {
        // バックトラック
        p = p_backup;
        s = mbsinc(s_backup);
        s_backup = s;
      } else {
        return 0;  // 不一致
      }
    } else if (ismbblead(*p)) {
      // パターン側だけがマルチバイト文字の場合
      if (p_backup) {
        // バックトラック
        p = p_backup;
        s = mbsinc(s_backup);
        s_backup = s;
      } else {
        return 0;  // 不一致
      }
    } else if (ignore_case ? tolower(*p) == tolower(*s) : *p == *s) {
      // シングルバイト文字が一致
      s++;
      p++;
    } else if (p_backup) {
      // バックトラック
      p = p_backup;
      s = ++s_backup;
    } else {
      return 0;  // 不一致
    }
  }

  // 残りのパターンが全て '*' なら成功
  while (*p == '*') p = mbsinc(p);

  // パターンの終わりまで来たらマッチ
  return *p == '\0';
}

/**
 * @brief 指定された条件を評価する
 *
 * ディレクトリエントリに基づいて、指定された条件を評価し、条件を満たすかどうかを判定する
 *
 * @param[in] entry 評価対象のディレクトリエントリ
 * @param[in] opts 評価基準を含む Options 構造体へのポインタ
 * @param[in] fs_ignore_case ファイルシステムが大文字小文字を区別しない場合は
 * 1、区別する場合は 0
 * @return 条件を満たす場合は 1 を返し、満たさない場合は 0
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
 * @brief snprintf 関数のラッパー
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
 * @brief 2 つのパスを連結する
 *
 * @param[out] dest      結合結果を格納するバッファ
 * @param[in]  dest_size バッファのサイズ
 * @param[in]  dir       ディレクトリパス
 * @param[in]  file      ファイル名またはサブディレクトリ名
 */
static void join_paths(char *dest, size_t dest_size, const char *dir,
                       const char *file) {
  safe_snprintf(dest, dest_size, "%s%s%s",  //
                dir, is_path_end_with_separator(dir) ? "" : "/", file);
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

    entries[entry_count].is_dir = is_directory_entry(entry);

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
