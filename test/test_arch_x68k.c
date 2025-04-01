/**
 * @file test_arch_x68k.c
 * @brief arch_x68k.c の関数をテストするテストコード
 */
#include <stdio.h>
#include <string.h>

#include "../arch.h"

/**
 * @brief is_path_end_with_separator() 関数のテスト
 *
 * パスの末尾が区切り文字 ('/' または '\\') で終わるかどうかを確認する
 * @return テスト結果 (0: 成功, 1: 失敗)
 */
int test_is_path_end_with_separator(void) {
  int failed = 0;

  // スラッシュで終わるパス
  if (!is_path_end_with_separator("C:/foo/bar/")) {
    printf("テスト 1: パスがスラッシュで終わる - 失敗\n");
    failed++;
  } else {
    printf("テスト 1: パスがスラッシュで終わる - OK\n");
  }

  // バックスラッシュで終わるパス
  if (!is_path_end_with_separator("C:\\foo\\bar\\")) {
    printf("テスト 2: パスがバックスラッシュで終わる - 失敗\n");
    failed++;
  } else {
    printf("テスト 2: パスがバックスラッシュで終わる - OK\n");
  }

  // 区切り文字で終わらないパス
  if (is_path_end_with_separator("C:/foo/bar")) {
    printf("テスト 3: パスが区切り文字で終わらない - 失敗\n");
    failed++;
  } else {
    printf("テスト 3: パスが区切り文字で終わらない - OK\n");
  }

  // 空のパス
  if (is_path_end_with_separator("")) {
    printf("テスト 4: 空のパス - 失敗\n");
    failed++;
  } else {
    printf("テスト 4: 空のパス - OK\n");
  }

  // マルチバイト文字を含むパス (日本語 + 区切り文字なし)
  if (is_path_end_with_separator("C:/フォルダ/テスト")) {
    printf("テスト 5: 日本語を含むパスが区切り文字で終わらない - 失敗\n");
    failed++;
  } else {
    printf("テスト 5: 日本語を含むパスが区切り文字で終わらない - OK\n");
  }

  // マルチバイト文字を含むパス (日本語 + 区切り文字あり)
  if (!is_path_end_with_separator("C:/フォルダ/テスト/")) {
    printf("テスト 6: 日本語を含むパスが区切り文字で終わる - 失敗\n");
    failed++;
  } else {
    printf("テスト 6: 日本語を含むパスが区切り文字で終わる - OK\n");
  }

  // ドライブ名のみの場合
  if (is_path_end_with_separator("C:")) {
    printf("テスト 7: ドライブ名のみの場合 - 失敗\n");
    failed++;
  } else {
    printf("テスト 7: ドライブ名のみの場合 - OK\n");
  }

  // ドライブ名 + 区切り文字の場合
  if (!is_path_end_with_separator("C:/")) {
    printf("テスト 8: ドライブ名に区切り文字が続く場合 - 失敗\n");
    failed++;
  } else {
    printf("テスト 8: ドライブ名に区切り文字が続く場合 - OK\n");
  }

  return failed;
}

/**
 * @brief メイン関数
 * @return テスト結果 (0: 成功, 0以外: 失敗)
 */
int main(void) {
  printf(
      "twon.r の出力と比較してください :\n\n"
      "%s\n\n\n",
      is_filesystem_ignore_case()
          ? "(V)TwentyOne.sys が常駐していないか、 TwentyOne -C です"
          : "TwentyOne +C です");

  int failed = 0;

  printf("is_path_end_with_separator のテスト開始\n");
  failed += test_is_path_end_with_separator();

  if (failed) {
    printf("テスト失敗: %d 件\n", failed);
  } else {
    printf("全てのテスト成功！\n");
  }

  return failed;
}
