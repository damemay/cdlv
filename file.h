#ifndef CDLV_FILE_H
#define CDLV_FILE_H

#include <stdio.h>

char* cdlv_read_file_to_str(const char* path);
char** cdlv_read_file_in_lines(const char* path, size_t* line_count);
void cdlv_free_file_in_lines(char** file, const size_t line_count);

#endif
