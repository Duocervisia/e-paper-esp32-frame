from e_paper_esp32_frame.config import CONFIG
from e_paper_esp32_frame.lib.epd.epd import EPD as LowLevelEPD


class EPD(LowLevelEPD):
    def __init__(self):
        super().__init__(CONFIG["display"])
