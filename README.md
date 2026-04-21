# Industrial Automation System

Industrial Automation System is a QNX-based minor project designed to simulate a compact but realistic industrial production environment using a multi-process architecture. Instead of building a single monolithic program, this project separates plant behavior into multiple coordinated subsystems such as the master controller, conveyor controller, robot controller, sensor simulator, HMI client, and logger. This approach reflects how industrial automation systems are commonly structured in practice, where different functional units operate independently but communicate through reliable inter-process coordination mechanisms.

The core objective of this project is to demonstrate process synchronization, plant monitoring, basic quality control logic, alarm handling, and supervisory visualization in a way that is academically strong and also professional enough for GitHub presentation. The QNX side of the project focuses on POSIX shared memory and named semaphores to exchange live plant state across processes, while the Python side provides a supervisory dashboard that presents throughput, quality yield, maintenance risk, event activity, and machine status in a clean and classic control-room style interface.

This project was improved from a simple collection of independent controller examples into a more structured industrial simulation. The master controller initializes the production cycle, establishes the shared runtime state, supervises plant progress, and coordinates completion. The conveyor controller simulates workpiece transfer, the robot controller performs pick-and-place style handling with inspection logic, the sensor simulator generates live telemetry such as temperature, vibration, and conveyor speed, the HMI client provides operator-style textual monitoring, and the logger records important runtime activity for post-run review.

One of the major strengths of this project is the combination of embedded systems concepts with a presentation layer that makes the output easy to understand. While the QNX simulation demonstrates system-level concepts such as concurrency, IPC, and process orchestration, the Python dashboard makes the project visually appealing and easier to explain during demonstrations, viva sessions, or GitHub portfolio reviews. This makes the project suitable not only as a minor project submission but also as a strong showcase of systems programming, industrial automation modeling, and monitoring interface design.

## Key Features

- Multi-process industrial automation simulation on QNX.
- POSIX shared memory and semaphore-based IPC architecture.
- Separate modules for master control, conveyor, robot, sensors, HMI, and logging.
- Simulated production workflow with throughput, accepted/rejected parts, and batch monitoring.
- Live telemetry generation including temperature, vibration, conveyor speed, and maintenance risk.
- Alarm handling and quality inspection logic for a more realistic plant model.
- Python dashboard for supervisory monitoring and demonstration.
- Demo data generator for presenting the dashboard even without a live QNX runtime connection.

## Project Modules

### Master Controller
The master controller is responsible for initializing the plant state, creating shared runtime resources, supervising production progress, and handling the overall lifecycle of the simulation. It acts as the supervisory brain of the system and ensures the production cycle is tracked from startup to completion.

### Conveyor Controller
The conveyor controller simulates the movement of workpieces through the plant. It updates the system state when a product is loaded and when it reaches the robot station, allowing the rest of the plant logic to operate in a coordinated sequence.

### Robot Controller
The robot controller simulates workpiece pickup, handling, and inspection. It is responsible for marking workpieces as accepted or rejected based on runtime conditions, which introduces practical quality-control behavior into the project.

### Sensor Simulator
The sensor simulator continuously generates plant telemetry values such as temperature, vibration, and conveyor speed. These values are used to update maintenance risk and raise alarm conditions when thresholds are exceeded.

### HMI Client
The HMI client provides a textual operator-side system status view, showing real-time information about plant condition, progress, and alarms in a console-friendly format.

### Logger
The logger records important plant events and KPI changes into runtime logs so that the project also demonstrates event tracking and plant history collection.

### Python Dashboard
The Python dashboard reads runtime telemetry and event logs and presents them in a classic industrial supervisory interface. It displays throughput, quality yield, machine state, alarm condition, and recent plant activity in a dashboard layout suitable for screenshots, presentations, and GitHub showcases.

## Technologies Used

- QNX Momentics IDE
- C programming language
- POSIX shared memory
- POSIX named semaphores
- Python
- Tkinter
- JSON and CSV runtime logging

## How to Run

### QNX Simulation
Build the project in QNX Momentics and run the binaries. Start `master_controller` first, and then launch the remaining supporting processes manually if required by the IDE runtime behavior.

### Python Dashboard
To run the dashboard locally on Windows:
1. Start the demo data generator:
   `python demo_data_generator.py`
2. Open the dashboard in another terminal:
   `python dashboard.py`

This setup allows the dashboard to display continuously changing telemetry even when the QNX runtime is not directly writing to the local Windows filesystem.

## GitHub Showcase Value

This project is valuable as a GitHub portfolio project because it combines operating-system level programming, industrial automation concepts, inter-process communication, telemetry handling, and a user-facing visualization layer. It shows the ability to work across low-level systems programming and higher-level application design, which makes it more complete than a standard lab exercise or basic controller demo.

## Future Improvements

- Add network-based telemetry streaming between QNX target and dashboard host.
- Add predictive maintenance analytics based on telemetry patterns.
- Extend the project to support multiple robot/conveyor stations.
- Add database-backed event storage for longer-term plant history.
- Introduce configurable operating modes such as manual, automatic, and maintenance.
