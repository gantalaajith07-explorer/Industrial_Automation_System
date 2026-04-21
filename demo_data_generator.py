import csv
import json
import random
import time
from pathlib import Path


BASE_DIR = Path(__file__).resolve().parent
RUNTIME_DIR = BASE_DIR / "runtime"
SNAPSHOT_PATH = RUNTIME_DIR / "plant_snapshot.json"
EVENTS_PATH = RUNTIME_DIR / "events.csv"

EVENT_POOL = [
    ("MASTER", "Supervisor heartbeat updated."),
    ("CONVEYOR", "Workpiece moved to robot pick station."),
    ("ROBOT", "Pick-and-place cycle completed successfully."),
    ("LOGGER", "KPI frame committed to event history."),
    ("SENSOR", "Telemetry refresh completed."),
    ("HMI", "Operator screen synchronized with plant state."),
]


def ensure_runtime():
    RUNTIME_DIR.mkdir(exist_ok=True)
    with EVENTS_PATH.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(["timestamp", "component", "message"])


def append_event(component, message):
    timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
    with EVENTS_PATH.open("a", encoding="utf-8", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow([timestamp, component, message])


def main():
    ensure_runtime()

    processed = 0
    accepted = 0
    rejected = 0
    loaded = 0
    uptime = 0

    while True:
        if processed >= 12:
            append_event("MASTER", "Batch complete. Restarting demo cycle.")
            processed = 0
            accepted = 0
            rejected = 0
            loaded = 0
            uptime = 0
            time.sleep(1)

        loaded = min(loaded + random.randint(0, 1), 12)

        if processed < loaded and random.random() > 0.25:
            processed += 1
            if random.random() > 0.18:
                accepted += 1
                append_event("ROBOT", "Workpiece accepted after inspection.")
                last_event = "Robot passed workpiece quality inspection"
                alarm_active = 0
                alarm_code = 0
            else:
                rejected += 1
                append_event("ROBOT", "Workpiece rejected. Quality deviation recorded.")
                last_event = "Robot flagged workpiece as rejected"
                alarm_active = 1
                alarm_code = 3001
        else:
            component, message = random.choice(EVENT_POOL)
            append_event(component, message)
            last_event = message
            alarm_active = 0
            alarm_code = 0

        temperature = round(random.uniform(42.0, 79.0), 2)
        vibration = round(random.uniform(1.1, 4.6), 2)
        conveyor_speed = round(random.uniform(0.72, 1.12), 2)
        maintenance_risk = min(99, max(8, int((temperature - 35.0) * 1.1 + vibration * 9)))
        uptime += 1

        snapshot = {
            "system_running": 1,
            "mode": "AUTO",
            "batch_target": 12,
            "parts_loaded": loaded,
            "parts_processed": processed,
            "parts_accepted": accepted,
            "parts_rejected": rejected,
            "conveyor_busy": 1 if random.random() > 0.55 else 0,
            "robot_busy": 1 if random.random() > 0.62 else 0,
            "part_at_robot": 1 if random.random() > 0.7 else 0,
            "alarm_active": alarm_active,
            "last_alarm_code": alarm_code,
            "maintenance_risk": maintenance_risk,
            "temperature_c": temperature,
            "vibration_mm_s": vibration,
            "conveyor_speed_mps": conveyor_speed,
            "uptime_seconds": uptime,
            "last_event": last_event,
            "last_update_epoch": int(time.time()),
        }

        with SNAPSHOT_PATH.open("w", encoding="utf-8") as handle:
            json.dump(snapshot, handle, indent=2)

        time.sleep(1)


if __name__ == "__main__":
    main()
