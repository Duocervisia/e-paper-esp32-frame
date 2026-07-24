import gc

from machine import freq

from e_paper_esp32_frame.battery import bat_idle
from e_paper_esp32_frame.config import CONFIG
from e_paper_esp32_frame.ds import ds
from e_paper_esp32_frame.epd import EPD
from e_paper_esp32_frame.mqtt import (
    log,
    log_exception,
    setup as setup_mqtt,
)
from e_paper_esp32_frame.internet import connect_wifi, sync_time
from e_paper_esp32_frame.storage import load_state, mount_storage, refresh_catalog, save_state, select_image
from e_paper_esp32_frame.time_utils import seconds_until_next_image


def run():
    freq(240_000_000)
    gc.collect()

    log("-- main --")

    storage_ok = True
    try:
        mount_storage()
    except Exception as error:
        storage_ok = False
        log("sd mount failed: {}: {}".format(type(error).__name__, error))

    if storage_ok:
        state = load_state()
        files = refresh_catalog(state)
    else:
        state = None
        files = []

    wifi_ok = connect_wifi()
    time_ok = False
    if wifi_ok:
        time_ok = sync_time()
        setup_mqtt()

    try:
        bat_idle()
    except Exception as error:
        log("battery voltage read failed: {}: {}".format(type(error).__name__, error))

    if not storage_ok:
        log("display cycle aborted: sd card unavailable")
        ds(CONFIG["sleep"]["fallback_seconds"])
        return

    display = EPD()
    if display.init() != 0:
        log("display init failed")
        ds(CONFIG["sleep"]["fallback_seconds"])
        return

    image_path = select_image(files, state, time_ok)
    if image_path is None:
        log("no bmp files found")
        save_state(state)
        display.sleep()
        ds(CONFIG["sleep"]["fallback_seconds"])
        return

    log("displaying {}".format(image_path))
    display.display_bmp(image_path)
    display.sleep()

    save_state(state)

    if time_ok:
        sleep_seconds = seconds_until_next_image()
    else:
        sleep_seconds = CONFIG["sleep"]["fallback_seconds"]

    ds(sleep_seconds)


try:
    run()
except Exception as error:
    log_exception(error)
    raise
