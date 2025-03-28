#include <stdio.h>
#include <string.h>

// static 関数をテストするため、efind.c ファイルをインクルードする
#include "../efind.c"

/**
 * @brief join_paths 関数のテストケース実行
 *
 * @param base ベースとなるパス
 * @param path 結合するパス
 * @param expected 期待される結果
 * @return テストが成功した場合は 0、失敗した場合は 1
 */
static int test_case(const char *base, const char *path, const char *expected) {
  char result[1024];

  join_paths(result, sizeof(result), base, path);

  if (strcmp(result, expected) == 0) {
    printf("OK: base=\"%s\", path=\"%s\" => \"%s\"\n", base, path, result);
    return 0;
  } else {
    printf("NG: base=\"%s\", path=\"%s\" => \"%s\" (期待値: \"%s\")\n", base,
           path, result, expected);
    return 1;
  }
}

/**
 * @brief join_paths 関数のテスト実行メイン関数
 *
 * @return エラー数（0 なら全テスト成功）
 */
int main(void) {
  int errors = 0;

  printf("join_paths のテスト開始\n");
  printf("-------------------------\n");

  // 基本的なパス結合
  errors += test_case("/usr/local", "bin", "/usr/local/bin");
  errors += test_case("/usr/local/", "bin", "/usr/local/bin");

  // 相対パスのケース（パス区切りを含まない path）
  errors += test_case("/usr/local", "bin", "/usr/local/bin");
  errors += test_case("/usr", "local", "/usr/local");
  errors += test_case("/", "etc", "/etc");

  // 特殊な base パスのケース
  errors += test_case("/", "usr", "/usr");
  errors += test_case("C:", "Windows", "C:/Windows");
  errors += test_case("C:\\", "Windows", "C:\\Windows");
  errors += test_case("\\", "temp", "\\temp");

  // 絶対パスのケース
  errors += test_case("/usr/local", "bin", "/usr/local/bin");
  errors += test_case("C:\\Windows", "System32", "C:\\Windows/System32");

  // 現在のディレクトリを表すドットのケース
  errors += test_case(".", "file", "./file");
  errors += test_case("./dir", "file", "./dir/file");

  printf("-------------------------\n");
  printf("テスト終了: %s（エラー数: %d）\n", errors == 0 ? "成功" : "失敗",
         errors);

  return errors > 0 ? 1 : 0;
}
