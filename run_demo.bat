@echo off
cd /d "%~dp0"
start "" python dashboard.py
python demo_data_generator.py
