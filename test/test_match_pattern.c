#include <stdio.h>
#include <string.h>

// static 関数をテストするため、efind.c ファイルをインクルードする
#include "../efind.c"

/**
 * @brief テスト結果を表示する関数
 *
 * テストケースの実行結果を表示する
 *
 * @param[in] test_name テスト名
 * @param[in] pattern マッチングパターン
 * @param[in] string 比較文字列
 * @param[in] ignore_case 大文字小文字を区別しない場合は 1、区別する場合は 0
 * @param[in] fs_ignore_case ファイルシステムが大文字小文字を区別しない場合は
 * 1、区別する場合は 0
 * @param[in] expected 期待される結果
 * @param[in] result 実際の結果
 */
void print_test_result(const char *test_name, const char *pattern,
                       const char *string, int ignore_case, int fs_ignore_case,
                       int expected, int result) {
  printf("%s: ", test_name);
  if (expected == result) {
    printf("成功\n");
  } else {
    printf(
        "失敗 (パターン: \"%s\", 文字列: \"%s\", ignore_case: %d, "
        "fs_ignore_case: %d, 期待値: %d, 結果: %d)\n",
        pattern, string, ignore_case, fs_ignore_case, expected, result);
  }
}

/**
 * @brief 単一のテストケースを実行する関数
 *
 * 指定されたパターンと文字列でマッチングテストを実行する
 *
 * @param[in] test_name テスト名
 * @param[in] pattern マッチングパターン
 * @param[in] string 比較文字列
 * @param[in] ignore_case 大文字小文字を区別しない場合は 1、区別する場合は 0
 * @param[in] fs_ignore_case ファイルシステムが大文字小文字を区別しない場合は
 * 1、区別する場合は 0
 * @param[in] expected 期待される結果
 * @return int テスト成功時は 1、失敗時は 0
 */
int run_test(const char *test_name, const char *pattern, const char *string,
             int ignore_case, int fs_ignore_case, int expected) {
  int result = match_pattern(pattern, string, ignore_case, fs_ignore_case);
  print_test_result(test_name, pattern, string, ignore_case, fs_ignore_case,
                    expected, result);
  return (expected == result);
}

/**
 * @brief メイン関数
 *
 * 様々なパターンでテストケースを実行する
 *
 * @return int 全テスト成功時は 0、失敗時は 1
 */
