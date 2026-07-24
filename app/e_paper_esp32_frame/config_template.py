"""
Copy or rename this file to config.py before installing the app.
Fill in the Wi-Fi and MQTT values for your environment.
"""

APP_NAME = "e-paper-esp32-frame"

CONFIG = {
    "backup_wifi": {
        "ssid": "YOUR_WIFI_SSID",
        "password": "YOUR_WIFI_PASSWORD",
        "static_ip": None,
    },
    "storage": {
        "mount_point": "/sd",
        "setup_file": "/sd/setup.json",
        "info_file": "/sd/info.txt",
        "state_file": "/sd/state.json",
        "image_root": "/sd",
    },
    "display": {
        "power_pin": 26,
        "rst": 3,
        "dc": 17,
        "cs": 16,
        "busy": 25,
        "sck": 21,
        "mosi": 22,
        "spi_id": 1,
        "baudrate": 2_000_000,
        "width": 800,
        "height": 480,
        "panel": "spectra6",
    },
    "battery": {
        "enabled": True,
        "pin": 34,
        "samples": 10,
        "critical": 0.1,
        "vref": 3.578,
        "divider_ratio": 2.0,
    },
    "sd": {
        "cs": 22,
        "sck": 18,
        "mosi": 23,
        "miso": 19,
        "spi_id": 2,
        "baudrate": 10_000_000,
    },
    "time": {
        "ntp_host": "europe.pool.ntp.org",
        "utc_offset_seconds": 3600,
        "display_before_hour": 9,
    },
    "mqtt": {
        "enabled": False,
        "host": "YOUR_MQTT_BROKER",
        "port": 1883,
        "user": "YOUR_MQTT_USER",
        "password": "YOUR_MQTT_PASSWORD",
        "client_id": "e-paper-esp32-frame",
        "topic_prefix": "e-paper-esp32-frame/log",
        "keepalive": 60,
        "history_slots": 100,
        "sequence_file": "/mqtt-sequence.txt",
    },
    "logging": {
        "file": "/log.txt",
        "max_bytes": 256 * 1024,
    },
    "sleep": {
        "target_hour": 10,
        "target_minute": 0,
        "fallback_seconds": 24 * 60 * 60,
    },
}
