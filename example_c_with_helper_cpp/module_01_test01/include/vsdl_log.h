#ifndef VSDL_LOG_H
#define VSDL_LOG_H

#include <stdbool.h>
#include <stdarg.h>

void vsdl_init_log(const char* filename, bool enableFileOutput);
void vsdl_log(const char* format, ...);
void vsdl_cleanup_log(void);
void vsdl_toggle_log_file(bool enable);

#endif