CC ?= qcc
CFLAGS ?= -Wall -Wextra -std=c11
LDFLAGS ?=

ifeq ($(findstring qcc,$(CC)),qcc)
THREAD_FLAGS =
RT_FLAGS =
else
THREAD_FLAGS = -pthread
RT_FLAGS = -lrt
endif

COMMON = industrial_system.o
TARGETS = master_controller robot_controller conveyer_controller sensor_sim hmi_client logger

all: $(TARGETS)

industrial_system.o: industrial_system.c industrial_system.h
	$(CC) $(CFLAGS) -c industrial_system.c

master_controller: master_controller.c $(COMMON)
	$(CC) $(CFLAGS) -o $@ master_controller.c $(COMMON) $(LDFLAGS) $(RT_FLAGS) $(THREAD_FLAGS)

robot_controller: robot_controller.c $(COMMON)
	$(CC) $(CFLAGS) -o $@ robot_controller.c $(COMMON) $(LDFLAGS) $(RT_FLAGS) $(THREAD_FLAGS)

conveyer_controller: conveyer_controller.c $(COMMON)
	$(CC) $(CFLAGS) -o $@ conveyer_controller.c $(COMMON) $(LDFLAGS) $(RT_FLAGS) $(THREAD_FLAGS)

sensor_sim: sensor_sim.c $(COMMON)
	$(CC) $(CFLAGS) -o $@ sensor_sim.c $(COMMON) $(LDFLAGS) $(RT_FLAGS) $(THREAD_FLAGS)

hmi_client: hmi_client.c $(COMMON)
	$(CC) $(CFLAGS) -o $@ hmi_client.c $(COMMON) $(LDFLAGS) $(RT_FLAGS) $(THREAD_FLAGS)

logger: logger.c $(COMMON)
	$(CC) $(CFLAGS) -o $@ logger.c $(COMMON) $(LDFLAGS) $(RT_FLAGS) $(THREAD_FLAGS)

clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean
