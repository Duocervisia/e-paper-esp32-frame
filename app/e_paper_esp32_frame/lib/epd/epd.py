try:
    import ustruct as struct
except ImportError:
    import struct

import micropython

from machine import Pin, SPI
from time import sleep_ms

from e_paper_esp32_frame.mqtt import log


EPD_WIDTH = 800
EPD_HEIGHT = 480

EPD_7IN3E_BLACK = 0x0
EPD_7IN3E_WHITE = 0x1
EPD_7IN3E_YELLOW = 0x2
EPD_7IN3E_RED = 0x3
EPD_7IN3E_BLUE = 0x5
EPD_7IN3E_GREEN = 0x6

DISPLAY_PALETTE = (
    (0, 0, 0),
    (255, 255, 255),
    (255, 255, 0),
    (255, 0, 0),
    (0, 0, 255),
    (0, 255, 0),
)

DISPLAY_COLOR_INDEX = {
    0x000000: EPD_7IN3E_BLACK,
    0xFFFFFF: EPD_7IN3E_WHITE,
    0xFFFF00: EPD_7IN3E_YELLOW,
    0xFF0000: EPD_7IN3E_RED,
    0x0000FF: EPD_7IN3E_BLUE,
    0x00FF00: EPD_7IN3E_GREEN,
}


def _nearest_color_index(red, green, blue):
    best_index = EPD_7IN3E_BLACK
    best_score = 0

    score = 195075 - 510 * (red + green + blue)
    if score < best_score:
        best_score = score
        best_index = EPD_7IN3E_WHITE

    score = 130050 - 510 * (red + green)
    if score < best_score:
        best_score = score
        best_index = EPD_7IN3E_YELLOW

    score = 65025 - 510 * red
    if score < best_score:
        best_score = score
        best_index = EPD_7IN3E_RED

    score = 65025 - 510 * blue
    if score < best_score:
        best_score = score
        best_index = EPD_7IN3E_BLUE

    score = 65025 - 510 * green
    if score < best_score:
        best_index = EPD_7IN3E_GREEN

    return best_index


@micropython.native
def _pack_dithered_row(row, packed, width):
    output_index = 0
    for column in range(width - 1, -1, -2):
        pixel_index = column * 3
        blue = row[pixel_index]
        green = row[pixel_index + 1]
        red = row[pixel_index + 2]
        left = DISPLAY_COLOR_INDEX.get((red << 16) | (green << 8) | blue)
        if left is None:
            left = _nearest_color_index(red, green, blue)

        if column > 0:
            pixel_index -= 3
            blue = row[pixel_index]
            green = row[pixel_index + 1]
            red = row[pixel_index + 2]
            right = DISPLAY_COLOR_INDEX.get((red << 16) | (green << 8) | blue)
            if right is None:
                right = _nearest_color_index(red, green, blue)
        else:
            right = EPD_7IN3E_WHITE

        packed[output_index] = (left << 4) | right
        output_index += 1


def _clamp(value):
    if value < 0:
        return 0
    if value > 255:
        return 255
    return value


