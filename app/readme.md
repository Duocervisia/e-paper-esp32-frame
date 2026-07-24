# Micropython app

This folder contains the Micropython port of the e-paper frame.

## Configuration

Copy or rename `e_paper_esp32_frame/config_template.py` to
`e_paper_esp32_frame/config.py`, then enter the Wi-Fi and MQTT credentials for
the installation. The real `config.py` is ignored by Git and must not be
committed.

## Layout

- `boot.py`: minimal startup hook
- `main.py`: application entry point
- `e_paper_esp32_frame/config.py`: hardware and runtime configuration
- `e_paper_esp32_frame/internet.py`: WLAN and NTP handling
- `e_paper_esp32_frame/ds.py`: deep sleep handling
- `e_paper_esp32_frame/storage.py`: SD card mounting and image selection
- `e_paper_esp32_frame/time_utils.py`: date and sleep calculations
- `e_paper_esp32_frame/epd.py`: display-facing wrapper around the EPD driver
- `e_paper_esp32_frame/mqtt.py`: optional MQTT logging and broker setup
- `e_paper_esp32_frame/lib/epd/epd.py`: low-level Waveshare display driver port

## Files on the SD card

Copy the following files to the SD card root:

- `setup.json`
- your `.bmp` images
- optional `info.txt` to trigger a rescan when the image set changes

The BMP files should be 800 x 480 px and use the same naming pattern as the Arduino version if you want date-based selection.

## MQTT Logging

Set `CONFIG["mqtt"]["enabled"] = True` in `e_paper_esp32_frame/config.py` and fill in the broker details to forward the application log output to an MQTT topic. The logger still prints locally on the serial console.

The broker keeps a retained ring buffer below
`<topic_prefix>/<device-id>/history/000` through `history/099`. Every JSON
event contains a persistent sequence number and timestamp. The retained
`latest` topic identifies the newest slot; `status` contains the current
device state and `last-error` contains the most recently classified error.
Publications use MQTT QoS 1 and wait for the broker's PUBACK. Before entering
deep sleep, the app flushes partial logs and closes the MQTT connection
cleanly.

## Local Logging

Every complete log line is also appended to `/log.txt` on the ESP32 internal
flash. Lines written before NTP synchronization use an `UNSYNCED` uptime
timestamp. Unhandled exceptions include their traceback. The file is rotated
when it reaches the configured `logging.max_bytes` limit (256 KiB by default).
