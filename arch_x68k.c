#include "arch.h"

int is_filesystem_ignore_case(void) {
  int ret;
  __asm__ volatile(
      "move.w	#3, %%sp@-\n"  // _TWON_GETOPT
      ".short	0xffb0\n"      // DOS _TWON
      "addq.l	#2, %%sp\n"
      : "=d"(ret)  // Output operand to capture d0
  );
  // DOS _TWON の戻り値が 0xffffffff でなく、
  // かつ _TWON_C_BIT が立っていれば 0 を返す
  return ret != -1 && ret & (1 << 30) ? 0 : 1;
}
