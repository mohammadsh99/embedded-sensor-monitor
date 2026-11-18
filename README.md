# Embedded Sensor Monitoring & Data Logger

A beginner-friendly C program that tracks sensor temperatures, derives status
and alert flags, calculates summary statistics, and saves or loads readings
from a CSV file.

## Features

- Stores sensor IDs, temperatures, statuses, and bitwise alert flags.
- Searches for a sensor by ID and updates its temperature.
- Classifies temperatures below 35°C as `NORMAL`, from 35°C up to (but not
  including) 50°C as `WARNING`, and at or above 50°C as `CRITICAL`.
- Calculates the minimum, maximum, and average temperature.
- Saves and loads sensor data in CSV format with input validation.
- Uses dynamically allocated arrays and checks allocation and file errors.

## Technologies

- C11
- GCC and GNU Make
- Standard C library; no third-party dependencies

## Project structure

```text
embedded-sensor-monitor/
├── main.c                 # Demonstration and application flow
├── sensor.h               # Sensor types and function declarations
├── sensor.c               # Status, search, update, printing, statistics
├── storage.h              # CSV persistence declarations
├── storage.c              # CSV parsing and file operations
├── tests/
│   └── test_sensor.c      # Dependency-free automated tests
├── Makefile
├── README.md
└── .gitignore
```

## Build and run (Windows PowerShell)

Install GCC and GNU Make (for example, MinGW-w64), then open PowerShell in the
project directory.

Build the application:

```powershell
mingw32-make all
```

Run it:

```powershell
.\sensor_monitor_project.exe
```

The application writes `sensors.csv` to its current working directory. To
compile directly without Make:

```powershell
gcc -std=c11 -Wall -Wextra -Wpedantic main.c sensor.c storage.c -o sensor_monitor_project.exe -lm
```

## Example output

```text
=== Initial Sensors ===
Sensor ID: 101 | Temperature: 25.00 | Status: NORMAL | Flags: 0
Sensor ID: 102 | Temperature: 40.00 | Status: WARNING | Flags: 1
Sensor ID: 103 | Temperature: 60.00 | Status: CRITICAL | Flags: 2

Found sensor: 102

Temperature updated successfully!

=== After First Update ===
Sensor ID: 102 | Temperature: 45.50 | Status: WARNING | Flags: 1
...
Sensors saved successfully!
Sensors loaded successfully!
```

The full output includes the remaining sensors and statistics after each
temperature update.

## CSV format

The file has one header row followed by one sensor per row:

```csv
id,temperature,status,flags
101,25.00,0,0
102,45.50,1,1
103,60.00,2,2
```

Statuses are encoded as `0` (`NORMAL`), `1` (`WARNING`), or `2` (`CRITICAL`).
Flags are an unsigned 8-bit value; bit 0 marks a warning and bit 1 marks a
critical reading. When loading, status and flags are recalculated from each
temperature, matching the original program's behavior.

## Testing

Run the automated tests:

```powershell
mingw32-make test
```

The tests cover sensor lookup, updates and exact thresholds, bitwise flags,
statistics, CSV saving/loading and validation, missing files, and dynamic
array resizing.

Try AddressSanitizer and UndefinedBehaviorSanitizer when supported by the
installed GCC build:

```powershell
mingw32-make sanitize
```

Some Windows GCC distributions do not include sanitizer runtime support. If
the sanitizer target fails because the compiler or runtime does not support
these options, the regular build and test targets can still be used.

## What I learned

This project demonstrates how to organize a C program across header and source
files, manage dynamically allocated arrays, validate external CSV input, use
bitwise flags, and test boundary conditions. It also highlights the
importance of clear ownership rules for allocated memory and checking file
and allocation errors.
