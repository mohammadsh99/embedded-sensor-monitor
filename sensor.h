#ifndef SENSOR_H
#define SENSOR_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    NORMAL = 0,
    WARNING = 1,
    CRITICAL = 2
} SensorStatus;

typedef struct {
    int id;
    float temperature;
    SensorStatus status;
    uint8_t flags;
} Sensor;

typedef struct {
    size_t count;
    float minimum;
    float maximum;
    double average;
} SensorStatistics;

#define SENSOR_WARNING_FLAG ((uint8_t)(1u << 0))
#define SENSOR_CRITICAL_FLAG ((uint8_t)(1u << 1))

Sensor *findSensor(Sensor sensors[], size_t count, int id);
void updateSensorStatus(Sensor *sensor);
int updateTemperature(Sensor sensors[], size_t count, int id,
                      float newTemperature);
int calculateStatistics(const Sensor sensors[], size_t count,
                        SensorStatistics *statistics);
void printSensors(const Sensor sensors[], size_t count);
void showStatistics(const Sensor sensors[], size_t count);

#endif
