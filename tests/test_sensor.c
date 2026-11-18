#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../sensor.h"
#include "../storage.h"

static unsigned int totalTests;
static unsigned int passedTests;

#define CHECK(condition)                                                     \
    do {                                                                      \
        if (!(condition)) {                                                   \
            fprintf(stderr, "  check failed at %s:%d: %s\n", __FILE__,       \
                    __LINE__, #condition);                                    \
            return 0;                                                         \
        }                                                                     \
    } while (0)

static int writeCsv(const char *filename, const char *contents)
{
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        return 0;
    }

    int success = fputs(contents, file) >= 0;
    if (fclose(file) != 0) {
        success = 0;
    }
    return success;
}

static int testFindingSensors(void)
{
    Sensor sensors[] = {
        {101, 20.0f, NORMAL, 0},
        {102, 40.0f, WARNING, SENSOR_WARNING_FLAG}
    };

    CHECK(findSensor(sensors, 2, 102) == &sensors[1]);
    CHECK(findSensor(sensors, 2, 999) == NULL);
    CHECK(findSensor(NULL, 2, 101) == NULL);
    CHECK(findSensor(sensors, 0, 101) == NULL);
    return 1;
}

static int testTemperatureThresholdsAndFlags(void)
{
    Sensor sensors[] = {{1, 0.0f, NORMAL, 0}};

    CHECK(updateTemperature(sensors, 1, 1, 34.99f));
    CHECK(sensors[0].status == NORMAL);
    CHECK(sensors[0].flags == 0);

    CHECK(updateTemperature(sensors, 1, 1, 35.0f));
    CHECK(sensors[0].status == WARNING);
    CHECK((sensors[0].flags & SENSOR_WARNING_FLAG) != 0);
    CHECK((sensors[0].flags & SENSOR_CRITICAL_FLAG) == 0);

    CHECK(updateTemperature(sensors, 1, 1, 49.99f));
    CHECK(sensors[0].status == WARNING);
    CHECK(sensors[0].flags == SENSOR_WARNING_FLAG);

    CHECK(updateTemperature(sensors, 1, 1, 50.0f));
    CHECK(sensors[0].status == CRITICAL);
    CHECK((sensors[0].flags & SENSOR_CRITICAL_FLAG) != 0);
    CHECK((sensors[0].flags & SENSOR_WARNING_FLAG) == 0);

    CHECK(updateTemperature(sensors, 1, 1, 55.0f));
    CHECK(sensors[0].status == CRITICAL);
    CHECK(sensors[0].flags == SENSOR_CRITICAL_FLAG);
    CHECK(!updateTemperature(sensors, 1, 999, 10.0f));
    CHECK(!updateTemperature(sensors, 1, 1, NAN));
    CHECK(!updateTemperature(sensors, 1, 1, INFINITY));
    CHECK(sensors[0].temperature == 55.0f);
    return 1;
}

static int testStatistics(void)
{
    Sensor sensors[] = {
        {1, -5.0f, NORMAL, 0},
        {2, 25.0f, NORMAL, 0},
        {3, 40.0f, WARNING, SENSOR_WARNING_FLAG}
    };
    SensorStatistics statistics;

    CHECK(calculateStatistics(sensors, 3, &statistics));
    CHECK(statistics.count == 3);
    CHECK(statistics.minimum == -5.0f);
    CHECK(statistics.maximum == 40.0f);
    CHECK(fabs(statistics.average - 20.0) < 0.0001);
    CHECK(!calculateStatistics(NULL, 3, &statistics));
    CHECK(!calculateStatistics(sensors, 0, &statistics));
    CHECK(!calculateStatistics(sensors, 3, NULL));
    return 1;
}

static int testSavingAndLoadingCsv(void)
{
    const char *filename = "test_sensors.csv";
    Sensor sensors[] = {
        {101, 25.0f, NORMAL, 0},
        {102, 40.0f, WARNING, SENSOR_WARNING_FLAG},
        {103, 60.0f, CRITICAL, SENSOR_CRITICAL_FLAG}
    };
    Sensor *loaded = NULL;
    size_t loadedCount = 0;

    CHECK(saveSensors(filename, sensors, 3));
    CHECK(loadSensors(filename, &loaded, &loadedCount));
    CHECK(loadedCount == 3);
    CHECK(loaded[0].id == 101);
    CHECK(loaded[0].temperature == 25.0f);
    CHECK(loaded[0].status == NORMAL);
    CHECK(loaded[1].status == WARNING);
    CHECK(loaded[1].flags == SENSOR_WARNING_FLAG);
    CHECK(loaded[2].status == CRITICAL);
    CHECK(loaded[2].flags == SENSOR_CRITICAL_FLAG);

    free(loaded);
    CHECK(remove(filename) == 0);

    CHECK(writeCsv(filename,
                   "id,temperature,status,flags\n104,40.00,0,2\n"));
    loaded = NULL;
    loadedCount = 0;
    CHECK(loadSensors(filename, &loaded, &loadedCount));
    CHECK(loadedCount == 1);
    CHECK(loaded[0].status == WARNING);
    CHECK(loaded[0].flags == SENSOR_WARNING_FLAG);

    free(loaded);
    CHECK(remove(filename) == 0);
    return 1;
}

