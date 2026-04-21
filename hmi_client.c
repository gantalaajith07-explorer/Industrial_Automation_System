#include "industrial_system.h"

#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int fd = -1;
    plant_state_t *state = NULL;
    sem_t *state_lock = NULL;

    if (open_shared_state(0, &state, &fd) != 0) {
        return EXIT_FAILURE;
    }

    state_lock = open_named_semaphore(SEM_STATE_LOCK, 1, 0);
    if (state_lock == SEM_FAILED) {
        perror("sem_open");
        close_shared_state(state, fd);
        return EXIT_FAILURE;
    }

    printf("=== HMI CLIENT ===\n");
    while (1) {
        int running = 0;
        int processed = 0;
        int target = 0;
        int accepted = 0;
        int rejected = 0;
        int alarm = 0;
        int maintenance_risk = 0;
        double temperature = 0.0;
        double vibration = 0.0;
        char mode[16];
        char last_event[128];

        sem_wait(state_lock);
        running = state->system_running;
        processed = state->parts_processed;
        target = state->batch_target;
        accepted = state->parts_accepted;
        rejected = state->parts_rejected;
        alarm = state->alarm_active;
        maintenance_risk = state->maintenance_risk;
        temperature = state->temperature_c;
        vibration = state->vibration_mm_s;
        safe_copy(mode, sizeof(mode), state->mode);
        safe_copy(last_event, sizeof(last_event), state->last_event);
        sem_post(state_lock);

        printf("[HMI] Mode:%s | Done:%02d/%02d | OK:%02d | Reject:%02d | Alarm:%s | Risk:%d%%\n",
               mode, processed, target, accepted, rejected, alarm ? "ON" : "OFF", maintenance_risk);
        printf("[HMI] Temp: %.1f C | Vib: %.2f mm/s | Event: %s\n", temperature, vibration, last_event);

        if (!running) {
            break;
        }

        sleep_ms(1800);
    }

    close_named_semaphore(state_lock);
    close_shared_state(state, fd);
    return EXIT_SUCCESS;
}
