# Disable built-in rules and variables
MAKEFLAGS += --no-builtin-rules
MAKEFLAGS += --no-builtin-variables

# toolset version
TOOLSET_VER := $(shell head -n 1 version)

# programs to compile
PROGS := ocr-open

# flags
CFLAGS := $(filter-out -g,$(shell dpkg-buildflags --get CFLAGS))	\
          $(shell dpkg-buildflags --get LDFLAGS)	\
          -s -std=c11 -Wall -Wextra -Wformat -Wl,--strip-all	\
          -DPROG_VER=\"$(TOOLSET_VER)\"

# source directory
SRC := src

# release file
RELEASE_FILE := ocr-$(TOOLSET_VER).tar.xz

# compile all
.PHONY: all
all: $(PROGS)

# release
.PHONY: release
release: $(RELEASE_FILE)

$(RELEASE_FILE): $(PROGS) LICENSE
	tar -caf $@ $^

# clean-up
.PHONY: clean
clean:
	rm -f $(PROGS) $(RELEASE_FILE)

# programs ----------------------------------------------------------------------
COMMON_SRC := utils.c utils.h

# ocr-open
OCR_OPEN_SRC = $(COMMON_SRC) ocr-open.c page_spec.c page_spec.h

ocr-open: $(addprefix $(SRC)/,$(OCR_OPEN_SRC))
	gcc $(CFLAGS) -DPROG_NAME=\"$@\" -o $@ $(filter %.c,$+) -lmagic