class EPD:
    def __init__(self, config):
        self._cfg = config
        self._spi = SPI(
            config["spi_id"],
            baudrate=config["baudrate"],
            polarity=0,
            phase=0,
            sck=Pin(config["sck"]),
            mosi=Pin(config["mosi"]),
        )
        self._rst = Pin(config["rst"], Pin.OUT, value=1)
        self._dc = Pin(config["dc"], Pin.OUT, value=0)
        self._cs = Pin(config["cs"], Pin.OUT, value=1)
        self._busy = Pin(config["busy"], Pin.IN)
        self.width = config.get("width", EPD_WIDTH)
        self.height = config.get("height", EPD_HEIGHT)
        self._initialized = False

    def _write_command(self, command):
        self._dc.off()
        self._cs.off()
        self._spi.write(bytearray((command,)))
        self._cs.on()

    def _write_data(self, data):
        self._dc.on()
        self._cs.off()
        if isinstance(data, int):
            self._spi.write(bytearray((data,)))
        else:
            self._spi.write(data)
        self._cs.on()

    def _busy_wait(self):
        while self._busy.value() == 0:
            sleep_ms(1)

    def _reset(self):
        self._rst.on()
        sleep_ms(20)
        self._rst.off()
        sleep_ms(2)
        self._rst.on()
        sleep_ms(20)

    def init(self):
        self._reset()
        sleep_ms(20)
        self._busy_wait()

        self._write_command(0xAA)
        for value in (0x49, 0x55, 0x20, 0x08, 0x09, 0x18):
            self._write_data(value)

        self._write_command(0x01)
        for value in (0x3F, 0x00, 0x32, 0x2A, 0x0E, 0x2A):
            self._write_data(value)

        self._write_command(0x00)
        self._write_data(0x5F)
        self._write_data(0x69)

        self._write_command(0x03)
        for value in (0x00, 0x54, 0x00, 0x44):
            self._write_data(value)

        self._write_command(0x05)
        for value in (0x40, 0x1F, 0x1F, 0x2C):
            self._write_data(value)

        self._write_command(0x06)
        for value in (0x6F, 0x1F, 0x1F, 0x22):
            self._write_data(value)

        self._write_command(0x08)
        for value in (0x6F, 0x1F, 0x1F, 0x22):
            self._write_data(value)

        self._write_command(0x13)
        self._write_data(0x00)
        self._write_data(0x04)

        self._write_command(0x30)
        self._write_data(0x3C)

        self._write_command(0x41)
        self._write_data(0x00)

        self._write_command(0x50)
        self._write_data(0x3F)

        self._write_command(0x60)
        self._write_data(0x02)
        self._write_data(0x00)

        self._write_command(0x61)
        self._write_data(0x03)
        self._write_data(0x20)
        self._write_data(0x01)
        self._write_data(0xE0)

        self._write_command(0x82)
        self._write_data(0x1E)

        self._write_command(0x84)
        self._write_data(0x00)

        self._write_command(0x86)
        self._write_data(0x00)

        self._write_command(0xE3)
        self._write_data(0x2F)

        self._write_command(0xE0)
        self._write_data(0x00)

        self._write_command(0xE6)
        self._write_data(0x00)

        self._initialized = True
        return 0

    def _power_on(self):
        self._write_command(0x04)
        sleep_ms(20)
        self._busy_wait()

    def _refresh(self):
        self._write_command(0x12)
        self._write_data(0x00)
        sleep_ms(100)
        self._busy_wait()

    def _power_off(self):
        self._write_command(0x02)
        self._write_data(0x00)
        self._busy_wait()

    def sleep(self):
        if self._initialized:
            self._power_off()

    def _nearest_color_index(self, red, green, blue):
        exact_index = DISPLAY_COLOR_INDEX.get((red << 16) | (green << 8) | blue)
        if exact_index is not None:
            return exact_index
        return _nearest_color_index(red, green, blue)

    def _pack_two_pixels(self, left, right):
        return bytes(((left << 4) | right,))

    def display_bmp(self, filename):
        log("drawing {}".format(filename))
        with open(filename, "rb") as bmp_file:
            magic = bmp_file.read(2)
            if magic != b"BM":
                raise ValueError("not a bmp file")

            bmp_file.seek(10)
            pixel_offset = struct.unpack("<I", bmp_file.read(4))[0]
            header_size = struct.unpack("<I", bmp_file.read(4))[0]
            width = struct.unpack("<I", bmp_file.read(4))[0]
            height = struct.unpack("<I", bmp_file.read(4))[0]
            bmp_file.seek(2, 1)
            bit_depth = struct.unpack("<H", bmp_file.read(2))[0]

            if bit_depth != 24:
                raise ValueError("only 24-bit bmps are supported in this build")

            line_size = ((bit_depth * width + 31) // 32) * 4
            self._write_command(0x10)
            row = bytearray(line_size)
            packed = bytearray((width + 1) // 2)

            # Match the legacy Arduino implementation and the physical frame
            # orientation: BMP rows and pixels are both sent in reverse order,
            # which rotates the image by 180 degrees.
            for row_index in range(height):
                rows_done = row_index
                if rows_done % 60 == 0:
                    log("sending row {}/{}".format(rows_done, height))

                bmp_file.seek(pixel_offset + row_index * line_size)
                if bmp_file.readinto(row) != line_size:
                    raise ValueError("truncated bmp pixel data")

                _pack_dithered_row(row, packed, width)
                self._write_data(packed)

            log("pixel data sent, refreshing display")
            self._power_on()
            self._refresh()
            log("display refresh complete")
            self._power_off()
