#include "industrial_system.h"

#include <errno.h>
#include <stdlib.h>

static int file_exists(const char *path) {
    struct stat info;
    return stat(path, &info) == 0;
}

static void append_char(char **cursor, size_t *remaining, char value) {
    if (*remaining > 1U) {
        **cursor = value;
        (*cursor)++;
        (*remaining)--;
    }
}

int ensure_runtime_dir(void) {
#ifdef _WIN32
    if (_mkdir(RUNTIME_DIR) == 0 || errno == EEXIST) {
        return 0;
    }
#else
    if (mkdir(RUNTIME_DIR, 0777) == 0 || errno == EEXIST) {
        return 0;
    }
#endif
    perror("mkdir");
    return -1;
}

int reset_runtime_files(void) {
    FILE *snapshot = fopen(SNAPSHOT_FILE, "w");
    FILE *events = fopen(EVENTS_FILE, "w");

    if (!snapshot || !events) {
        perror("fopen");
        if (snapshot) {
            fclose(snapshot);
        }
        if (events) {
            fclose(events);
        }
        return -1;
    }

    fprintf(snapshot, "{\n  \"status\": \"booting\"\n}\n");
    fprintf(events, "timestamp,component,message\n");
    fclose(snapshot);
    fclose(events);
    return 0;
}

void unlink_resources(void) {
    shm_unlink(SHM_NAME);
    sem_unlink(SEM_STATE_LOCK);
    sem_unlink(SEM_EVENT_LOCK);
    sem_unlink(SEM_PART_READY);
}

int open_shared_state(int create, plant_state_t **state, int *fd) {
    int flags = create ? (O_CREAT | O_RDWR) : O_RDWR;
    int descriptor = shm_open(SHM_NAME, flags, 0666);

    if (descriptor == -1) {
        perror("shm_open");
        return -1;
    }

    if (create && ftruncate(descriptor, (off_t)sizeof(plant_state_t)) == -1) {
        perror("ftruncate");
        close(descriptor);
        return -1;
    }

    *state = mmap(NULL, sizeof(plant_state_t), PROT_READ | PROT_WRITE, MAP_SHARED, descriptor, 0);
    if (*state == MAP_FAILED) {
        perror("mmap");
        close(descriptor);
        return -1;
    }

    *fd = descriptor;
    return 0;
}

void close_shared_state(plant_state_t *state, int fd) {
    if (state && state != MAP_FAILED) {
        munmap(state, sizeof(plant_state_t));
    }
    if (fd >= 0) {
        close(fd);
    }
}

sem_t *open_named_semaphore(const char *name, unsigned int initial_value, int create) {
    int flags = create ? O_CREAT : 0;
    return sem_open(name, flags, 0666, initial_value);
}

void close_named_semaphore(sem_t *sem) {
    if (sem && sem != SEM_FAILED) {
        sem_close(sem);
    }
}

void initialize_state(plant_state_t *state) {
    memset(state, 0, sizeof(*state));
    state->start_time = time(NULL);
    state->last_update = state->start_time;
    safe_copy(state->mode, sizeof(state->mode), "BOOT");
    safe_copy(state->last_event, sizeof(state->last_event), "State initialized");
}

void safe_copy(char *dest, size_t dest_size, const char *src) {
    if (dest_size == 0) {
        return;
    }

    if (!src) {
        dest[0] = '\0';
        return;
    }

    snprintf(dest, dest_size, "%s", src);
}

void current_timestamp(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *info = localtime(&now);

    if (!info) {
        safe_copy(buffer, size, "unknown-time");
        return;
    }

    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", info);
}

void sleep_ms(int milliseconds) {
    usleep((useconds_t)milliseconds * 1000U);
}

void json_escape(const char *src, char *dest, size_t dest_size) {
    char *cursor = dest;
    size_t remaining = dest_size;

    if (dest_size == 0) {
        return;
    }

    if (!src) {
        dest[0] = '\0';
        return;
    }

    while (*src != '\0' && remaining > 1U) {
        if (*src == '"' || *src == '\\') {
            append_char(&cursor, &remaining, '\\');
            append_char(&cursor, &remaining, *src);
        } else if (*src == '\n' || *src == '\r') {
            append_char(&cursor, &remaining, ' ');
        } else {
            append_char(&cursor, &remaining, *src);
        }
        src++;
    }

    *cursor = '\0';
}

