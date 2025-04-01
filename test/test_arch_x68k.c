#include <stdio.h>
#include <string.h>

#include "../arch.h"

/**
 * @brief is_filesystem_ignore_case 関数のテスト実行メイン関数
 *
 * @return エラー数（0 なら全テスト成功）
 */
int main(void) {
  int errors = 0;

  printf(
      "twon.r の出力と比較してください :\n\n"
      "%s\n",
      is_filesystem_ignore_case()
          ? "(V)TwentyOne.sys が常駐していないか、 TwentyOne -C です"
          : "TwentyOne +C です");

  return errors > 0 ? 1 : 0;
}
