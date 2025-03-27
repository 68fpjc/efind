#include <mbctype.h>
#include <mbstring.h>

/**
 * @brief 指定されたポインタの前のマルチバイト文字の先頭ポインタを返す
 *
 * Windows の mbsdec() の挙動に合わせて実装
 *
 * @param[in] start マルチバイト文字列の先頭ポインタ
 * @param[in] current 現在位置のポインタ
 * @return 前のマルチバイト文字の先頭ポインタ、ない場合は NULL
 */
unsigned char* mbsdec(const unsigned char* start, unsigned char* current) {
  unsigned char* p;
  unsigned char* q;

  /* 現在位置が文字列の先頭かそれ以前の場合は NULL を返す */
  if (current <= start) return NULL;

  /* カレントポインタが 2 バイト文字の2バイト目を指していた場合 */
  if (current > start && ismbblead(*(current - 2)) &&
      !ismbblead(*(current - 1))) {
    return (unsigned char*)(current - 2);
  }

  /* 前のバイトへ移動 */
  p = current - 1;

  /* 文字列の最初から走査して、前の文字の先頭位置を正確に特定する */
  q = (unsigned char*)start;
  while (q < p) {
    if (ismbblead(*q) && q + 1 < current) {
      /* 2 バイト文字の先頭バイトを検出 */
      if (q + 2 == current) {
        /* 前の文字は 2 バイト文字で、 current はその次の位置を指している */
        return q;
      }
      if (q + 2 <= p) {
        /* 2 バイト文字をスキップ */
        q += 2;
      } else {
        /* p が 2 バイト文字の途中を指している場合 */
        return q;
      }
    } else {
      /* 1 バイト文字をスキップ */
      q++;
    }
  }

  return p;
}
