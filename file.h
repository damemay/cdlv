#ifndef CDLV_FILE_H
#define CDLV_FILE_H

#include "cdlv.h"

cdlv_error cdlv_read_file_to_str(cdlv* base, const char* path, char** string);
cdlv_error cdlv_read_file_in_lines(cdlv* base, const char* path, size_t* line_count, char*** string_array);
void cdlv_free_file_in_lines(char** file, const size_t line_count);

#endif
