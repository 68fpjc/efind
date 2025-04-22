#ifndef ARCH_H
#define ARCH_H

#include <dirent.h>

/**
 * @brief ファイル属性のビットフラグ定義
 */
#define FILE_ATTR_SYMLINK (1 << 0)     // シンボリックリンク属性
#define FILE_ATTR_EXECUTABLE (1 << 1)  // 実行可能属性

/**
 * @brief ファイルシステムが大文字小文字を区別するかどうかを判定する
 *
 * @return 0: 大文字小文字を区別する、1: 大文字小文字を区別しない
 */
int is_filesystem_ignore_case(void);

/**
 * @brief struct dirent のエントリがディレクトリを表しているかどうかを判定する
 *
 * @param[in] entry 判定するディレクトリエントリ
 * @return ディレクトリの場合は非ゼロ値、それ以外は 0
 */
int is_directory_entry(struct dirent *entry);

/**
 * @brief ファイルの属性を取得する
 *
 * ファイルの複数の属性を取得し、ビットフラグで返す
 *
 * @param[in] pathname 属性を取得するファイルの名前
 * @return 属性のビットフラグ (FILE_ATTR_* 定数の組み合わせ)
 */
int get_file_attributes(const char *path);

/**
 * @brief 文字列の末尾がパス区切り文字終わっているかを判定する
 *
 * @param path 判定するパス文字列
 * @return int パス区切り文字で終わっている場合は非ゼロ値、それ以外は 0
 */
int is_path_end_with_separator(const char *path);

/**
 * @brief パス文字列の末尾に "." を付加すべきかどうかを判定する
 *
 * @param[in] path 判定するパス文字列
 * @return int "." を付加すべき場合は非ゼロ値、それ以外は 0
 */
int should_append_dot(const char *path);

#endif /* ARCH_H */
