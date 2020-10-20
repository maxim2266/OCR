# Disable built-in rules and variables
MAKEFLAGS += --no-builtin-rules
MAKEFLAGS += --no-builtin-variables

# version spec
VER_FILE := version

# toolset version
VER := $(shell head -n 1 $(VER_FILE))

# programs to compile
PROGS := ocr-open ocr-ls ocr

# other scripts
SCRIPTS := crop-image norm-image norm-text norm-page

# flags
CFLAGS := -O2 -s -std=c11 -Wall -Wextra -Wformat -Wl,--strip-all	\
          -DPROG_VER=\"$(VER)\" -D_GNU_SOURCE

# source directory
SRC := src

# release file
RELEASE_FILE := ocr-$(VER).tar.xz

# compile all
.PHONY: all
all: $(PROGS) $(SCRIPTS)

# release
.PHONY: release
release: $(RELEASE_FILE)

$(RELEASE_FILE): $(PROGS) $(SCRIPTS) LICENSE
	tar -caf $@ $^

# clean-up
.PHONY: clean
clean:
	rm -f $(PROGS) $(RELEASE_FILE)

# version update for scripts
$(SCRIPTS): $(VER_FILE)
	sed -i -E 's/^VER=.+/VER=\"$(VER)\"/' $@

# version update for compiled programs
$(PROGS): $(VER_FILE)

# programs ----------------------------------------------------------------------
COMMON_SRC := utils.c utils.h page_spec.c page_spec.h str.h str.c

# ocr-open
OCR_OPEN_SRC := $(COMMON_SRC) ocr_open.c

ocr-open: $(addprefix $(SRC)/,$(OCR_OPEN_SRC))
	gcc $(CFLAGS) -DPROG_NAME=\"$@\" -o $@ $(filter %.c,$^) -lmagic

# ocr-ls
OCR_LS_SRC := $(COMMON_SRC) ocr_ls.c list_pages.h list_pages.c

ocr-ls: $(addprefix $(SRC)/,$(OCR_LS_SRC))
	gcc $(CFLAGS) -DPROG_NAME=\"$@\" -o $@ $(filter %.c,$^)

# ocr
OCR_SRC := $(COMMON_SRC) ocr.c tesseract.h tesseract.c list_pages.h list_pages.c

ocr: $(addprefix $(SRC)/,$(OCR_SRC))
	gcc $(CFLAGS) -DPROG_NAME=\"$@\" -o $@ $(filter %.c,$^)
