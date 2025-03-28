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
 * @param[in] expected 期待される結果
 * @param[in] result 実際の結果
 */
void print_test_result(const char *test_name, const char *pattern,
                       const char *string, int expected, int result) {
  printf("%s: ", test_name);
  if (expected == result) {
    printf("成功\n");
  } else {
    printf("失敗 (パターン: \"%s\", 文字列: \"%s\", 期待値: %d, 結果: %d)\n",
           pattern, string, expected, result);
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
 * @param[in] expected 期待される結果
 * @return int テスト成功時は 1、失敗時は 0
 */
int run_test(const char *test_name, const char *pattern, const char *string,
             int expected) {
  int result = match_pattern_case_insensitive(pattern, string);
  print_test_result(test_name, pattern, string, expected, result);
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

  printf("match_pattern_case_insensitive のテストを開始します\n");
  printf("----------------------------------------------------\n");

  // 基本的なマッチングテスト
  if (!run_test("基本的な完全一致", "hello", "hello", 1)) failed_tests++;
  if (!run_test("大文字小文字の区別なし", "Hello", "hElLo", 1)) failed_tests++;
  if (!run_test("不一致のテスト", "hello", "world", 0)) failed_tests++;

  // ワイルドカード '*' のテスト
  if (!run_test("前方一致", "hel*", "hello", 1)) failed_tests++;
  if (!run_test("後方一致", "*llo", "hello", 1)) failed_tests++;
  if (!run_test("中間一致", "h*o", "hello", 1)) failed_tests++;
  if (!run_test("複数の *", "h*l*o", "hello", 1)) failed_tests++;
  if (!run_test("* のみ", "*", "anything", 1)) failed_tests++;
  if (!run_test("空文字列に対する *", "*", "", 1)) failed_tests++;

  // ワイルドカード '?' のテスト
  if (!run_test("? 一文字置換", "h?llo", "hello", 1)) failed_tests++;
  if (!run_test("複数の ?", "h??lo", "hello", 1)) failed_tests++;
  if (!run_test("不一致の ?", "h?", "hello", 0)) failed_tests++;

  // 複合的なワイルドカードのテスト
  if (!run_test("* と ? の組み合わせ", "h?l*o", "hello", 1)) failed_tests++;
  if (!run_test("複雑なパターン", "*?l*", "hello", 1)) failed_tests++;

  // マルチバイト文字のテスト
  if (!run_test("日本語の完全一致", "テスト", "テスト", 1)) failed_tests++;
  if (!run_test("日本語の前方一致", "テス*", "テスト", 1)) failed_tests++;
  if (!run_test("日本語の後方一致", "*スト", "テスト", 1)) failed_tests++;
  if (!run_test("日本語の ? テスト", "テ?ト", "テスト", 1)) failed_tests++;
  if (!run_test("日本語と ASCII の混合", "テスト*", "テストfile", 1))
    failed_tests++;
  if (!run_test("日本語の不一致", "テスト", "試験", 0)) failed_tests++;

  // 複雑なケース
  if (!run_test("ファイル名の拡張子マッチ", "*.txt", "document.txt", 1))
    failed_tests++;
  if (!run_test("複合拡張子", "*.t?t", "document.txt", 1)) failed_tests++;
  if (!run_test("複合パターン", "??c*.*", "document.txt", 1)) failed_tests++;
  if (!run_test("日本語ファイル名", "テスト*.*", "テストファイル.txt", 1))
    failed_tests++;

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
