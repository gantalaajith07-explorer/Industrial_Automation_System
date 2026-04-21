#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#ifndef _QNX_SOURCE
#define _QNX_SOURCE
#endif

#ifndef INDUSTRIAL_SYSTEM_H
#define INDUSTRIAL_SYSTEM_H

#include <fcntl.h>
#include <semaphore.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#ifdef _WIN32
#include <direct.h>
#endif

#define SHM_NAME "/ias_plant_state"
#define SEM_STATE_LOCK "/ias_state_lock"
#define SEM_EVENT_LOCK "/ias_event_lock"
#define SEM_PART_READY "/ias_part_ready"

#define RUNTIME_DIR "runtime"
#define SNAPSHOT_FILE "runtime/plant_snapshot.json"
#define EVENTS_FILE "runtime/events.csv"

typedef struct {
    int system_running;
    int batch_target;
    int parts_loaded;
    int parts_processed;
    int parts_accepted;
    int parts_rejected;
    int conveyor_busy;
    int robot_busy;
    int part_at_robot;
    int alarm_active;
    int last_alarm_code;
    int maintenance_risk;
    double temperature_c;
    double vibration_mm_s;
    double conveyor_speed_mps;
    long uptime_seconds;
    time_t start_time;
    time_t last_update;
    char mode[16];
    char last_event[128];
} plant_state_t;

int ensure_runtime_dir(void);
int reset_runtime_files(void);
void unlink_resources(void);
int open_shared_state(int create, plant_state_t **state, int *fd);
void close_shared_state(plant_state_t *state, int fd);
sem_t *open_named_semaphore(const char *name, unsigned int initial_value, int create);
void close_named_semaphore(sem_t *sem);
void initialize_state(plant_state_t *state);
void safe_copy(char *dest, size_t dest_size, const char *src);
void current_timestamp(char *buffer, size_t size);
void sleep_ms(int milliseconds);
int write_snapshot_locked(const plant_state_t *state);
int append_event(sem_t *event_lock, const char *component, const char *message);
void json_escape(const char *src, char *dest, size_t dest_size);
void csv_escape(const char *src, char *dest, size_t dest_size);

#endif
