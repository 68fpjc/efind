#ifndef EFIND_H
#define EFIND_H

#define MAX_CONDITIONS 100  // 条件の最大数

/**
 * @brief 論理演算子を表す列挙型
 *
 * @enum Operator
 */
typedef enum {
  OP_NONE,  // 論理演算子が指定されていない状態
  OP_AND,   // 論理積 (AND) を表す
  OP_OR     // 論理和 (OR) を表す
} Operator;

/**
 * @brief ファイルの種類を表す列挙型
 *
 * @enum FileType
 */
typedef enum {
  TYPE_NONE,    // ファイルタイプが指定されていない状態
  TYPE_FILE,    // 通常のファイル
  TYPE_DIR,     // ディレクトリ
  TYPE_SYMLINK  // シンボリックリンク
} FileType;

/**
 * @brief 検索条件を表す構造体
 *
 * @struct Condition
 */
typedef struct {
  char *pattern;    // 検索に使用するパターン文字列
  FileType type;    // ファイルの種類を指定するためのフィールド
  Operator op;      // 条件を組み合わせるための演算子
  int ignore_case;  // 大文字小文字を区別しない場合は 1、区別する場合は 0
} Condition;

/**
 * @brief 検索オプションを表す構造体
 *
 * @struct Options
 */
typedef struct {
  int maxdepth;                          // 最大の検索深さ
  int condition_count;                   // 条件の数
  Condition conditions[MAX_CONDITIONS];  // 検索条件
} Options;

// 関数プロトタイプ

/**
 * @brief 指定されたディレクトリを再帰的に検索する
 *
 * @param[in] base_dir 検索を開始するディレクトリのパス
 * @param[in] current_depth 現在の検索深さ
 * @param[in] opts 検索オプションを含む構造体へのポインタ
 * @return int 成功時は 0、エラー時は 1
 */
int search_directory(const char *base_dir, int current_depth,
                     const Options *opts);

#endif /* EFIND_H */
