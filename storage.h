#ifndef STORAGE_H
#define STORAGE_H

#include <stddef.h>

#include "sensor.h"

int saveSensors(const char *filename, const Sensor sensors[], size_t count);

/*
 * The output pointer must refer to a Sensor * initialized to NULL. On
 * success, the caller owns the returned array and must free it.
 */
int loadSensors(const char *filename, Sensor **sensors, size_t *count);

#endif