static int testInvalidCsv(void)
{
    const char *filename = "test_invalid_sensors.csv";
    const char *invalidFiles[] = {
        "id,temperature,status,flags\nabc,20,0,0\n",
        "id,temperature,status,flags\n2147483648,20,0,0\n",
        "id,temperature,status,flags\n1,nan,0,0\n",
        "id,temperature,status,flags\n1,20,9,0\n",
        "id,temperature,status,flags\n1,20,0,256\n",
        "id,temperature,status,flags\n1,20,0,0,extra\n",
        "id,temperature,status,flags\n1,20,0,0\n1,21,0,0\n",
        "wrong,header\n1,20,0,0\n",
        "id,temperature,status,flags\n"
    };

    for (size_t i = 0; i < sizeof(invalidFiles) / sizeof(invalidFiles[0]);
         i++) {
        Sensor *loaded = NULL;
        size_t loadedCount = 0;
        CHECK(writeCsv(filename, invalidFiles[i]));
        CHECK(!loadSensors(filename, &loaded, &loadedCount));
        CHECK(loaded == NULL);
        CHECK(loadedCount == 0);
        CHECK(remove(filename) == 0);
    }

    char oversized[384];
    size_t position = (size_t)snprintf(
        oversized, sizeof(oversized), "id,temperature,status,flags\n1,");
    memset(oversized + position, '9', 300);
    position += 300;
    memcpy(oversized + position, ",0,0\n", 6);
    CHECK(writeCsv(filename, oversized));
    Sensor *loaded = NULL;
    size_t loadedCount = 0;
    CHECK(!loadSensors(filename, &loaded, &loadedCount));
    CHECK(remove(filename) == 0);
    return 1;
}

static int testMissingFileAndOutputOwnership(void)
{
    const char *filename = "file_that_does_not_exist.csv";
    remove(filename);
    Sensor *loaded = NULL;
    size_t loadedCount = 17;

    CHECK(!loadSensors(filename, &loaded, &loadedCount));
    CHECK(loaded == NULL);
    CHECK(loadedCount == 0);

    Sensor existing[] = {{1, 20.0f, NORMAL, 0}};
    Sensor *existingPointer = existing;
    loadedCount = 1;
    CHECK(!loadSensors(filename, &existingPointer, &loadedCount));
    CHECK(existingPointer == existing);
    CHECK(loadedCount == 1);
    return 1;
}

static int testDynamicResizing(void)
{
    const char *filename = "test_many_sensors.csv";
    FILE *file = fopen(filename, "w");
    CHECK(file != NULL);
    CHECK(fputs("id,temperature,status,flags\n", file) >= 0);
    for (int id = 1; id <= 25; id++) {
        CHECK(fprintf(file, "%d,%d,0,0\n", id, id) >= 0);
    }
    CHECK(fclose(file) == 0);

    Sensor *loaded = NULL;
    size_t loadedCount = 0;
    CHECK(loadSensors(filename, &loaded, &loadedCount));
    CHECK(loadedCount == 25);
    CHECK(loaded[0].id == 1);
    CHECK(loaded[24].id == 25);
    CHECK(loaded[24].temperature == 25.0f);

    free(loaded);
    CHECK(remove(filename) == 0);
    return 1;
}

static void runTest(const char *name, int (*testFunction)(void))
{
    totalTests++;
    printf("[ RUN      ] %s\n", name);
    if (testFunction()) {
        passedTests++;
        printf("[       OK ] %s\n", name);
    } else {
        printf("[  FAILED  ] %s\n", name);
    }
}

int main(void)
{
    runTest("finding existing and missing sensors", testFindingSensors);
    runTest("temperature thresholds and bitwise flags",
            testTemperatureThresholdsAndFlags);
    runTest("minimum, maximum, and average statistics", testStatistics);
    runTest("saving and loading CSV", testSavingAndLoadingCsv);
    runTest("rejecting invalid CSV data", testInvalidCsv);
    runTest("missing files and output ownership",
            testMissingFileAndOutputOwnership);
    runTest("dynamic array resizing", testDynamicResizing);

    printf("\n%u/%u tests passed.\n", passedTests, totalTests);
    return passedTests == totalTests ? 0 : 1;
}
