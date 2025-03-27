#include <stdio.h>

#include "../mbstring.h"

void run_test(const char* test_name, int actual, int expected) {
  printf("テスト '%s': ", test_name);
  if (actual == expected) {
    printf("成功\n");
  } else {
    printf("失敗 (期待値: %d, 実際値: %d)\n", expected, actual);
  }
}

int main() {
  int test_count = 0;
  int pass_count = 0;

  // テスト 1: current と start が等しい場合は NULL を返す
  {
    unsigned char str[] = "テスト";
    unsigned char* result = mbsdec(str, str);

    printf("テスト 1: current と start が等しい場合\n");
    if (result == NULL) {
      printf("  成功: NULL が返りました\n");
      pass_count++;
    } else {
      printf("  失敗: NULL が期待されましたが、%p が返りました\n", result);
    }
    test_count++;
  }

  // テスト 2: current が start より前の場合は NULL を返す
  {
    unsigned char str[] = "テスト";
    unsigned char* result = mbsdec(str + 1, str);

    printf("テスト 2: current が start より前の場合\n");
    if (result == NULL) {
      printf("  成功: NULL が返りました\n");
      pass_count++;
    } else {
      printf("  失敗: NULL が期待されましたが、%p が返りました\n", result);
    }
    test_count++;
  }

  // テスト 3: ASCII 文字列での 1 バイト文字の処理
  {
    unsigned char str[] = "abcde";
    unsigned char* curr = str + 3;      // 'd' を指す
    unsigned char* expected = str + 2;  // 'c' を指す
    unsigned char* result = mbsdec(str, curr);
    printf("テスト 3: ASCII 文字列での 1 バイト文字の処理\n");
    if (result == expected) {
      printf("  成功: 前の文字に正しく移動しました\n");
      pass_count++;
    } else {
      printf("  失敗: 期待値 %p, 実際値 %p\n", expected, result);
    }
    test_count++;
  }

  // テスト 4: ASCII 制御文字を含む処理
  {
    unsigned char str[] = "a\t\n\r\b";
    unsigned char* curr = str + 4;      // '\b' を指す
    unsigned char* expected = str + 3;  // '\r' を指す
    unsigned char* result = mbsdec(str, curr);
    printf("テスト 4: ASCII 制御文字を含む処理\n");
    if (result == expected) {
      printf("  成功: 前の文字に正しく移動しました\n");
      pass_count++;
    } else {
      printf("  失敗: 期待値 %p, 実際値 %p\n", expected, result);
    }
    test_count++;
  }

  // テスト 5: 拡張 ASCII 文字 (0x80-0xFF) の処理
  {
    unsigned char str[] = {0x7E, 0x7F, 0x80, 0x81,
                           0x00};       // ~, DEL, 拡張 ASCII 文字
    unsigned char* curr = str + 3;      // 0x81 を指す
    unsigned char* expected = str + 2;  // 0x80 を指す
    unsigned char* result = mbsdec(str, curr);
    printf("テスト 5: 拡張 ASCII 文字の処理\n");
    if (result == expected) {
      printf("  成功: 前の文字に正しく移動しました\n");
      pass_count++;
    } else {
      printf("  失敗: 期待値 %p, 実際値 %p\n", expected, result);
    }
    test_count++;
  }

  // テスト 6: 基本的な日本語文字列 (2 バイト文字) の処理
  {
    unsigned char str[] = "あいうえお";
    unsigned char* curr = str + 2;  // 'い' の先頭を指す
    unsigned char* expected = str;  // 'あ' の先頭を指す
    unsigned char* result = mbsdec(str, curr);

    printf("テスト 6: 基本的な日本語文字列 (2 バイト文字) の処理\n");
    if (result == expected) {
      printf("  成功: 前の文字 (あ) に正しく移動しました\n");
      pass_count++;
    } else {
      printf("  失敗: 期待値 %p, 実際値 %p\n", expected, result);
    }
    test_count++;
  }

  // テスト 7: 日本語の異なる文字コード範囲 (平仮名・片仮名・漢字)
  {
    unsigned char str[] = "アあ漢";
    unsigned char* curr = str + 4;      // '漢' の先頭を指す
    unsigned char* expected = str + 2;  // 'あ' の先頭を指す
    unsigned char* result = mbsdec(str, curr);

    printf("テスト 7: 日本語の異なる文字コード範囲\n");
    if (result == expected) {
      printf("  成功: 前の文字 (あ) に正しく移動しました\n");
      pass_count++;
    } else {
      printf("  失敗: 期待値 %p, 実際値 %p\n", expected, result);
    }
    test_count++;
  }

  // テスト 8: 2 バイト文字の 2 バイト目から呼び出した場合
  {
    unsigned char str[] = "あいうえお";
    unsigned char* curr = str + 1;  // 'あ' の 2 バイト目を指す
    unsigned char* expected = str;  // 'あ' の先頭を指す - Windows では単に
                                    // curr - 1 を返す可能性がある
    unsigned char* result = mbsdec(str, curr);

    printf("テスト 8: 2 バイト文字の 2 バイト目から呼び出した場合\n");
    if (result == expected) {
      printf("  成功: 同じ文字の先頭 (あ) に正しく移動しました\n");
      pass_count++;
    } else {
      // Windows の実装では curr - 1 を返す可能性があるためチェック
      if (result == curr - 1) {
        printf("  成功: Windows互換の実装（前のバイト位置を返す）\n");
        pass_count++;
      } else {
        printf("  失敗: 期待値 %p または %p, 実際値 %p\n", expected, curr - 1,
               result);
      }
    }
    test_count++;
  }

  // テスト 9: 混合文字列 (1 バイト文字と 2 バイト文字) の処理
  {
    unsigned char str[] = "aあbいc";
    unsigned char* curr = str + 3;      // 'b' を指す
    unsigned char* expected = str + 1;  // 'あ' の先頭を指す
    unsigned char* result = mbsdec(str, curr);

    printf("テスト 9: 混合文字列の処理 (1 バイト文字の前が 2 バイト文字)\n");
    if (result == expected) {
      printf("  成功: 前の文字 (あ) に正しく移動しました\n");
      pass_count++;
    } else {
      printf("  失敗: 期待値 %p, 実際値 %p\n", expected, result);
    }
    test_count++;
  }

  // テスト 10: 混合文字列で 2 バイト文字の前が 1 バイト文字
  {
    unsigned char str[] = "aあbいc";
    unsigned char* curr = str + 4;      // 'い' の先頭を指す
    unsigned char* expected = str + 3;  // 'b' を指す
    unsigned char* result = mbsdec(str, curr);

    printf("テスト 10: 混合文字列の処理 (2 バイト文字の前が 1 バイト文字)\n");
    if (result == expected) {
      printf("  成功: 前の文字 (b) に正しく移動しました\n");
      pass_count++;
    } else {
      printf("  失敗: 期待値 %p, 実際値 %p\n", expected, result);
    }
    test_count++;
  }

  // テスト 11: 2 バイト文字の下位バイトが ismbblead() で true
  // になる可能性があるケース
  {
    // 特殊な 2 バイト文字シーケンスを作成
    unsigned char str[] = {0x81, 0x81, 0x82,
                           0x00};   // 0x81 は先頭バイトとして使われることが多い
    unsigned char* curr = str + 2;  // 2 番目の文字の先頭を指す
    unsigned char* expected = str;  // 1 番目の文字の先頭を指す
    unsigned char* result = mbsdec(str, curr);

    printf(
        "テスト 11: 2 バイト文字の下位バイトが ismbblead() で true "
        "になるケース\n");
    if (result == expected) {
      printf("  成功: 正しく前の文字の先頭を指しました\n");
      pass_count++;
    } else {
      printf("  失敗: 期待値 %p, 実際値 %p\n", expected, result);
    }
    test_count++;
  }

  // テスト 12: 2 バイト文字の連続で、2 バイト目から前の文字を参照
  {
    unsigned char str[] = "漢字ひらがな";
    unsigned char* curr = str + 3;      // '字' の 2 バイト目を指す
    unsigned char* expected = str + 2;  // '字' の 1 バイト目を指す
    unsigned char* result = mbsdec(str, curr);

    printf("テスト 12: 2 バイト文字の連続で 2 バイト目から前の文字を参照\n");
    if (result == expected) {
      printf("  成功: 正しく前のバイト位置を指しました\n");
      pass_count++;
    } else {
      printf("  失敗: 期待値 %p, 実際値 %p\n", expected, result);
    }
    test_count++;
  }

  // テスト 13: 記号を含む日本語文字列
  {
    unsigned char str[] = "あ！？漢";
    unsigned char* curr = str + 6;      // '漢' の先頭を指す
    unsigned char* expected = str + 4;  // '？' の先頭を指す
    unsigned char* result = mbsdec(str, curr);

    printf("テスト 13: 記号を含む日本語文字列\n");
    if (result == expected) {
      printf("  成功: 前の文字 (？) に正しく移動しました\n");
      pass_count++;
    } else {
      printf("  失敗: 期待値 %p, 実際値 %p\n", expected, result);
    }
    test_count++;
  }

  // テスト 14: 文字コード範囲の端の値をテスト
  {
    // 日本語の文字コード範囲の端に近い値
    unsigned char str[] = {0x80, 0x40, 0x9F, 0xFC, 0xE0,
                           0x40, 0xFC, 0xFC, 0x00};
    unsigned char* curr = str + 6;      // 後ろから 3 番目の文字の先頭
    unsigned char* expected = str + 4;  // 後ろから 4 番目の文字の先頭
    unsigned char* result = mbsdec(str, curr);

    printf("テスト 14: 文字コード範囲の端の値\n");
    if (result == expected) {
      printf("  成功: 前の文字に正しく移動しました\n");
      pass_count++;
    } else {
      printf("  失敗: 期待値 %p, 実際値 %p\n", expected, result);
    }
    test_count++;
  }

  // 結果の概要を表示
  printf("\nテスト結果: %d / %d 成功 (%.1f%%)\n", pass_count, test_count,
         (float)pass_count / test_count * 100);

  return (pass_count == test_count) ? 0 : 1;
}
