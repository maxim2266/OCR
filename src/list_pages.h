#pragma once

#include "page_spec.h"
#include "str.h"

unsigned page_no(const str name, const str ext);

str_list* list_files(const char* const dir,
					 const page_spec* const spec,
					 const char* const ext);