import os

try:
    import ujson as json
except ImportError:
    import json

from machine import Pin

from e_paper_esp32_frame.config import CONFIG
from e_paper_esp32_frame.mqtt import log
from e_paper_esp32_frame.time_utils import display_date_string, increment_date_string


def _join_path(base, name):
    if base.endswith("/"):
        return base + name.lstrip("/")
    return base + "/" + name.lstrip("/")


def mount_storage():
    mount_point = CONFIG["storage"]["mount_point"]
    try:
        names = os.listdir(mount_point)
        setup_name = CONFIG["storage"]["setup_file"].rsplit("/", 1)[-1]
        if setup_name in names or any(name.lower().endswith(".bmp") for name in names):
            log("sd already mounted")
            return mount_point
    except OSError:
        pass

    sd_cfg = CONFIG["sd"]
    from machine import SDCard
    import vfs

    sd = SDCard(
        slot=sd_cfg["spi_id"],
        sck=Pin(sd_cfg["sck"]),
        mosi=Pin(sd_cfg["mosi"]),
        miso=Pin(sd_cfg["miso"]),
        cs=Pin(sd_cfg["cs"], Pin.OUT, value=1),
        freq=sd_cfg["baudrate"],
    )
    vfs.mount(sd, mount_point)
    log("sd mounted")
    return mount_point


def read_text(path):
    try:
        with open(path, "r") as file_handle:
            return file_handle.read()
    except OSError:
        return ""


def load_state():
    try:
        with open(CONFIG["storage"]["state_file"], "r") as file_handle:
            state = json.loads(file_handle.read())
            if isinstance(state, dict):
                return state
    except OSError:
        pass
    return {
        "checker": "",
        "file_count": 0,
        "image_index": 0,
        "date": "01.01",
    }


def save_state(state):
    try:
        with open(CONFIG["storage"]["state_file"], "w") as file_handle:
            file_handle.write(json.dumps(state))
    except OSError as error:
        log("state save failed: {}".format(error))


def scan_bmp_files():
    root = CONFIG["storage"]["image_root"]
    files = []
    for name in os.listdir(root):
        if name.lower().endswith(".bmp"):
            files.append(name)
    files.sort()
    return files


def refresh_catalog(state):
    info_text = read_text(CONFIG["storage"]["info_file"])
    if state.get("checker", "") != info_text:
        state["checker"] = info_text
        state["image_index"] = 0

    files = scan_bmp_files()
    state["file_count"] = len(files)
    return files


def select_image(files, state, time_working):
    if not files:
        return None

    if time_working:
        date_string, _ = display_date_string()
    else:
        date_string = increment_date_string(state.get("date", "01.01"))

    state["date"] = date_string

    for file_name in files:
        if date_string in file_name:
            return _join_path(CONFIG["storage"]["image_root"], file_name)

    file_count = len(files)
    image_index = state.get("image_index", 0)
    if image_index >= file_count - 1:
        next_index = 0
    else:
        next_index = image_index + 1

    state["image_index"] = next_index
    return _join_path(CONFIG["storage"]["image_root"], files[image_index])
