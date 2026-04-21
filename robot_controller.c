#include "industrial_system.h"

#include <stdio.h>
#include <stdlib.h>

static int decide_quality(double temperature, double vibration) {
    int thermal_penalty = temperature > 70.0 ? 20 : 0;
    int vibration_penalty = vibration > 3.4 ? 15 : 0;
    int random_penalty = rand() % 20;
    return (thermal_penalty + vibration_penalty + random_penalty) < 25;
}

int main(void) {
    int fd = -1;
    plant_state_t *state = NULL;
    sem_t *state_lock = NULL;
    sem_t *event_lock = NULL;
    sem_t *part_ready = NULL;

    srand((unsigned int)time(NULL));

    if (open_shared_state(0, &state, &fd) != 0) {
        return EXIT_FAILURE;
    }

    state_lock = open_named_semaphore(SEM_STATE_LOCK, 1, 0);
    event_lock = open_named_semaphore(SEM_EVENT_LOCK, 1, 0);
    part_ready = open_named_semaphore(SEM_PART_READY, 0, 0);
    if (state_lock == SEM_FAILED || event_lock == SEM_FAILED || part_ready == SEM_FAILED) {
        perror("sem_open");
        close_shared_state(state, fd);
        return EXIT_FAILURE;
    }

    append_event(event_lock, "ROBOT", "Robot controller online.");

    while (1) {
        int is_running = 1;
        int has_part = 0;
        double temperature = 0.0;
        double vibration = 0.0;

        sem_wait(part_ready);

        sem_wait(state_lock);
        is_running = state->system_running;
        has_part = state->part_at_robot;
        if (has_part && is_running) {
            state->robot_busy = 1;
            temperature = state->temperature_c;
            vibration = state->vibration_mm_s;
            safe_copy(state->last_event, sizeof(state->last_event), "Robot started pick-and-place cycle");
            state->last_update = time(NULL);
            write_snapshot_locked(state);
        }
        sem_post(state_lock);

        if (!is_running) {
            break;
        }

        if (!has_part) {
            sleep_ms(100);
            continue;
        }

        append_event(event_lock, "ROBOT", "Processing part at pick station.");
        printf("[Robot] Picking, placing, and inspecting workpiece.\n");
        sleep_ms(1200);

        sem_wait(state_lock);
        state->robot_busy = 0;
        state->part_at_robot = 0;
        state->parts_processed += 1;

        if (decide_quality(temperature, vibration)) {
            state->parts_accepted += 1;
            safe_copy(state->last_event, sizeof(state->last_event), "Robot passed workpiece quality inspection");
            sem_post(state_lock);
            append_event(event_lock, "ROBOT", "Workpiece accepted after inspection.");
        } else {
            state->parts_rejected += 1;
            state->alarm_active = 1;
            state->last_alarm_code = 3001;
            safe_copy(state->last_event, sizeof(state->last_event), "Robot flagged workpiece as rejected");
            sem_post(state_lock);
            append_event(event_lock, "ROBOT", "Workpiece rejected. Quality deviation recorded.");
        }

        sem_wait(state_lock);
        if (state->alarm_active && state->maintenance_risk < 65) {
            state->alarm_active = 0;
            state->last_alarm_code = 0;
        }
        state->last_update = time(NULL);
        write_snapshot_locked(state);
        sem_post(state_lock);
    }

    append_event(event_lock, "ROBOT", "Robot controller shutting down.");
    close_named_semaphore(state_lock);
    close_named_semaphore(event_lock);
    close_named_semaphore(part_ready);
    close_shared_state(state, fd);
    return EXIT_SUCCESS;
}
