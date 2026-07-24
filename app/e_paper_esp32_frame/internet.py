import ujson as json

from e_paper_esp32_frame.config import CONFIG
from e_paper_esp32_frame.mqtt import log


def load_wifi_credentials():
    try:
        with open(CONFIG["storage"]["setup_file"], "r") as file_handle:
            data = json.loads(file_handle.read())
        return data.get("ssid", ""), data.get("password", "")
    except (OSError, ValueError):
        wifi = CONFIG["backup_wifi"]
        log("wifi credentials: using backup configuration")
        return wifi["ssid"], wifi["password"]


def connect_wifi(timeout_seconds=20):
    import network
    import time

    ssid, password = load_wifi_credentials()
    if not ssid:
        log("wifi disabled: no ssid configured")
        return False

    nic = network.WLAN(network.STA_IF)
    nic.active(True)

    static_ip = CONFIG["backup_wifi"].get("static_ip")
    if static_ip:
        nic.ifconfig(static_ip)

    if nic.isconnected():
        log("wifi already connected")
        return True

    log("connecting to wifi {}...".format(ssid), end="")
    nic.connect(ssid, password)

    start = time.ticks_ms()
    while not nic.isconnected():
        if time.ticks_diff(time.ticks_ms(), start) > timeout_seconds * 1000:
            log(" timed out")
            return False
        log(".", end="")
        time.sleep_ms(500)

    log(" connected")
    log(nic.ifconfig())
    return True


def sync_time():
    import ntptime

    try:
        ntptime.host = CONFIG["time"]["ntp_host"]
        ntptime.settime()
        log("time synced")
        return True
    except OSError as error:
        log("time sync failed: {}".format(error))
        return False
