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
  int attributes;  // 属性フラグ (FILE_ATTR_* の組み合わせ)
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
                         const int ignore_case, const int fs_ignore_case) {
  // ファイルシステムが大文字小文字を区別しない場合は、常に大文字小文字を区別しない処理を行う
  int effective_ignore_case = ignore_case;
  if (fs_ignore_case) {
    effective_ignore_case = 1;
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
        if (effective_ignore_case && ismbbalpha(p_char) && ismbbalpha(s_char)) {
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
      if (cond->type == TYPE_FILE &&
          (entry->is_dir || (entry->attributes & FILE_ATTR_SYMLINK))) {
        match = 0;
      } else if (cond->type == TYPE_DIR &&
                 (!entry->is_dir || (entry->attributes & FILE_ATTR_SYMLINK))) {
        match = 0;
      } else if (cond->type == TYPE_SYMLINK &&
                 !(entry->attributes & FILE_ATTR_SYMLINK)) {
        match = 0;
      } else if (cond->type == TYPE_EXECUTABLE &&
                 !(entry->attributes & FILE_ATTR_EXECUTABLE)) {
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
 * @brief ファイル属性チェックが必要かどうかを判定する
 *
 * Options 構造体を調べ、シンボリックリンクや実行可能ファイルなど
 * ファイル属性の検索条件が含まれているかを確認する
 *
 * @param[in] opts 検索オプション構造体へのポインタ
 * @return ファイル属性の検索が必要な場合は 1、それ以外は 0
 */
static int needs_file_attribute_check(const Options *opts) {
  if (opts == NULL || opts->condition_count == 0) {
    return 0;
  }

  // 各条件を調べ、TYPE_SYMLINK または TYPE_EXECUTABLE があるか確認
  for (int i = 0; i < opts->condition_count; i++) {
    if (opts->conditions[i].type == TYPE_SYMLINK ||
        opts->conditions[i].type == TYPE_EXECUTABLE) {
      return 1;
    }
  }

  return 0;
}

/**
 * @brief ディレクトリからエントリを収集する
 *
 * 指定されたディレクトリからすべてのエントリを読み込み、配列に格納する
 *
 * @param[in] dir_path 検索対象のディレクトリパス
 * @param[out] entries_ptr
 * 収集されたエントリの配列へのポインタ (関数内で割り当て)
 * @param[in] opts 検索オプション構造体へのポインタ
 * @return 成功時は収集されたエントリ数、失敗時は負の値
 */
static int collect_directory_entries(const char *dir_path,
                                     DirEntry **entries_ptr,
                                     const Options *opts) {
  DIR *dir;
  struct dirent *entry;
  DirEntry *entries = NULL;
  int entry_count = 0;
  int entry_capacity = 0;
  char *full_path = NULL;  // 動的に確保するように変更
  int check_attributes =
      needs_file_attribute_check(opts);  // ファイル属性のチェックが必要かどうか

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

    // 完全パスを構築 (alloc_formatted_string を使用)
    if (alloc_formatted_string(&full_path, "%s/%s", dir_path, entry->d_name) <
        0) {
      // メモリ確保に失敗した場合
      fprintf(stderr, "Memory allocation error for path\n");
      free(entries);
      closedir(dir);
      return -1;
    }

    // ディレクトリかどうかの判定
    entries[entry_count].is_dir = is_directory_entry(entry);

    // 属性情報の取得と設定 (属性チェックが必要な場合のみ)
    if (check_attributes) {
      entries[entry_count].attributes = get_file_attributes(full_path);
    } else {
      entries[entry_count].attributes = 0;
    }

    // 不要になったパスを解放
    free(full_path);
    full_path = NULL;

    entry_count++;
  }

  // ディレクトリハンドルはもう必要ないので閉じる
  closedir(dir);

  *entries_ptr = entries;
  return entry_count;
}

/**
 * @brief 通常ファイルを処理する
 *
 * 通常ファイルに対して条件評価を行い、条件に合致する場合は表示する
 *
 * @param[in] file_path 処理対象ファイルのパス
 * @param[in] opts 検索オプション構造体へのポインタ
 * @param[in] fs_ignore_case ファイルシステムが大文字小文字を区別しない場合は
 * 1、区別する場合は 0
 * @return 常に 0 を返す (正常終了)
 */
static int process_regular_file(const char *file_path, const Options *opts,
                                const int fs_ignore_case) {
  // 通常ファイル用の DirEntry を作成
  DirEntry file_entry;
  char *file_name = strrchr(file_path, '/');

  if (file_name == NULL) {
    file_name = strrchr(file_path, '\\');
  }

  // ファイル名部分を取得 (パス区切り文字がない場合はパス全体がファイル名)
  if (file_name == NULL) {
    file_name = (char *)file_path;
  } else {
    file_name++;  // パス区切り文字をスキップ
  }

  // ファイルエントリ情報を設定
  strncpy(file_entry.name, file_name, sizeof(file_entry.name) - 1);
  file_entry.name[sizeof(file_entry.name) - 1] = '\0';  // NULL 終端を保証
  file_entry.is_dir = 0;                                // 通常ファイル

  // ファイル属性のチェックが必要な場合のみ属性を取得
  if (needs_file_attribute_check(opts)) {
    file_entry.attributes = get_file_attributes(file_path);
  } else {
    file_entry.attributes = 0;
  }

  // 条件に合致するか評価して表示
  if (evaluate_conditions(&file_entry, opts, fs_ignore_case)) {
    printf("%s\n", file_path);
  }

  return 0;
}

/**
 * @brief ディレクトリを処理する
 *
 * ディレクトリ内のエントリを収集し、条件に合致するものを表示する
 * また、サブディレクトリがある場合は再帰的に処理する
 *
 * @param[in] dir_path 処理対象ディレクトリのパス
 * @param[in] current_depth 現在の再帰深度
 * @param[in] opts 検索オプション構造体へのポインタ
 * @param[in] fs_ignore_case ファイルシステムが大文字小文字を区別しない場合は
 * 1、区別する場合は 0
 * @return 成功時は 0、エラー時は 1
 */
static int process_directory(const char *dir_path, const int current_depth,
                             const Options *opts, const int fs_ignore_case) {
  DirEntry *entries = NULL;
  int entry_count = 0;
  int return_status = 0;
  char *dir_path_tmp = NULL;

  if (alloc_formatted_string(       //
          &dir_path_tmp, "%s%s%s",  //
          dir_path,                 //
          should_append_dot(dir_path) ? "." : "",
          is_path_end_with_separator(dir_path) ? "" : "/") < 0) {
    return 1;
  }

  if (opts->maxdepth >= 0 && current_depth > opts->maxdepth - 1) {
    free(dir_path_tmp);
    return 0;
  }

  // ディレクトリからエントリを収集
  entry_count = collect_directory_entries(dir_path_tmp, &entries, opts);
  if (entry_count < 0) {
    free(dir_path_tmp);
    // 最初の呼び出し (current_depth == 0) でエラーの場合のみエラーコードを返す
    return (current_depth == 0) ? 1 : 0;
  }

  // 収集したエントリを処理
  for (int i = 0; i < entry_count; i++) {
    char *path = NULL;
    // パスを結合
    if (alloc_formatted_string(&path, "%s%s", dir_path_tmp, entries[i].name) <
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
  free(dir_path_tmp);

  return return_status;
}

int search_directory(const char *base_dir, const int current_depth,
                     const Options *opts) {
  // ファイルシステムの大文字小文字の区別を検索開始時に 1 回だけチェック
  static int fs_case_checked = 0;
  static int fs_ignore_case = 0;

  if (!fs_case_checked) {
    fs_ignore_case = is_filesystem_ignore_case();
    fs_case_checked = 1;
  }

  // パスが存在する通常ファイルの場合
  if (is_existing_regular_file(base_dir)) {
    return process_regular_file(base_dir, opts, fs_ignore_case);
  } else {
    // ディレクトリの場合
    return process_directory(base_dir, current_depth, opts, fs_ignore_case);
  }
}
