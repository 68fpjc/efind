#ifndef ARCH_H
#define ARCH_H

/**
 * @brief ファイルシステムが大文字小文字を区別するかどうかを返す
 * @return 0: 大文字小文字を区別する、1: 大文字小文字を区別しない
 */
int is_filesystem_ignore_case(void);

#endif /* ARCH_H */
