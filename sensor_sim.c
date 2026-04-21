#include "industrial_system.h"

#include <stdio.h>
#include <stdlib.h>

static double bounded_noise(double base, double span) {
    double shift = (double)(rand() % 1000) / 1000.0;
    return base + (shift * span);
}

int main(void) {
    int fd = -1;
    plant_state_t *state = NULL;
    sem_t *state_lock = NULL;
    sem_t *event_lock = NULL;
    int previous_alarm = 0;

    srand((unsigned int)time(NULL));

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

    append_event(event_lock, "SENSOR", "Sensor simulator online.");

    while (1) {
        double temperature = bounded_noise(38.0, 42.0);
        double vibration = bounded_noise(1.2, 3.7);
        double conveyor_speed = bounded_noise(0.75, 0.4);
        int maintenance_risk = 20 + (int)((temperature - 35.0) * 1.2) + (int)(vibration * 10.0);
        int alarm = 0;
        int alarm_code = 0;

        if (maintenance_risk < 0) {
            maintenance_risk = 0;
        }
        if (maintenance_risk > 99) {
            maintenance_risk = 99;
        }

        if (temperature > 76.0) {
            alarm = 1;
            alarm_code = 1101;
        } else if (vibration > 4.2) {
            alarm = 1;
            alarm_code = 1202;
        }

        sem_wait(state_lock);
        if (!state->system_running) {
            sem_post(state_lock);
            break;
        }

        state->temperature_c = temperature;
        state->vibration_mm_s = vibration;
        state->conveyor_speed_mps = conveyor_speed;
        state->maintenance_risk = maintenance_risk;
        state->alarm_active = alarm ? 1 : state->alarm_active;
        state->last_alarm_code = alarm ? alarm_code : state->last_alarm_code;
        state->uptime_seconds = (long)(time(NULL) - state->start_time);
        safe_copy(state->last_event, sizeof(state->last_event),
                  alarm ? "Sensor layer raised an alarm condition" : "Sensor telemetry refreshed");
        state->last_update = time(NULL);
        write_snapshot_locked(state);
        sem_post(state_lock);

        if (alarm && !previous_alarm) {
            append_event(event_lock, "SENSOR", "Alarm raised from thermal or vibration threshold.");
        } else if (!alarm && previous_alarm) {
            append_event(event_lock, "SENSOR", "Alarm condition cleared after telemetry stabilized.");
            sem_wait(state_lock);
            state->alarm_active = 0;
            state->last_alarm_code = 0;
            write_snapshot_locked(state);
            sem_post(state_lock);
        }

        previous_alarm = alarm;
        printf("[Sensor] Temp %.1f C | Vibration %.2f mm/s | Conveyor %.2f m/s\n",
               temperature, vibration, conveyor_speed);
        sleep_ms(1000);
    }

    append_event(event_lock, "SENSOR", "Sensor simulator shutting down.");
    close_named_semaphore(state_lock);
    close_named_semaphore(event_lock);
    close_shared_state(state, fd);
    return EXIT_SUCCESS;
}
