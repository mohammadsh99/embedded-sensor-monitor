#include "storage.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CSV_LINE_SIZE 256

static int readLine(FILE *file, char line[CSV_LINE_SIZE])
{
    if (fgets(line, CSV_LINE_SIZE, file) == NULL) {
        return ferror(file) ? -1 : 0;
    }

    char *newline = strchr(line, '\n');
    if (newline != NULL) {
        *newline = '\0';
        size_t length = strlen(line);
        if (length > 0 && line[length - 1] == '\r') {
            line[length - 1] = '\0';
        }
        return 1;
    }

    if (!feof(file)) {
        int character;
        while ((character = fgetc(file)) != '\n' && character != EOF) {
        }
        return -1;
    }

    return 1;
}

static char *skipWhitespace(char *text)
{
    while (isspace((unsigned char)*text)) {
        text++;
    }
    return text;
}

static int hasOnlyTrailingWhitespace(char *text)
{
    return *skipWhitespace(text) == '\0';
}

static int parseInteger(char *text, int *value)
{
    errno = 0;
    char *end = NULL;
    long parsed = strtol(text, &end, 10);
    if (end == text || errno == ERANGE || parsed < INT_MIN ||
        parsed > INT_MAX || !hasOnlyTrailingWhitespace(end)) {
        return 0;
    }

    *value = (int)parsed;
    return 1;
}

static int parseTemperature(char *text, float *value)
{
    errno = 0;
    char *end = NULL;
    float parsed = strtof(text, &end);
    if (end == text || errno == ERANGE || !isfinite(parsed) ||
        !hasOnlyTrailingWhitespace(end)) {
        return 0;
    }

    *value = parsed;
    return 1;
}

static int parseFlags(char *text, uint8_t *value)
{
    if (*skipWhitespace(text) == '-') {
        return 0;
    }

    errno = 0;
    char *end = NULL;
    unsigned long parsed = strtoul(text, &end, 10);
    if (end == text || errno == ERANGE || parsed > UINT8_MAX ||
        !hasOnlyTrailingWhitespace(end)) {
        return 0;
    }

    *value = (uint8_t)parsed;
    return 1;
}

static int parseSensorLine(char *line, Sensor *sensor)
{
    char *fields[4];
    fields[0] = line;

    for (size_t i = 0; i < 3; i++) {
        char *comma = strchr(fields[i], ',');
        if (comma == NULL) {
            return 0;
        }
        *comma = '\0';
        fields[i + 1] = comma + 1;
    }

    if (strchr(fields[3], ',') != NULL) {
        return 0;
    }

    int id;
    float temperature;
    int status;
    uint8_t flags;
    if (!parseInteger(fields[0], &id) ||
        !parseTemperature(fields[1], &temperature) ||
        !parseInteger(fields[2], &status) ||
        !parseFlags(fields[3], &flags) ||
        status < NORMAL || status > CRITICAL) {
        return 0;
    }

    *sensor = (Sensor){id, temperature, (SensorStatus)status, flags};
    return 1;
}

int saveSensors(const char *filename, const Sensor sensors[], size_t count)
{
    if (filename == NULL || sensors == NULL || count == 0) {
        return 0;
    }

    for (size_t i = 0; i < count; i++) {
        if (!isfinite(sensors[i].temperature) ||
            sensors[i].status < NORMAL || sensors[i].status > CRITICAL) {
            return 0;
        }
    }

    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        return 0;
    }

    int success = fprintf(file, "id,temperature,status,flags\n") >= 0;
    for (size_t i = 0; success && i < count; i++) {
        success = fprintf(file, "%d,%.2f,%d,%u\n", sensors[i].id,
                          sensors[i].temperature, (int)sensors[i].status,
                          (unsigned int)sensors[i].flags) >= 0;
    }

    if (fclose(file) != 0) {
        success = 0;
    }

    return success;
}

int loadSensors(const char *filename, Sensor **sensors, size_t *count)
{
    if (filename == NULL || sensors == NULL || count == NULL ||
        *sensors != NULL) {
        return 0;
    }

    *count = 0;
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        return 0;
    }

    Sensor *loaded = NULL;
    size_t loadedCount = 0;
    size_t capacity = 0;
    int success = 0;
    char line[CSV_LINE_SIZE];

    int lineResult = readLine(file, line);
    if (lineResult != 1 || strcmp(line, "id,temperature,status,flags") != 0) {
        goto cleanup;
    }

    for (;;) {
        lineResult = readLine(file, line);
        if (lineResult == 0) {
            break;
        }
        if (lineResult < 0) {
            goto cleanup;
        }

        Sensor sensor;
        if (!parseSensorLine(line, &sensor) ||
            findSensor(loaded, loadedCount, sensor.id) != NULL) {
            goto cleanup;
        }

        if (loadedCount == capacity) {
            size_t maximumCapacity = SIZE_MAX / sizeof(*loaded);
            if (capacity >= maximumCapacity) {
                goto cleanup;
            }

            size_t newCapacity = capacity == 0 ? 4 : capacity;
            if (capacity != 0) {
                newCapacity = capacity > maximumCapacity / 2
                                  ? maximumCapacity
                                  : capacity * 2;
            }

            Sensor *resized = realloc(loaded, newCapacity * sizeof(*loaded));
            if (resized == NULL) {
                goto cleanup;
            }

            loaded = resized;
            capacity = newCapacity;
        }

        updateSensorStatus(&sensor);
        loaded[loadedCount++] = sensor;
    }

    if (ferror(file) || loadedCount == 0) {
        goto cleanup;
    }
    success = 1;

cleanup:
    if (fclose(file) != 0) {
        success = 0;
    }

    if (!success) {
        free(loaded);
        return 0;
    }

    *sensors = loaded;
    *count = loadedCount;
    return 1;
}
