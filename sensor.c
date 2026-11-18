#include "sensor.h"

#include <math.h>
#include <stdio.h>

static const char *statusName(SensorStatus status)
{
    switch (status) {
    case NORMAL:
        return "NORMAL";
    case WARNING:
        return "WARNING";
    case CRITICAL:
        return "CRITICAL";
    default:
        return "UNKNOWN";
    }
}

Sensor *findSensor(Sensor sensors[], size_t count, int id)
{
    if (sensors == NULL) {
        return NULL;
    }

    for (size_t i = 0; i < count; i++) {
        if (sensors[i].id == id) {
            return &sensors[i];
        }
    }

    return NULL;
}

void updateSensorStatus(Sensor *sensor)
{
    if (sensor == NULL || !isfinite(sensor->temperature)) {
        return;
    }

    if (sensor->temperature < 35.0f) {
        sensor->status = NORMAL;
        sensor->flags = 0;
    } else if (sensor->temperature < 50.0f) {
        sensor->status = WARNING;
        sensor->flags = SENSOR_WARNING_FLAG;
    } else {
        sensor->status = CRITICAL;
        sensor->flags = SENSOR_CRITICAL_FLAG;
    }
}

int updateTemperature(Sensor sensors[], size_t count, int id,
                      float newTemperature)
{
    Sensor *sensor = findSensor(sensors, count, id);

    if (sensor == NULL || !isfinite(newTemperature)) {
        return 0;
    }

    sensor->temperature = newTemperature;
    updateSensorStatus(sensor);
    return 1;
}

int calculateStatistics(const Sensor sensors[], size_t count,
                        SensorStatistics *statistics)
{
    if (sensors == NULL || count == 0 || statistics == NULL) {
        return 0;
    }

    float minimum = sensors[0].temperature;
    float maximum = sensors[0].temperature;
    if (!isfinite(minimum)) {
        return 0;
    }

    double sum = 0.0;
    for (size_t i = 0; i < count; i++) {
        float temperature = sensors[i].temperature;
        if (!isfinite(temperature)) {
            return 0;
        }

        if (temperature < minimum) {
            minimum = temperature;
        }
        if (temperature > maximum) {
            maximum = temperature;
        }
        sum += temperature;
    }

    double average = sum / (double)count;
    if (!isfinite(average)) {
        return 0;
    }

    *statistics = (SensorStatistics){count, minimum, maximum, average};
    return 1;
}

void printSensors(const Sensor sensors[], size_t count)
{
    if (sensors == NULL || count == 0) {
        printf("No sensors available!\n");
        return;
    }

    for (size_t i = 0; i < count; i++) {
        printf("Sensor ID: %d | Temperature: %.2f | Status: %s | Flags: %u\n",
               sensors[i].id, sensors[i].temperature,
               statusName(sensors[i].status), (unsigned int)sensors[i].flags);
    }
}

void showStatistics(const Sensor sensors[], size_t count)
{
    SensorStatistics statistics;
    if (!calculateStatistics(sensors, count, &statistics)) {
        printf("No valid sensor statistics available!\n");
        return;
    }

    printf("\n========== SENSOR STATISTICS ==========\n");
    printf("Total Sensors: %zu\n", statistics.count);
    printf("Minimum Temperature: %.2f C\n", statistics.minimum);
    printf("Maximum Temperature: %.2f C\n", statistics.maximum);
    printf("Average Temperature: %.2f C\n", statistics.average);
    printf("=======================================\n");
}
