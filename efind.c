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

  unsigned int p_char, s_char;

  while ((s_char = mbsnextc(s))) {
    p_char = mbsnextc(p);

    if (p_char == '*') {
      // '*' の場合: バックアップポインタを更新して次のパターン文字へ
      p = mbsinc(p);
      p_backup = p;
      s_backup = s;

      // パターンの終わりなら一致
      if (!mbsnextc(p)) return 1;

      p_char = mbsnextc(p);
    } else {
      if (p_char == '?') {
        // '?' の場合: 任意の 1 文字にマッチ (マルチバイト文字も含む)
        s = mbsinc(s);
        p = mbsinc(p);
      } else {
        // 大文字小文字を区別しない場合はアルファベット文字を小文字に変換
        if (ignore_case && ismbbalpha(p_char) && ismbbalpha(s_char)) {
          p_char = tolower(p_char);
          s_char = tolower(s_char);
        }

        if (p_char == s_char) {
          // 文字が一致 (マルチバイト文字も含む)
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
      }
    }
  }

  // 残りのパターンが全て '*' なら成功
  while (mbsnextc(p) == '*') p = mbsinc(p);

  // パターンの終わりまで来たらマッチ
  return !mbsnextc(p);
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
static int evaluate_conditions(const DirEntry *entry, const Options *opts,
                               const int fs_ignore_case) {
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
 * @brief asprintf 関数のラッパー
 *
 * asprintf を呼び出し、エラー時にはメッセージを出力する
 *
 * @param[out] strp  確保された文字列を格納するポインタのアドレス
 * @param[in] fmt    書式指定文字列
 * @param[in] ...    可変引数
 * @return 成功時は文字数、失敗時は負の値
 */
static int alloc_formatted_string(char **strp, const char *fmt, ...) {
  va_list ap;
  int result;

  va_start(ap, fmt);
  result = vasprintf(strp, fmt, ap);
  va_end(ap);

  if (result == -1) {
    fprintf(stderr, "Memory allocation error\n");
    *strp = NULL;
  }

  return result;
}

/**
 * @brief ディレクトリからエントリを収集する
 *
 * 指定されたディレクトリからすべてのエントリを読み込み、配列に格納する
 *
 * @param[in] dir_path 検索対象のディレクトリパス
 * @param[out] entries_ptr
 * 収集されたエントリの配列へのポインタ（関数内で割り当て）
 * @return 成功時は収集されたエントリ数、失敗時は負の値
 */
static int collect_directory_entries(const char *dir_path,
                                     DirEntry **entries_ptr) {
  DIR *dir;
  struct dirent *entry;
  DirEntry *entries = NULL;
  int entry_count = 0;
  int entry_capacity = 0;

  // ディレクトリを開いてエントリを収集
  if ((dir = opendir(dir_path)) == NULL) {
    fprintf(stderr, "Cannot open directory '%s': %s\n", dir_path,
            strerror(errno));
    return -1;
  }

  // 収集するエントリ用の初期メモリを確保
  entry_capacity = 128;  // 初期容量
  entries = (DirEntry *)malloc(sizeof(DirEntry) * entry_capacity);
  if (entries == NULL) {
    fprintf(stderr, "Memory allocation error\n");
    closedir(dir);
    return -1;
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
        return -1;
      }
      entries = new_entries;
    }

    // エントリ名をコピー
    strncpy(entries[entry_count].name, entry->d_name,
            sizeof(entries[entry_count].name) - 1);
    entries[entry_count].name[sizeof(entries[entry_count].name) - 1] =
        '\0';  // NULL終端を保証

    entries[entry_count].is_dir = is_directory_entry(entry);

    entry_count++;
  }

  // ディレクトリハンドルはもう必要ないので閉じる
  closedir(dir);

  *entries_ptr = entries;
  return entry_count;
}

int search_directory(const char *base_dir, const int current_depth,
                     const Options *opts) {
  DirEntry *entries = NULL;
  int entry_count = 0;
  int return_status = 0;
  char *base_dir_tmp = NULL;

  // ファイルシステムの大文字小文字の区別を検索開始時に 1 回だけチェック
  static int fs_case_checked = 0;
  static int fs_ignore_case = 0;

  if (!fs_case_checked) {
    fs_ignore_case = is_filesystem_ignore_case();
    fs_case_checked = 1;
  }

  if (alloc_formatted_string(       //
          &base_dir_tmp, "%s%s%s",  //
          base_dir,                 //
          should_append_dot(base_dir) ? "." : "",
          is_path_end_with_separator(base_dir) ? "" : "/") < 0) {
    return 1;
  }

  if (opts->maxdepth >= 0 && current_depth > opts->maxdepth - 1) {
    free(base_dir_tmp);
    return 0;
  }

  // ディレクトリからエントリを収集
  entry_count = collect_directory_entries(base_dir_tmp, &entries);
  if (entry_count < 0) {
    free(base_dir_tmp);
    // 最初の呼び出し (current_depth == 0) でエラーの場合のみエラーコードを返す
    return (current_depth == 0) ? 1 : 0;
  }

  // 収集したエントリを処理
  for (int i = 0; i < entry_count; i++) {
    char *path = NULL;
    // パスを結合
    if (alloc_formatted_string(&path, "%s%s", base_dir_tmp, entries[i].name) <
        0) {
      continue;
    }

    // 条件を評価して、マッチすれば出力
    if (evaluate_conditions(&entries[i], opts, fs_ignore_case)) {
      printf("%s\n", path);
    }

    // ディレクトリなら再帰的に処理
    if (entries[i].is_dir) {
      search_directory(path, current_depth + 1, opts);
    }

    free(path);
  }

  free(entries);
  free(base_dir_tmp);

  return return_status;
}
