PROGRAM = efind
VERSION = 0.1.6-dev

# ビルド成果物の設定
TARGET = $(PROGRAM).x
ARCHIVE = $(PROGRAM)-$(VERSION).zip
DISTDIR = dist

LIBMB_VERSION = 20240408-01
LIBMB_ARC = libmb-$(LIBMB_VERSION).zip
LIBMB_URL = https://github.com/68fpjc/libmb/releases/download/$(LIBMB_VERSION)/$(LIBMB_ARC)
LIBMB_DIR = libmb
LIBMB_INCLUDE_DIR = $(LIBMB_DIR)
LIBMB_LIB_DIR = $(LIBMB_DIR)
LIBMB_LIB = $(LIBMB_LIB_DIR)/libmb.a

# クロスコンパイル環境の設定
CROSS = m68k-xelf-
CC = $(CROSS)gcc
AS = $(CROSS)as
LD = $(CROSS)gcc

# コンパイルオプション
CFLAGS_COMMON = -m68000 -Wall -MMD -Ilibmb -D_GNU_SOURCE -DPROGRAM=\"$(PROGRAM)\" -DVERSION=\"$(VERSION)\"
ifdef RELEASE_BUILD
  CFLAGS = $(CFLAGS_COMMON) -O3  # リリースビルド
else
  CFLAGS = $(CFLAGS_COMMON) -O0 -g  # 開発ビルド : デバッグ情報付き
endif
LDFLAGS = -Llibmb
OBJS = main.o efind.o arch_x68k.o  # コンパイル対象のオブジェクトファイル
LDLIBS = -lmb
DEPS = $(patsubst %.o,%.d,$(OBJS))

# ターゲット定義
.PHONY: all extra-headers test clean veryclean release bump-version

# デフォルトターゲット : 実行ファイルのビルド
all: extra-headers $(TARGET)

# 実行ファイルのリンク
$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) $^ $(LDLIBS) -o $@

extra-headers: $(LIBMB_INCLUDE_DIR)/mbctype.h $(LIBMB_INCLUDE_DIR)/mbstring.h

$(LIBMB_INCLUDE_DIR)/mbctype.h: | $(LIBMB_LIB)
$(LIBMB_INCLUDE_DIR)/mbstring.h: | $(LIBMB_LIB)

# libcondrv をダウンロードする
$(LIBCONDRV_DIR)/$(LIBCONDRV_ARC):
	wget -q -P $(LIBCONDRV_DIR) $(LIBCONDRV_URL)

# libmb を展開する
$(LIBMB_LIB): | $(LIBMB_DIR)/$(LIBMB_ARC)
	7z x -y $(LIBMB_DIR)/$(LIBMB_ARC) -o$(LIBMB_LIB_DIR)

# libmb をダウンロードする
$(LIBMB_DIR)/$(LIBMB_ARC):
	wget -q -P $(LIBMB_DIR) $(LIBMB_URL)

# テストプログラム
TESTTARGET = test/test_match_pattern.x test/test_arch_x68k.x
TESTDEPS = $(patsubst %.x,%.d,$(TESTTARGET))

# テストプログラムのビルド
test: $(TESTTARGET)

# テストプログラムのリンク
test/%.x: test/%.o arch_x68k.o
	$(LD) $(LDFLAGS) $^ $(LDLIBS) -o $@

# 依存関係ファイルの取り込み
-include $(DEPS)
-include $(TESTDEPS)

# 中間ファイルの削除
clean:
	-rm -f *.x *.o *.elf* *.d
	-rm -rf $(LIBMB_DIR)/*
	-rm -f test/*.x test/*.o test/*.elf* test/*.d

# 配布ディレクトリを含めた完全クリーン
veryclean: clean
	-rm -rf $(DISTDIR)

# リリースビルドの作成
release:
	$(MAKE) clean
	$(MAKE) RELEASE_BUILD=yes
	mkdir -p $(DISTDIR)
	mv $(TARGET) $(DISTDIR)
	pandoc -f markdown -t plain README.md | iconv -t cp932 >$(DISTDIR)/README.txt
	cd $(DISTDIR) && 7z a $(PROGRAM)-$(VERSION).zip $(TARGET) README.txt
	$(MAKE) clean

# バージョン番号の更新
bump-version:
	@echo "Current version: $(VERSION)"
	@read -p "New version: " new_version && \
	sed -i "s/VERSION = $(VERSION)/VERSION = $$new_version/" makefile