void csv_escape(const char *src, char *dest, size_t dest_size) {
    char *cursor = dest;
    size_t remaining = dest_size;

    if (dest_size == 0) {
        return;
    }

    append_char(&cursor, &remaining, '"');
    if (src) {
        while (*src != '\0' && remaining > 1U) {
            if (*src == '"') {
                append_char(&cursor, &remaining, '"');
                append_char(&cursor, &remaining, '"');
            } else if (*src == '\n' || *src == '\r') {
                append_char(&cursor, &remaining, ' ');
            } else {
                append_char(&cursor, &remaining, *src);
            }
            src++;
        }
    }
    append_char(&cursor, &remaining, '"');
    *cursor = '\0';
}

int write_snapshot_locked(const plant_state_t *state) {
    FILE *snapshot = fopen(SNAPSHOT_FILE, "w");
    char escaped_event[256];

    if (!snapshot) {
        perror("fopen");
        return -1;
    }

    json_escape(state->last_event, escaped_event, sizeof(escaped_event));

    fprintf(snapshot,
            "{\n"
            "  \"system_running\": %d,\n"
            "  \"mode\": \"%s\",\n"
            "  \"batch_target\": %d,\n"
            "  \"parts_loaded\": %d,\n"
            "  \"parts_processed\": %d,\n"
            "  \"parts_accepted\": %d,\n"
            "  \"parts_rejected\": %d,\n"
            "  \"conveyor_busy\": %d,\n"
            "  \"robot_busy\": %d,\n"
            "  \"part_at_robot\": %d,\n"
            "  \"alarm_active\": %d,\n"
            "  \"last_alarm_code\": %d,\n"
            "  \"maintenance_risk\": %d,\n"
            "  \"temperature_c\": %.2f,\n"
            "  \"vibration_mm_s\": %.2f,\n"
            "  \"conveyor_speed_mps\": %.2f,\n"
            "  \"uptime_seconds\": %ld,\n"
            "  \"last_event\": \"%s\",\n"
            "  \"last_update_epoch\": %ld\n"
            "}\n",
            state->system_running,
            state->mode,
            state->batch_target,
            state->parts_loaded,
            state->parts_processed,
            state->parts_accepted,
            state->parts_rejected,
            state->conveyor_busy,
            state->robot_busy,
            state->part_at_robot,
            state->alarm_active,
            state->last_alarm_code,
            state->maintenance_risk,
            state->temperature_c,
            state->vibration_mm_s,
            state->conveyor_speed_mps,
            state->uptime_seconds,
            escaped_event,
            (long)state->last_update);

    fclose(snapshot);
    return 0;
}

int append_event(sem_t *event_lock, const char *component, const char *message) {
    FILE *events = NULL;
    char timestamp[32];
    char safe_component[128];
    char safe_message[256];

    if (event_lock && event_lock != SEM_FAILED) {
        sem_wait(event_lock);
    }

    if (!file_exists(EVENTS_FILE)) {
        events = fopen(EVENTS_FILE, "w");
        if (events) {
            fprintf(events, "timestamp,component,message\n");
            fclose(events);
        }
    }

    events = fopen(EVENTS_FILE, "a");
    if (!events) {
        perror("fopen");
        if (event_lock && event_lock != SEM_FAILED) {
            sem_post(event_lock);
        }
        return -1;
    }

    current_timestamp(timestamp, sizeof(timestamp));
    csv_escape(component, safe_component, sizeof(safe_component));
    csv_escape(message, safe_message, sizeof(safe_message));
    fprintf(events, "%s,%s,%s\n", timestamp, safe_component, safe_message);
    fclose(events);

    if (event_lock && event_lock != SEM_FAILED) {
        sem_post(event_lock);
    }

    return 0;
}
