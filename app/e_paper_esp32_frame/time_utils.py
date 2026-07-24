import time as time_module

from e_paper_esp32_frame.config import CONFIG


def now_local():
    return time_module.localtime(time_module.time() + CONFIG["time"]["utc_offset_seconds"])


def display_date_string(now=None):
    if now is None:
        now = now_local()

    if now[3] < CONFIG["time"]["display_before_hour"]:
        day = now[2] - 1
        month = now[1]
        year = now[0]
        if day == 0:
            prev = time_module.localtime(
                time_module.time() + CONFIG["time"]["utc_offset_seconds"] - 24 * 60 * 60
            )
            day = prev[2]
            month = prev[1]
            year = prev[0]
    else:
        day = now[2]
        month = now[1]
        year = now[0]

    return "{:02d}.{:02d}".format(day, month), year


def increment_date_string(date_string):
    day = int(date_string[0:2])
    month = int(date_string[3:5])
    year = 2000
    stamp = int(time_module.mktime((year, month, day + 1, 0, 0, 0, 0, 0)))
    normalized = time_module.localtime(stamp)
    return "{:02d}.{:02d}".format(normalized[2], normalized[1])


def seconds_until_next_image(now=None):
    if now is None:
        now = now_local()

    current_seconds = now[3] * 3600 + now[4] * 60 + now[5]
    target_seconds = CONFIG["sleep"]["target_hour"] * 3600 + CONFIG["sleep"]["target_minute"] * 60

    if current_seconds <= target_seconds - 3600:
        return target_seconds - current_seconds

    return 24 * 3600 - current_seconds + target_seconds
