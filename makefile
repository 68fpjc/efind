PROGRAM = efind
VERSION = 0.1.2

# ビルド成果物の設定
TARGET = $(PROGRAM).x
ARCHIVE = $(PROGRAM)-$(VERSION).zip
DISTDIR = dist

# クロスコンパイル環境の設定
CROSS = m68k-xelf-
CC = $(CROSS)gcc
AS = $(CROSS)as
LD = $(CROSS)gcc

# コンパイルオプション
CFLAGS_COMMON = -m68000 -Wall -MMD -Ilibmb -DPROGRAM=\"$(PROGRAM)\" -DVERSION=\"$(VERSION)\"
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
.PHONY: all test clean veryclean release bump-version

# デフォルトターゲット : 実行ファイルのビルド
all: $(TARGET)

# 実行ファイルのリンク
$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) $^ $(LDLIBS) -o $@

# テストプログラム
TESTTARGET = test/test_match_pattern.x test/test_join_paths.x test/test_fs_ic.x
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
