#include "industrial_system.h"

#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int fd = -1;
    plant_state_t *state = NULL;
    sem_t *state_lock = NULL;
    sem_t *event_lock = NULL;
    int previous_processed = -1;

    if (open_shared_state(0, &state, &fd) != 0) {
        return EXIT_FAILURE;
    }

    state_lock = open_named_semaphore(SEM_STATE_LOCK, 1, 0);
    event_lock = open_named_semaphore(SEM_EVENT_LOCK, 1, 0);
    if (state_lock == SEM_FAILED || event_lock == SEM_FAILED) {
        perror("sem_open");
        close_shared_state(state, fd);
        return EXIT_FAILURE;
    }

    append_event(event_lock, "LOGGER", "Logger process online.");

    while (1) {
        int running = 0;
        int processed = 0;
        int accepted = 0;
        int rejected = 0;
        int maintenance_risk = 0;

        sem_wait(state_lock);
        running = state->system_running;
        processed = state->parts_processed;
        accepted = state->parts_accepted;
        rejected = state->parts_rejected;
        maintenance_risk = state->maintenance_risk;
        sem_post(state_lock);

        if (processed != previous_processed) {
            char message[160];
            snprintf(message, sizeof(message),
                     "KPI update - processed:%d accepted:%d rejected:%d risk:%d%%",
                     processed, accepted, rejected, maintenance_risk);
            append_event(event_lock, "LOGGER", message);
            previous_processed = processed;
        }

        if (!running) {
            break;
        }

        sleep_ms(1000);
    }

    append_event(event_lock, "LOGGER", "Logger process shutting down.");
    close_named_semaphore(state_lock);
    close_named_semaphore(event_lock);
    close_shared_state(state, fd);
    return EXIT_SUCCESS;
}
