from machine import ADC, Pin
from time import sleep

from e_paper_esp32_frame.config import CONFIG
from e_paper_esp32_frame.mqtt import log, publish_battery_status


_battery_cfg = CONFIG["battery"]
_battery_adc = ADC(Pin(_battery_cfg["pin"], Pin.IN))
_battery_adc.atten(ADC.ATTN_11DB)
_battery_adc.width(ADC.WIDTH_12BIT)


def read_battery_voltage():
    """Return the averaged raw ADC value and calibrated battery voltage."""
    sample_count = _battery_cfg.get("samples", 10)
    raw = 0
    for _ in range(sample_count):
        raw += _battery_adc.read()
        sleep(0.01)
    raw //= sample_count

    vref = _battery_cfg.get("vref", 3.578)
    measured_voltage = (raw / 4095) * vref
    battery_voltage = measured_voltage * _battery_cfg.get("divider_ratio", 2.0)
    return raw, battery_voltage


def bat_idle():
    """Measure, log and publish raw value, voltage and charge percentage."""
    if not _battery_cfg.get("enabled", True):
        log("battery monitoring disabled")
        return None

    raw, voltage = read_battery_voltage()
    percentage = max(min((voltage - 3.3) / (4.2 - 3.3), 1.0), 0.0)
    critical = percentage <= _battery_cfg.get("critical", 0.1)

    if critical:
        log("battery critical: {}% ({:.2f} V)".format(round(percentage * 100), voltage))
    else:
        log("battery: {}% ({:.2f} V)".format(round(percentage * 100), voltage))

    publish_battery_status(raw, voltage, percentage, critical)
    return raw, voltage, percentage
