#include "industrial_system.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *label;
    const char *command;
} process_entry_t;

static void launch_support_processes(void) {
    process_entry_t processes[] = {
        {"sensor simulator", "./sensor_sim &"},
        {"conveyor controller", "./conveyer_controller &"},
        {"robot controller", "./robot_controller &"},
        {"HMI client", "./hmi_client &"},
        {"logger", "./logger &"},
    };
    size_t count = sizeof(processes) / sizeof(processes[0]);

    for (size_t index = 0; index < count; ++index) {
        int status = system(processes[index].command);
        if (status != 0) {
            fprintf(stderr, "[Master] Unable to launch %s.\n", processes[index].label);
        }
    }
}

int main(void) {
    int fd = -1;
    plant_state_t *state = NULL;
    sem_t *state_lock = NULL;
    sem_t *event_lock = NULL;
    sem_t *part_ready = NULL;

    if (ensure_runtime_dir() != 0) {
        return EXIT_FAILURE;
    }

    reset_runtime_files();
    unlink_resources();

    if (open_shared_state(1, &state, &fd) != 0) {
        return EXIT_FAILURE;
    }

    state_lock = open_named_semaphore(SEM_STATE_LOCK, 1, 1);
    event_lock = open_named_semaphore(SEM_EVENT_LOCK, 1, 1);
    part_ready = open_named_semaphore(SEM_PART_READY, 0, 1);
    if (state_lock == SEM_FAILED || event_lock == SEM_FAILED || part_ready == SEM_FAILED) {
        perror("sem_open");
        close_shared_state(state, fd);
        unlink_resources();
        return EXIT_FAILURE;
    }

    sem_wait(state_lock);
    initialize_state(state);
    state->system_running = 1;
    state->batch_target = 12;
    safe_copy(state->mode, sizeof(state->mode), "AUTO");
    safe_copy(state->last_event, sizeof(state->last_event), "Master boot sequence completed");
    state->last_update = time(NULL);
    write_snapshot_locked(state);
    sem_post(state_lock);

    append_event(event_lock, "MASTER", "Controller initialized and runtime files created.");
    append_event(event_lock, "MASTER", "Launching plant processes.");
    launch_support_processes();

    printf("=== Industrial Automation System ===\n");
    printf("[Master] Target batch size: %d units\n", state->batch_target);

    while (1) {
        int done = 0;
        int target = 0;
        int active_alarm = 0;
        int accepted = 0;
        int rejected = 0;
        int maintenance_risk = 0;

        sem_wait(state_lock);
        done = state->parts_processed;
        target = state->batch_target;
        active_alarm = state->alarm_active;
        accepted = state->parts_accepted;
        rejected = state->parts_rejected;
        maintenance_risk = state->maintenance_risk;
        state->uptime_seconds = (long)(time(NULL) - state->start_time);
        write_snapshot_locked(state);
        sem_post(state_lock);

        printf("[Master] Progress %02d/%02d | Accepted: %02d | Rejected: %02d | Risk: %d%%\n",
               done, target, accepted, rejected, maintenance_risk);

        if (active_alarm) {
            append_event(event_lock, "MASTER", "Alarm acknowledged. Plant remains in supervised auto mode.");
        }

        if (done >= target) {
            break;
        }

        sleep_ms(1500);
    }

    sem_wait(state_lock);
    state->system_running = 0;
    safe_copy(state->mode, sizeof(state->mode), "COMPLETE");
    safe_copy(state->last_event, sizeof(state->last_event), "Batch target achieved");
    state->uptime_seconds = (long)(time(NULL) - state->start_time);
    state->last_update = time(NULL);
    write_snapshot_locked(state);
    sem_post(state_lock);

    append_event(event_lock, "MASTER", "Production target reached. Plant shutdown initiated.");
    sem_post(part_ready);
    sleep_ms(500);

    close_named_semaphore(state_lock);
    close_named_semaphore(event_lock);
    close_named_semaphore(part_ready);
    close_shared_state(state, fd);
    unlink_resources();

    printf("[Master] Plant simulation complete. Review runtime files or launch dashboard.py.\n");
    return EXIT_SUCCESS;
}