int main(void) {
  int failed_tests = 0;

  printf("match_pattern のテストを開始します\n");
  printf("----------------------------------------------------\n");

  // ファイルシステムが大文字小文字を区別する場合のテスト
  printf(
      "\n【ファイルシステムが大文字小文字を区別する場合 (fs_ignore_case = "
      "0)】\n\n");

  // 基本的なマッチングテスト (ignore_case = 0, fs_ignore_case = 0)
  if (!run_test("基本的な完全一致", "hello", "hello", 0, 0, 1)) failed_tests++;
  if (!run_test("大文字小文字の区別あり", "Hello", "hello", 0, 0, 0))
    failed_tests++;
  if (!run_test("不一致のテスト", "hello", "world", 0, 0, 0)) failed_tests++;

  // ワイルドカード '*' のテスト
  if (!run_test("前方一致", "hel*", "hello", 0, 0, 1)) failed_tests++;
  if (!run_test("後方一致", "*llo", "hello", 0, 0, 1)) failed_tests++;
  if (!run_test("中間一致", "h*o", "hello", 0, 0, 1)) failed_tests++;
  if (!run_test("複数の *", "h*l*o", "hello", 0, 0, 1)) failed_tests++;
  if (!run_test("* のみ", "*", "anything", 0, 0, 1)) failed_tests++;
  if (!run_test("空文字列に対する *", "*", "", 0, 0, 1)) failed_tests++;

  // ワイルドカード '?' のテスト
  if (!run_test("? 一文字置換", "h?llo", "hello", 0, 0, 1)) failed_tests++;
  if (!run_test("複数の ?", "h??lo", "hello", 0, 0, 1)) failed_tests++;
  if (!run_test("不一致の ?", "h?", "hello", 0, 0, 0)) failed_tests++;

  // マルチバイト文字のテスト
  if (!run_test("日本語の完全一致", "テスト", "テスト", 0, 0, 1))
    failed_tests++;
  if (!run_test("日本語の前方一致", "テス*", "テスト", 0, 0, 1)) failed_tests++;
  if (!run_test("日本語の後方一致", "*スト", "テスト", 0, 0, 1)) failed_tests++;
  if (!run_test("日本語の ? テスト", "テ?ト", "テスト", 0, 0, 1))
    failed_tests++;
  if (!run_test("日本語と ASCII の混合", "テスト*", "テストfile", 0, 0, 1))
    failed_tests++;
  if (!run_test("日本語の不一致", "テスト", "試験", 0, 0, 0)) failed_tests++;

  // 半角英字と全角英字の比較テスト
  if (!run_test("半角と全角 (完全一致なし)", "hello", "ｈｅｌｌｏ", 0, 0, 0))
    failed_tests++;
  if (!run_test("半角と全角 (ワイルドカード)", "h*o", "ｈｅｌｌｏ", 0, 0, 0))
    failed_tests++;
  if (!run_test("全角と半角 (完全一致なし)", "ｈｅｌｌｏ", "hello", 0, 0, 0))
    failed_tests++;
  if (!run_test("全角のパターンと半角の一部一致", "ｈ*ｏ", "hello", 0, 0, 0))
    failed_tests++;
  if (!run_test("混合文字列 (半角パターン)", "he*", "heｌｌｏ", 0, 0, 1))
    failed_tests++;
  if (!run_test("混合文字列 (全角パターン)", "ｈｅ*", "ｈｅllo", 0, 0, 1))
    failed_tests++;

  // 大文字小文字を区別しないフラグ (ignore_case = 1, fs_ignore_case = 0)
  if (!run_test("-iname オプション相当", "Hello", "hello", 1, 0, 1))
    failed_tests++;
  if (!run_test("-iname 複合パターン", "He*O", "hello", 1, 0, 1))
    failed_tests++;

  // ファイルシステムが大文字小文字を区別しない場合のテスト
  printf(
      "\n【ファイルシステムが大文字小文字を区別しない場合 (fs_ignore_case = "
      "1)】\n\n");

  // 基本的なマッチングテスト (ignore_case = 0, fs_ignore_case = 1)
  if (!run_test("基本的な完全一致", "hello", "hello", 0, 1, 1)) failed_tests++;
  if (!run_test("大文字小文字の区別なし (-name)", "Hello", "hello", 0, 1, 1))
    failed_tests++;
  if (!run_test("不一致のテスト", "hello", "world", 0, 1, 0)) failed_tests++;

  // ワイルドカード '*' のテスト
  if (!run_test("前方一致", "hEl*", "hello", 0, 1, 1)) failed_tests++;
  if (!run_test("後方一致", "*lLo", "hello", 0, 1, 1)) failed_tests++;
  if (!run_test("中間一致", "h*O", "hello", 0, 1, 1)) failed_tests++;

  // ワイルドカード '?' のテスト
  if (!run_test("? 一文字置換", "h?LLo", "hello", 0, 1, 1)) failed_tests++;
  if (!run_test("複数の ?", "H??lO", "hello", 0, 1, 1)) failed_tests++;

  // 半角英字と全角英字の比較テスト
  if (!run_test("半角と全角 (区別あり)", "hello", "ｈｅｌｌｏ", 0, 1, 0))
    failed_tests++;
  if (!run_test("半角と全角 (ignore_case)", "hello", "ｈｅｌｌｏ", 1, 1, 0))
    failed_tests++;
  if (!run_test("全角と半角 (区別あり)", "ｈｅｌｌｏ", "hello", 0, 1, 0))
    failed_tests++;
  if (!run_test("全角大文字と半角小文字", "ＨＥＬＬＯ", "hello", 0, 1, 0))
    failed_tests++;
  if (!run_test("混合文字列 (半角パターン)", "he*", "heｌｌｏ", 0, 1, 1))
    failed_tests++;
  if (!run_test("混合文字列 (全角パターン)", "ｈｅ*", "ｈｅllo", 0, 1, 1))
    failed_tests++;

  // 大文字小文字を区別しないフラグ (ignore_case = 1, fs_ignore_case = 1)
  // fs_ignore_case = 1 の場合は ignore_case
  // の値に関わらず常に大文字小文字を区別しない
  if (!run_test("-name と -iname は同じ結果", "Hello", "hello", 1, 1, 1))
    failed_tests++;
  if (!run_test("-name 複合パターン", "He*O", "hello", 0, 1, 1)) failed_tests++;
  if (!run_test("-iname 複合パターン", "He*O", "hello", 1, 1, 1))
    failed_tests++;

  // 混合環境での特殊ケース
  if (!run_test("大文字のみのパターン", "HELLO", "hello", 0, 1, 1))
    failed_tests++;
  if (!run_test("大文字のみの文字列", "hello", "HELLO", 0, 1, 1))
    failed_tests++;
  if (!run_test("混合大文字小文字", "HeLLo", "hEllO", 0, 1, 1)) failed_tests++;

  // 結果表示
  printf("----------------------------------------------------\n");
  if (failed_tests == 0) {
    printf("全てのテストが成功しました！\n");
    return 0;
  } else {
    printf("%d 個のテストが失敗しました。\n", failed_tests);
    return 1;
  }
}
