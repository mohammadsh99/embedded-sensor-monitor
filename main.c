#include <stdio.h>
#include <stdlib.h>

#include "sensor.h"
#include "storage.h"

int main(void)
{
    size_t count = 3;
    Sensor *sensors = malloc(count * sizeof(*sensors));

    if (sensors == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        return 1;
    }

    sensors[0] = (Sensor){101, 25.0f, NORMAL, 0};
    sensors[1] = (Sensor){102, 40.0f, WARNING, 1};
    sensors[2] = (Sensor){103, 60.0f, CRITICAL, 2};

    printf("\n=== Initial Sensors ===\n");
    printSensors(sensors, count);

    size_t newCount = count + 1;
    Sensor *resizedSensors = realloc(sensors, newCount * sizeof(*sensors));
    if (resizedSensors == NULL) {
        fprintf(stderr, "Memory reallocation failed.\n");
        free(sensors);
        return 1;
    }

    sensors = resizedSensors;
    sensors[count] = (Sensor){104, 32.0f, NORMAL, 0};
    count = newCount;

    printf("\n=== After Adding Sensor ===\n");
    printSensors(sensors, count);

    Sensor *found = findSensor(sensors, count, 102);
    if (found != NULL) {
        printf("\nFound sensor: %d\n", found->id);
    } else {
        printf("\nSensor not found!\n");
    }

    if (updateTemperature(sensors, count, 102, 45.5f)) {
        printf("\nTemperature updated successfully!\n");
    }

    printf("\n=== After First Update ===\n");
    printSensors(sensors, count);
    showStatistics(sensors, count);

    if (updateTemperature(sensors, count, 102, 55.0f)) {
        printf("\nSensor 102 updated to 55 degrees!\n");
    }

    printf("\n=== After Critical Update ===\n");
    printSensors(sensors, count);
    showStatistics(sensors, count);

    if (!updateTemperature(sensors, count, 999, 70.0f)) {
        printf("\nSensor 999 not found!\n");
    }

    int exitCode = 0;
    printf("\n=== Saving Sensors ===\n");
    if (saveSensors("sensors.csv", sensors, count)) {
        printf("Sensors saved successfully!\n");
    } else {
        fprintf(stderr, "Failed to save sensors!\n");
        exitCode = 1;
    }

    Sensor *loadedSensors = NULL;
    size_t loadedCount = 0;

    printf("\n=== Loading Sensors ===\n");
    if (loadSensors("sensors.csv", &loadedSensors, &loadedCount)) {
        printf("Sensors loaded successfully!\n");
        printSensors(loadedSensors, loadedCount);
        showStatistics(loadedSensors, loadedCount);
    } else {
        fprintf(stderr, "Failed to load sensors!\n");
        exitCode = 1;
    }

    free(loadedSensors);
    free(sensors);
    printf("\nMemory freed successfully.\n");

    return exitCode;
}
