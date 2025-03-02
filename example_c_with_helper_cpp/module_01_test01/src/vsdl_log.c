#include "vsdl_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static FILE* logFile = NULL;
static bool fileOutputEnabled = false;

void vsdl_init_log(const char* filename, bool enableFileOutput) {
    fileOutputEnabled = enableFileOutput;
    if (fileOutputEnabled) {
        logFile = fopen(filename, "w");
        if (!logFile) {
            fprintf(stderr, "Failed to open log file %s\n", filename);
            fileOutputEnabled = false;
        } else {
            time_t now = time(NULL);
            fprintf(logFile, "Log started at %s", ctime(&now));
            fflush(logFile);
        }
    }
}

void vsdl_log(const char* format, ...) {
    va_list args;
    va_start(args, format);

    // Print to stdout
    vprintf(format, args);

    // Print to file if enabled
    if (fileOutputEnabled && logFile) {
        vfprintf(logFile, format, args);
        fflush(logFile);
    }

    va_end(args);
}

void vsdl_cleanup_log(void) {
    if (logFile) {
        time_t now = time(NULL);
        fprintf(logFile, "Log ended at %s", ctime(&now));
        fclose(logFile);
        logFile = NULL;
    }
}

void vsdl_toggle_log_file(bool enable) {
    fileOutputEnabled = enable;
    if (enable && !logFile) {
        logFile = fopen("debug.log", "a"); // Reopen in append mode if previously closed
        if (!logFile) {
            fprintf(stderr, "Failed to reopen log file\n");
            fileOutputEnabled = false;
        }
    }
}