#pragma once

#include "str.h"

// check tesseract presence and version
void tess_check(void);

// read list of installed languages
str_list* tess_langs(void);

// extract text from the given file
void tess_extract_text(const str file, const char** opts, const unsigned num_opts);