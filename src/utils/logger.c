#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LOG_FILE_PATH "app_logs.json"

static void getCurrentTime(char *buffer, size_t len) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, len, "%Y-%m-%dT%H:%M:%S", t);
}

static void writeJsonLogs(const char *jsonLog) {
    if (!jsonLog) return;

    FILE *f = fopen(LOG_FILE_PATH, "a");
    if (!f) {
        perror("Failed to open log file");
        return;
    }

    fprintf(f, "%s\n", jsonLog);
    fclose(f);
}

void createApplicationLog(const char *logType, int code, const char *message) {
    if (!logType || !message) return;

    char startTime[32], endTime[32];
    getCurrentTime(startTime, sizeof(startTime));
    getCurrentTime(endTime, sizeof(endTime));

    char json[512 + strlen(logType) + strlen(message)];
    snprintf(json, sizeof(json),
        "{ \"LOG_TYPE\": \"%s\", \"start_time\": \"%s\", \"end_time\": \"%s\", \"code\": %d, \"message\": \"%s\" }",
        logType, startTime, endTime, code, message);

    writeJsonLogs(json);
}