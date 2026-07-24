def ds(seconds):
    """Put the MCU into deep sleep."""
    from e_paper_esp32_frame.mqtt import log, publish_status, shutdown

    from machine import Pin, deepsleep
    import esp32

    from e_paper_esp32_frame.config import CONFIG

    if seconds is None or seconds < 0:
        seconds = CONFIG["sleep"]["fallback_seconds"]

    publish_status("sleeping", {"wakeup_in_seconds": seconds})
    log("> deepsleep")

    display_cfg = CONFIG["display"]
    power_pin = Pin(display_cfg["power_pin"], Pin.OUT, value=1)

    esp32.gpio_deep_sleep_hold(True)
    power_pin.init(Pin.OUT, hold=True, value=1)

    log("wakeup in {}s".format(seconds))
    shutdown()
    deepsleep(int(seconds * 1000))
