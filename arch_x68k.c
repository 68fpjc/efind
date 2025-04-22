#include <ctype.h>
#include <dirent.h>
#include <mbstring.h>
#include <string.h>
#include <x68k/dos.h>

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

int is_directory_entry(struct dirent *entry) { return entry->d_type == DT_DIR; }

int get_file_attributes(const char *path) {
  int dos_attr;
  int result = 0;
  dos_attr = _dos_chmod(path, -1);
  if (_DOS_ISLNK(dos_attr)) {
    result |= FILE_ATTR_SYMLINK;
  }
  if (dos_attr & _DOS_IEXEC) {
    result |= FILE_ATTR_EXECUTABLE;
  }
  return result;
}

int is_path_end_with_separator(const char *path) {
  const unsigned char *p = (const unsigned char *)path;
  const unsigned char *p_prev = NULL;
  while (*p) {
    p_prev = p;
    p = mbsinc((unsigned char *)p);
  }
  return p_prev && (*p_prev == '/' || *p_prev == '\\');
}

int should_append_dot(const char *path) {
  // path がドライブレターのみの場合は、"." を付加する
  return strlen(path) == 2 && isalpha((unsigned char)path[0]) && path[1] == ':';
}
