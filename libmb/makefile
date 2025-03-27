# ビルド成果物の設定
TARGET = libmb.a

# クロスコンパイル環境の設定
CROSS = m68k-xelf-
CC = $(CROSS)gcc
AS = $(CROSS)as
LD = $(CROSS)gcc
AR = $(CROSS)ar
RANLIB = $(CROSS)ranlib

# コンパイルオプション
CFLAGS = -m68000 -Wall -MMD -I. -O3 -D__common

LIBC_VERSION = 1.1.32a
LIBC_ARCHIVE = v$(LIBC_VERSION).tar.gz
# X68000 LIBC のソースファイル
LIBC_SRCS_CTYPE = \
	_mbctype.c
LIBC_SRCS_MBCTYPE = \
	ismbbalnum.c ismbbalpha.c ismbbgraph.c ismbbkalnum.c \
	ismbbkana.c ismbbkpunct.c ismbblead.c ismbbprint.c ismbbpunct.c \
	ismbbtrail.c ismbcalpha.c ismbcdigit.c ismbchira.c ismbckata.c \
	ismbcl0.c ismbcl1.c ismbcl2.c ismbclegal.c ismbclower.c \
	ismbcprint.c ismbcspace.c ismbcsymbol.c ismbcupper.c mbctohira.c \
	mbctokata.c mbctolower.c mbctoupper.c
LIBC_SRCS_MBSTRING = \
	_mblen.c _mbset.c mbbtombc.c mbbtype.c mbctombb.c mbsbtype.c \
	mbscat.c mbschr.c mbscmp.c mbscpy.c mbscspn.c mbsdup.c mbsicmp.c \
	mbsinc.c mbslen.c mbslwr.c mbsnbcnt.c mbsncat.c mbsnccnt.c \
	mbsncmp.c mbsncpy.c mbsnextc.c mbsnicmp.c mbsninc.c mbspbrk.c \
	mbsrchr.c mbsrev.c mbsset.c mbsspn.c mbsspnp.c mbstok.c mbsupr.c
OBJS = \
	$(patsubst %.c,%.o,$(LIBC_SRCS_CTYPE) $(LIBC_SRCS_MBCTYPE) $(LIBC_SRCS_MBSTRING)) \
	mbsdec.o

# 依存関係ファイル
DEPS = $(patsubst %.o,%.d,$(OBJS))

# テスト用プログラム
TEST_PROGRAMS = test/test_mbsdec.x

.PHONY: all clean test

# デフォルトターゲット
all: $(TARGET)

# テスト実行ターゲット
test: $(TEST_PROGRAMS)

$(TARGET): $(OBJS)
	$(AR) rc $@ $?
	$(RANLIB) $@

$(LIBC_SRCS_CTYPE): %.c: $(LIBC_ARCHIVE)
	@test -f $@ || tar -xvf $< --strip-components=3 libc-src-$(LIBC_VERSION)/src/ctype/$@

$(LIBC_SRCS_MBCTYPE): %.c: $(LIBC_ARCHIVE)
	@test -f $@ || tar -xvf $< --strip-components=3 libc-src-$(LIBC_VERSION)/src/mbctype/$@

$(LIBC_SRCS_MBSTRING): %.c: $(LIBC_ARCHIVE)
	@test -f $@ || tar -xvf $< --strip-components=3 libc-src-$(LIBC_VERSION)/src/mbstring/$@

$(LIBC_ARCHIVE):
	wget https://github.com/kg68k/libc-src/archive/refs/tags/$@

# テストプログラムのビルドルール
test/%.x: test/%.c $(TARGET)
	$(CC) $(CFLAGS) $< -o $@ -L. -lmb

# 依存関係ファイルの取り込み
-include $(DEPS)

# 中間ファイルの削除
clean: 
	-rm -rf *.a *.o *.d *.tar.gz $(LIBC_SRCS_CTYPE) $(LIBC_SRCS_MBCTYPE) $(LIBC_SRCS_MBSTRING) test/*.x
