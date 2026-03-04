CC = gcc
CFLAGS = -Wall -Wextra -I../src -std=c99
CXXFLAGS = -Wall -Wextra -I../src

test_mqtt_v5: test_mqtt_v5.c ../src/YuaMQTT.cpp ../src/YuaMQTT.h
	$(CC) $(CFLAGS) -o test_mqtt_v5 test_mqtt_v5.c ../src/YuaMQTT.cpp -lstdc++

.PHONY: test clean

test: test_mqtt_v5
	./test_mqtt_v5

clean:
	rm -f test_mqtt_v5 test_mqtt_v5.exe
