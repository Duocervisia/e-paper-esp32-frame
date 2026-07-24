from e_paper_esp32_frame.config import CONFIG

import os
import sys
import time

try:
    import ujson as json
except ImportError:
    import json

try:
    from e_paper_esp32_frame.lib.umqttsimple import MQTTClient
except ImportError:
    try:
        from umqtt.simple import MQTTClient
    except ImportError:
        MQTTClient = None

try:
    import ubinascii as binascii
except ImportError:
    import binascii

import machine

_client = None
_enabled = bool(CONFIG.get("mqtt", {}).get("enabled", False))
_base_topic = None
_history = []
_partial = ""
_sequence = None


def _timestamp():
    now = time.localtime(time.time() + CONFIG["time"]["utc_offset_seconds"])
    if now[0] < 2024:
        return "UNSYNCED +{}ms".format(time.ticks_ms())
    return "{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}".format(
        now[0], now[1], now[2], now[3], now[4], now[5]
    )


def _append_local(message):
    logging_cfg = CONFIG.get("logging", {})
    path = logging_cfg.get("file", "/log.txt")
    max_bytes = logging_cfg.get("max_bytes", 256 * 1024)

    try:
        try:
            file_size = os.stat(path)[6]
        except OSError:
            file_size = 0

        mode = "a"
        if max_bytes > 0 and file_size >= max_bytes:
            mode = "w"

        stamp = _timestamp()
        with open(path, mode) as file_handle:
            if mode == "w":
                file_handle.write("[{}] log rotated after {} bytes\n".format(stamp, file_size))

            for line in str(message).split("\n"):
                if line:
                    file_handle.write("[{}] {}\n".format(stamp, line))
    except Exception as error:
        print("local log write failed: {}".format(error))


def _device_id():
    return binascii.hexlify(machine.unique_id()).decode()


def _topic_prefix():
    mqtt_cfg = CONFIG.get("mqtt", {})
    return mqtt_cfg.get("topic_prefix", "e-paper-esp32-frame/log")


def _publish_to(topic, message, retain=False):
    global _client
    if _client is None:
        return False
    try:
        _client.publish(topic, message.encode(), retain=retain, qos=1)
        return True
    except Exception as error:
        failure = "mqtt publish failed: {}".format(error)
        print(failure)
        _append_local(failure)
        _client = None
        return False


def _next_sequence():
    global _sequence

    mqtt_cfg = CONFIG.get("mqtt", {})
    sequence_file = mqtt_cfg.get("sequence_file", "/mqtt-sequence.txt")
    if _sequence is None:
        try:
            with open(sequence_file, "r") as file_handle:
                _sequence = int(file_handle.read())
        except (OSError, ValueError):
            _sequence = -1

    _sequence += 1
    try:
        with open(sequence_file, "w") as file_handle:
            file_handle.write(str(_sequence))
    except OSError as error:
        failure = "mqtt sequence save failed: {}".format(error)
        print(failure)
        _append_local(failure)
    return _sequence


def _message_level(message):
    lowered = message.lower()
    for marker in ("error", "failed", "fatal", "traceback", "timed out"):
        if marker in lowered:
            return "error"
    return "info"


def _publish_log_event(message):
    if _client is None or _base_topic is None:
        return False

    mqtt_cfg = CONFIG.get("mqtt", {})
    slot_count = mqtt_cfg.get("history_slots", 100)
    if slot_count < 1:
        slot_count = 1

    sequence = _next_sequence()
    slot = sequence % slot_count
    level = _message_level(message)
    event = {
        "sequence": sequence,
        "slot": slot,
        "timestamp": _timestamp(),
        "level": level,
        "message": message,
    }
    payload = json.dumps(event)

    if not _publish_to(
        "{}/history/{:03d}".format(_base_topic, slot),
        payload,
        retain=True,
    ):
        return False

    latest = {
        "sequence": sequence,
        "slot": slot,
        "timestamp": event["timestamp"],
        "history_slots": slot_count,
    }
    if not _publish_to(_base_topic + "/latest", json.dumps(latest), retain=True):
        return False

    if level == "error":
        _publish_to(_base_topic + "/last-error", payload, retain=True)
    return True


def publish_status(status, details=None):
    if _client is None or _base_topic is None:
        return False

    payload = {
        "timestamp": _timestamp(),
        "status": status,
    }
    if details is not None:
        payload["details"] = details
    return _publish_to(_base_topic + "/status", json.dumps(payload), retain=True)


def publish_battery_status(raw, voltage, percentage, critical):
    if _client is None or _base_topic is None:
        return False

    payload = {
        "timestamp": _timestamp(),
        "raw": raw,
        "voltage": voltage,
        "percentage": percentage,
        "percentage_display": round(percentage * 100),
        "critical": critical,
        "unit": "V",
    }
    return _publish_to(_base_topic + "/battery", json.dumps(payload), retain=True)


def setup():
    global _client, _enabled, _base_topic, _history

    mqtt_cfg = CONFIG.get("mqtt", {})
    if not _enabled:
        return False

    if MQTTClient is None:
        log("mqtt disabled: client library missing")
        _enabled = False
        return False

    host = mqtt_cfg.get("host", "")
    if not host:
        log("mqtt disabled: no broker configured")
        _enabled = False
        return False

    client_id = mqtt_cfg.get("client_id", "e-paper-esp32-frame") + "-" + _device_id()
    _base_topic = _topic_prefix() + "/" + _device_id()
    _client = MQTTClient(
        client_id,
        host,
        port=mqtt_cfg.get("port", 1883),
        user=mqtt_cfg.get("user", None) or None,
        password=mqtt_cfg.get("password", None) or None,
        keepalive=mqtt_cfg.get("keepalive", 60),
    )

    try:
        _client.connect()
    except Exception as error:
        log("mqtt connect failed: {}".format(error))
        _client = None
        return False

    pending_history = _history
    _history = []
    for index, message in enumerate(pending_history):
        if not _publish_log_event(message):
            _history.extend(pending_history[index:])
            return False

    log("mqtt connected: {}".format(host))
    publish_status("online")
    return True


def log(message="", end="\n"):
    global _partial

    text = str(message)
    print(text, end=end)

    if end == "\n":
        full_message = _partial + text
        _partial = ""
        if full_message:
            _append_local(full_message)

        if not full_message:
            return
        if not _enabled:
            return
        if _client is None:
            if full_message:
                _history.append(full_message)
            return
        _publish_log_event(full_message)
        return

    _partial += text


def log_exception(error):
    log("fatal error: {}: {}".format(type(error).__name__, error))

    logging_cfg = CONFIG.get("logging", {})
    path = logging_cfg.get("file", "/log.txt")
    try:
        with open(path, "a") as file_handle:
            file_handle.write("[{}] traceback:\n".format(_timestamp()))
            sys.print_exception(error, file_handle)
    except Exception as log_error:
        print("local traceback write failed: {}".format(log_error))

    sys.print_exception(error)


def flush():
    global _partial

    if _partial:
        message = _partial
        _partial = ""
        _append_local(message)
        if _enabled:
            if _client is None:
                _history.append(message)
            else:
                _publish_log_event(message)


def shutdown():
    global _client

    flush()
    if _client is None:
        return

    try:
        _client.disconnect()
    except Exception as error:
        failure = "mqtt disconnect failed: {}".format(error)
        print(failure)
        _append_local(failure)
    finally:
        _client = None
