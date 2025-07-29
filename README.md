# E-paper ESP32 Frame

> **Note:** Changed the display from Waveshare 7.3inch ACeP 7-Color E-Paper E-Ink Display to Waveshare 7.3inch E Ink Spectra 6 (E6) Full Color E-Paper Display for better image quality

![ESP e-paper frame](images/e-paper-esp32-frame.jpg?raw=true)
![ESP e-paper frame](images/e-paper-esp32-frame-backside.jpg?raw=true)

This project provides a comprehensive installation guide for an e-paper picture frame that updates daily. The frame features a Waveshare E6 full color e-paper display, which, combined with the implemented Floyd-Steinberg Dithering algorithm, creates the illusion of a greater color depth. Users have the ability to convert their own images using the included BMP-Converter. Additionally, the frame can be connected to the internet to display specific images on designated days. The 1000mAh battery is expected to last for approximately 700 days and can be recharged through the ESP32's type-C port.

## Features

- **Daily Updates**: Automatically updates the displayed image daily.
- **Internet Connectivity**: Connects to the internet to fetch images based on specific dates.
- **Image Conversion**: Includes a BMP-Converter for converting images to the required format.
- **Low Power Consumption**: Utilizes the FireBeetle 2 ESP32-E for low power consumption during deep sleep.
- **Customizable**: Users can convert and display their own images.
- **3D Printed Case**: Provides a 3D printed case design for housing all components

## Table of Contents

- [Components](#components)
- [Installation](#installation)
- [Convert the images](#convert-the-images)
- [Contributing](#contributing)
- [License](#license)

## Components

- [FireBeetle 2 ESP32-E](https://www.dfrobot.com/product-2195.html) (8,20€): A microcontroller with low power consumption during deep sleep.
- [Li-Po 503450 1000mAh 3.7V with PH2.0 connector](https://de.aliexpress.com/item/1005005848216887.html?spm=a2g0o.productlist.main.7.4882bxqubxqujS&algo_pvid=f88e0468-64ce-490d-baf5-9a3526f8d347&aem_p4p_detail=202410210512142839878311816700000036523&algo_exp_id=f88e0468-64ce-490d-baf5-9a3526f8d347-3&pdp_npi=4%40dis%21EUR%2123.19%2123.19%21%21%2124.61%2124.61%21%40211b618e17295127342901132eb8fb%2112000034564214083%21sea%21DE%213852088484%21X&curPageLogUid=NMeMpLOLSds0&utparam-url=scene%3Asearch%7Cquery_from%3A&search_p4p_id=202410210512142839878311816700000036523_1) (~2.30€ p.p.): A rechargeable lithium polymer battery for power supply. (Dimensions: 5mm (H) x 34mm (W) x 50mm (L))
- [Micro SD Card Module](https://de.aliexpress.com/item/1005005591145849.html?spm=a2g0o.productlist.main.3.a9e0333916KKv5&algo_pvid=ddaef2a1-d621-4a9a-8b38-0c9e925de657&algo_exp_id=ddaef2a1-d621-4a9a-8b38-0c9e925de657-1&pdp_npi=4%40dis%21EUR%211.85%211.85%21%21%211.96%211.96%21%40210390b817295128395262508eb456%2112000033669348102%21sea%21DE%213852088484%21X&curPageLogUid=DYdi0FD60FO3&utparam-url=scene%3Asearch%7Cquery_from%3A) (~0.37€ p.p.): A module for handling micro SD cards. (Dimensions: 18mm x 18mm)
- [7.3inch E Ink Spectra 6 (E6) Full Color E-Paper Display Module + HAT](https://www.waveshare.com/product/displays/e-paper/epaper-1/7.3inch-e-paper-hat-e.htm) (~70€): An e-paper display module with seven colors, SPI communication, featuring 800×480 pixels.
- [AO3400 Mosfet](https://de.aliexpress.com/item/1005006142488372.html?spm=a2g0o.productlist.main.3.522061ed9rfvJa&algo_pvid=978c5230-3de5-4602-bc51-f52ba142549f&algo_exp_id=978c5230-3de5-4602-bc51-f52ba142549f-2&pdp_ext_f=%7B%22order%22%3A%22513%22%2C%22eval%22%3A%221%22%7D&pdp_npi=4%40dis%21EUR%213.81%211.25%21%21%2130.92%2110.15%21%402103868817537992087935798eae7d%2112000035951341855%21sea%21DE%213852088484%21X&curPageLogUid=ftciQM0K6a8g&utparam-url=scene%3Asearch%7Cquery_from%3A) (0.05€ p.p.): This mosfet cuts off power to the SD Card and e-paper display during deep sleep.
- Toggle Switch (Optional): A switch used to turn off the power from the battery. (Thread diameter: 5mm)
- Printed Case: A 3D printed case to hold all the components.
- Four Heat Inserts and Screws: Hardware components for assembly. (M3)
- Picture frame: A standard picture frame that can accommodate the e-paper frame.

Total cost for essential components: 80.92€


## Installation

1. Clone the repository:
	```sh
	git clone https://github.com/Duocervisia/e-paper-esp32-frame.git
	```
3. Connect your ESP32 to the e-paper display and SD card module according to the `Schematic for Components.png`. Cut the low-power Solder Jumper Pad, located on the front side of the Firebeetle 2, to achieve optimal power consumption during deep sleep.
4. Upload the code to your ESP32.
5. Convert the images you want to display using the BMP-Converter. See [Convert the images](#convert-the-images). This application can convert most image formats to BMP format with the correct dimensions of 800 x 480 px, which is needed for the microcontroller to display the image. It also provides basic operations to modify the images.
6. Add the converted images to the SD card, including the info.txt file.
8. Print both .stl files that can be found in the `/3D_modell` directory. Make sure that the Lid is printed with the nail holder facing up. For more detailed information, refer to the [README.md](3D_modell/README.md) in the `/3D_modell` directory.
9. (Optional) Create a small hole, for example using a soldering iron, between the battery and ESP32 to integrate a toggle switch.
10. Attach the 3D printed case to the backplate of the picture frame by creating a small slit on the top. You can either glue the case to the backplate or use screws to secure them together. On the opposite side, mount the e-paper display. Connect the ribbon cable of the e-paper to the case through the created slit. For a perfect fit, use the provided `passepartout.pdf` file.

## Convert the images

To convert the images, follow these steps:

1. Navigate to the `/bmpConverter/build` directory.
2. Run the `converter.exe` executable.

Alternatively, you can follow these steps:

1. Navigate to the `/bmpConverter` directory.
2. Read the [README.md](bmpConverter/README.md) file in the directory and install the required dependencies.
3. Run the `converter.py` script.

## Contributing

Contributions are highly appreciated! Feel free to open an issue or submit a pull request. A big thank you to TreeSparrow69 for their valuable contributions to this project.

## License

This project is licensed under the MIT License.
