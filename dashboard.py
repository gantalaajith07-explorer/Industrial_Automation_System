import csv
import json
from pathlib import Path
import tkinter as tk
from tkinter import ttk


BASE_DIR = Path(__file__).resolve().parent
SNAPSHOT_PATH = BASE_DIR / "runtime" / "plant_snapshot.json"
EVENTS_PATH = BASE_DIR / "runtime" / "events.csv"


class Dashboard(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Industrial Automation Dashboard")
        self.geometry("1180x760")
        self.minsize(1024, 680)
        self.configure(bg="#d7d3c8")

        self.status_value = tk.StringVar(value="Waiting for runtime data")
        self.mode_value = tk.StringVar(value="BOOT")
        self.kpi_processed = tk.StringVar(value="0 / 0")
        self.kpi_quality = tk.StringVar(value="0.0%")
        self.kpi_alarm = tk.StringVar(value="Normal")
        self.kpi_risk = tk.StringVar(value="0%")
        self.footer_value = tk.StringVar(value="Launch master_controller to generate telemetry.")
        self.machine_state = {}

        self.gauges = {}

        self._configure_style()
        self._build_layout()
        self.refresh()

    def _configure_style(self):
        style = ttk.Style()
        style.theme_use("clam")
        style.configure("Card.TFrame", background="#f7f4eb", relief="flat")
        style.configure("Panel.TFrame", background="#1f2a2f")
        style.configure("Header.TLabel", background="#1f2a2f", foreground="#f4efe2",
                        font=("Georgia", 24, "bold"))
        style.configure("SubHeader.TLabel", background="#1f2a2f", foreground="#c4cdbd",
                        font=("Georgia", 11))
        style.configure("CardTitle.TLabel", background="#f7f4eb", foreground="#324047",
                        font=("Georgia", 11, "bold"))
        style.configure("CardValue.TLabel", background="#f7f4eb", foreground="#11181c",
                        font=("Georgia", 22, "bold"))
        style.configure("Body.TLabel", background="#f7f4eb", foreground="#213038",
                        font=("Georgia", 11))
        style.configure("Footer.TLabel", background="#d7d3c8", foreground="#2f383d",
                        font=("Georgia", 10))
        style.configure("Treeview", font=("Consolas", 10), rowheight=28,
                        background="#fcfbf7", fieldbackground="#fcfbf7", foreground="#162024")
        style.configure("Treeview.Heading", font=("Georgia", 10, "bold"),
                        background="#b49f6b", foreground="#101518")

    def _build_layout(self):
        header = ttk.Frame(self, style="Panel.TFrame", padding=(24, 20))
        header.pack(fill="x")

        ttk.Label(header, text="Industrial Automation System", style="Header.TLabel").pack(anchor="w")
        ttk.Label(
            header,
            text="Classic supervisory view for conveyor, robot, quality, and plant telemetry.",
            style="SubHeader.TLabel",
        ).pack(anchor="w", pady=(4, 0))

        hero = ttk.Frame(self, padding=(24, 20), style="Card.TFrame")
        hero.pack(fill="x", padx=24, pady=(24, 14))

        left = ttk.Frame(hero, style="Card.TFrame")
        left.pack(side="left", fill="both", expand=True)
        ttk.Label(left, text="Plant Status", style="CardTitle.TLabel").pack(anchor="w")
        ttk.Label(left, textvariable=self.status_value, style="CardValue.TLabel").pack(anchor="w", pady=(6, 0))
        ttk.Label(left, textvariable=self.footer_value, style="Body.TLabel", wraplength=640).pack(anchor="w", pady=(10, 0))

        right = ttk.Frame(hero, style="Card.TFrame")
        right.pack(side="right", padx=(12, 0))
        ttk.Label(right, text="Mode", style="CardTitle.TLabel").pack(anchor="e")
        ttk.Label(right, textvariable=self.mode_value, style="CardValue.TLabel").pack(anchor="e")

        cards = ttk.Frame(self, padding=(24, 0))
        cards.pack(fill="x")
        self._build_kpi_card(cards, "Throughput", self.kpi_processed).pack(side="left", fill="x", expand=True, padx=(0, 8))
        self._build_kpi_card(cards, "Quality Yield", self.kpi_quality).pack(side="left", fill="x", expand=True, padx=8)
        self._build_kpi_card(cards, "Maintenance Risk", self.kpi_risk).pack(side="left", fill="x", expand=True, padx=8)
        self._build_kpi_card(cards, "Alarm State", self.kpi_alarm).pack(side="left", fill="x", expand=True, padx=(8, 0))

        middle = ttk.Frame(self, padding=(24, 18))
        middle.pack(fill="both", expand=True)

        left_panel = ttk.Frame(middle, style="Card.TFrame", padding=18)
        left_panel.pack(side="left", fill="both", expand=True, padx=(0, 10))
        ttk.Label(left_panel, text="Live Process Metrics", style="CardTitle.TLabel").pack(anchor="w")

        gauges_container = ttk.Frame(left_panel, style="Card.TFrame")
        gauges_container.pack(fill="both", expand=True, pady=(16, 0))
        self._build_gauge(gauges_container, "Temperature (C)", "#b7563c", 100)
        self._build_gauge(gauges_container, "Vibration (mm/s)", "#6b4f2a", 5)
        self._build_gauge(gauges_container, "Maintenance Risk (%)", "#2f5f75", 100)
        self._build_gauge(gauges_container, "Conveyor Speed (m/s)", "#4b7f52", 2)

        right_panel = ttk.Frame(middle, style="Card.TFrame", padding=18)
        right_panel.pack(side="left", fill="both", expand=True)
        ttk.Label(right_panel, text="Machine Overview", style="CardTitle.TLabel").pack(anchor="w")
        machine_bar = ttk.Frame(right_panel, style="Card.TFrame")
        machine_bar.pack(fill="x", pady=(14, 18))
        self._build_machine_tile(machine_bar, "Conveyor").pack(side="left", fill="x", expand=True, padx=(0, 8))
        self._build_machine_tile(machine_bar, "Robot").pack(side="left", fill="x", expand=True, padx=8)
        self._build_machine_tile(machine_bar, "Alarm").pack(side="left", fill="x", expand=True, padx=(8, 0))

        ttk.Label(right_panel, text="Recent Event Feed", style="CardTitle.TLabel").pack(anchor="w")

        columns = ("timestamp", "component", "message")
        tree = ttk.Treeview(right_panel, columns=columns, show="headings", height=15)
        tree.heading("timestamp", text="Timestamp")
        tree.heading("component", text="Component")
        tree.heading("message", text="Event")
        tree.column("timestamp", width=170, anchor="w")
        tree.column("component", width=110, anchor="center")
        tree.column("message", width=430, anchor="w")
        tree.pack(fill="both", expand=True, pady=(16, 0))
        self.event_tree = tree

        footer = ttk.Frame(self, padding=(24, 0, 24, 16))
        footer.pack(fill="x")
        ttk.Label(
            footer,
            text="Dashboard refreshes automatically from runtime/plant_snapshot.json and runtime/events.csv",
            style="Footer.TLabel",
        ).pack(anchor="w")

    def _build_kpi_card(self, parent, title, variable):
        frame = ttk.Frame(parent, style="Card.TFrame", padding=16)
        ttk.Label(frame, text=title, style="CardTitle.TLabel").pack(anchor="w")
        ttk.Label(frame, textvariable=variable, style="CardValue.TLabel").pack(anchor="w", pady=(8, 0))
        return frame

    def _build_gauge(self, parent, title, color, upper_bound):
        section = ttk.Frame(parent, style="Card.TFrame")
        section.pack(fill="x", pady=7)

        title_row = ttk.Frame(section, style="Card.TFrame")
        title_row.pack(fill="x")
        ttk.Label(title_row, text=title, style="Body.TLabel").pack(side="left")
        value_var = tk.StringVar(value="0.00")
        ttk.Label(title_row, textvariable=value_var, style="Body.TLabel").pack(side="right")

        canvas = tk.Canvas(section, height=24, bg="#efe7d7", highlightthickness=0)
        canvas.pack(fill="x", pady=(6, 0))
        fill = canvas.create_rectangle(0, 0, 0, 24, fill=color, width=0)
        border = canvas.create_rectangle(1, 1, 699, 23, outline="#796b58", width=1)
        label = canvas.create_text(12, 12, text="", anchor="w", fill="#f6f2e8", font=("Consolas", 10, "bold"))

        self.gauges[title] = {
            "canvas": canvas,
            "fill": fill,
            "border": border,
            "label": label,
            "value": value_var,
            "upper": upper_bound,
        }

    def _build_machine_tile(self, parent, title):
        frame = ttk.Frame(parent, style="Card.TFrame", padding=14)
        ttk.Label(frame, text=title, style="CardTitle.TLabel").pack(anchor="center")
        value = tk.StringVar(value="Standby")
        label = tk.Label(
            frame,
            textvariable=value,
            bg="#d9d1bf",
            fg="#172126",
            font=("Georgia", 12, "bold"),
            padx=10,
            pady=10,
        )
        label.pack(fill="x", pady=(10, 0))
        self.machine_state[title] = {"text": value, "label": label}
        return frame

    def refresh(self):
        snapshot = self._read_snapshot()
        events = self._read_events()

        self.mode_value.set(snapshot.get("mode", "BOOT"))
        running = bool(snapshot.get("system_running", 0))
        processed = int(snapshot.get("parts_processed", 0))
        target = int(snapshot.get("batch_target", 0))
        accepted = int(snapshot.get("parts_accepted", 0))
        rejected = int(snapshot.get("parts_rejected", 0))
        alarm = bool(snapshot.get("alarm_active", 0))
        uptime = int(snapshot.get("uptime_seconds", 0))

        yield_value = (accepted / processed * 100.0) if processed else 0.0
        self.status_value.set("Production Active" if running else "Idle / Completed")
        self.kpi_processed.set(f"{processed} / {target}")
        self.kpi_quality.set(f"{yield_value:.1f}%")
        self.kpi_risk.set(f"{int(snapshot.get('maintenance_risk', 0))}%")
        self.kpi_alarm.set("Alarm Active" if alarm else "Normal")
        self.footer_value.set(snapshot.get("last_event", "Waiting for the next update."))

        status_tail = f"Uptime: {uptime}s | Accepted: {accepted} | Rejected: {rejected}"
        self.status_value.set(f"{self.status_value.get()}    {status_tail}")

        self._set_gauge("Temperature (C)", snapshot.get("temperature_c", 0.0), "Thermal")
        self._set_gauge("Vibration (mm/s)", snapshot.get("vibration_mm_s", 0.0), "Motion")
        self._set_gauge("Maintenance Risk (%)", snapshot.get("maintenance_risk", 0.0), "Risk")
        self._set_gauge("Conveyor Speed (m/s)", snapshot.get("conveyor_speed_mps", 0.0), "Flow")
        self._set_machine_tile("Conveyor", "Running" if snapshot.get("conveyor_busy", 0) else "Ready",
                               "#a97d4e" if snapshot.get("conveyor_busy", 0) else "#d9d1bf")
        self._set_machine_tile("Robot", "Processing" if snapshot.get("robot_busy", 0) else "Idle",
                               "#5b7f62" if snapshot.get("robot_busy", 0) else "#d9d1bf")
        self._set_machine_tile("Alarm", f"Code {snapshot.get('last_alarm_code', 0)}" if alarm else "Clear",
                               "#9f4b3f" if alarm else "#d9d1bf")
        self._populate_events(events)

        self.after(1000, self.refresh)

    def _set_gauge(self, title, value, prefix):
        gauge = self.gauges[title]
        canvas = gauge["canvas"]
        canvas.update_idletasks()
        width = max(canvas.winfo_width() - 2, 10)
        upper = gauge["upper"] or 1
        ratio = max(0.0, min(float(value) / float(upper), 1.0))
        fill_width = int(width * ratio)
        canvas.coords(gauge["fill"], 1, 1, fill_width, 23)
        canvas.coords(gauge["border"], 1, 1, width, 23)
        canvas.coords(gauge["label"], 12, 12)
        canvas.itemconfigure(gauge["label"], text=f"{prefix} {value:.2f}")
        gauge["value"].set(f"{value:.2f}")

    def _set_machine_tile(self, title, text, color):
        tile = self.machine_state[title]
        tile["text"].set(text)
        tile["label"].configure(bg=color, fg="#f7f3eb" if color != "#d9d1bf" else "#172126")

    def _populate_events(self, events):
        existing = self.event_tree.get_children()
        for item in existing:
            self.event_tree.delete(item)

        for row in events[-12:]:
            self.event_tree.insert("", "end", values=(row["timestamp"], row["component"], row["message"]))

    def _read_snapshot(self):
        if not SNAPSHOT_PATH.exists():
            return {}

        try:
            with SNAPSHOT_PATH.open("r", encoding="utf-8") as handle:
                return json.load(handle)
        except (OSError, json.JSONDecodeError):
            return {}

    def _read_events(self):
        if not EVENTS_PATH.exists():
            return []

        try:
            with EVENTS_PATH.open("r", encoding="utf-8") as handle:
                rows = list(csv.DictReader(handle))
        except OSError:
            return []

        return rows


if __name__ == "__main__":
    Dashboard().mainloop()
