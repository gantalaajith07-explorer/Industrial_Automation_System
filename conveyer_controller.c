#include "industrial_system.h"

#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int fd = -1;
    plant_state_t *state = NULL;
    sem_t *state_lock = NULL;
    sem_t *event_lock = NULL;
    sem_t *part_ready = NULL;

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

    append_event(event_lock, "CONVEYOR", "Conveyor controller online.");

    while (1) {
        int should_process = 0;
        int cycle_number = 0;

        sem_wait(state_lock);
        if (!state->system_running) {
            sem_post(state_lock);
            break;
        }

        if (strcmp(state->mode, "AUTO") == 0 &&
            !state->conveyor_busy &&
            !state->part_at_robot &&
            state->parts_loaded < state->batch_target) {
            state->conveyor_busy = 1;
            state->parts_loaded += 1;
            cycle_number = state->parts_loaded;
            safe_copy(state->last_event, sizeof(state->last_event), "Conveyor accepted a new workpiece");
            state->last_update = time(NULL);
            write_snapshot_locked(state);
            should_process = 1;
        }
        sem_post(state_lock);

        if (!should_process) {
            sleep_ms(250);
            continue;
        }

        append_event(event_lock, "CONVEYOR", "Transporting workpiece to robot pick station.");
        printf("[Conveyor] Batch item %d entering transfer lane.\n", cycle_number);
        sleep_ms(900);

        sem_wait(state_lock);
        state->conveyor_busy = 0;
        state->part_at_robot = 1;
        safe_copy(state->last_event, sizeof(state->last_event), "Workpiece delivered to robot station");
        state->last_update = time(NULL);
        write_snapshot_locked(state);
        sem_post(state_lock);

        append_event(event_lock, "CONVEYOR", "Workpiece delivered. Robot notified.");
        sem_post(part_ready);
    }

    append_event(event_lock, "CONVEYOR", "Conveyor controller shutting down.");
    close_named_semaphore(state_lock);
    close_named_semaphore(event_lock);
    close_named_semaphore(part_ready);
    close_shared_state(state, fd);
    return EXIT_SUCCESS;
}
