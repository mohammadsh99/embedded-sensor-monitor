CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic
LDLIBS = -lm

APP = sensor_monitor_project.exe
TEST = tests/test_sensor.exe
SANITIZER_TEST = tests/test_sensor_sanitize.exe

.PHONY: all test sanitize clean

all: $(APP)

$(APP): main.c sensor.c sensor.h storage.c storage.h
	$(CC) $(CFLAGS) main.c sensor.c storage.c -o $(APP) $(LDLIBS)

$(TEST): tests/test_sensor.c sensor.c sensor.h storage.c storage.h
	$(CC) $(CFLAGS) tests/test_sensor.c sensor.c storage.c -o $(TEST) $(LDLIBS)

test: $(TEST)
	./$(TEST)

sanitize:
	$(CC) $(CFLAGS) -g -O1 -fsanitize=address,undefined \
		-fno-omit-frame-pointer tests/test_sensor.c sensor.c storage.c \
		-o $(SANITIZER_TEST) $(LDLIBS)
	./$(SANITIZER_TEST)

clean:
	powershell -NoProfile -Command "Remove-Item -Force -ErrorAction SilentlyContinue -LiteralPath '$(APP)', '$(TEST)', '$(SANITIZER_TEST)'; Get-ChildItem -File -Filter '*.o' | Remove-Item -Force; Get-ChildItem -LiteralPath 'tests' -File -Filter '*.o' | Remove-Item -Force"
