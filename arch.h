#ifndef ARCH_H
#define ARCH_H

#include <dirent.h>

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
 * @brief 文字列の末尾がパス区切り文字終わっているかを判定する
 *
 * @param path 判定するパス文字列
 * @return int パス区切り文字で終わっている場合は非ゼロ値、それ以外は 0
 */
int is_path_end_with_separator(const char *path);

#endif /* ARCH_H */
