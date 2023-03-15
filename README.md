## Hardware

* A development board with ESP32-S3
* A USB cable for Power supply and programming

The following development board has been used for the development of this project.

| Board              | LED type    | Pin    |
| ------------------ | ----------- | ------ |
| ESP32-S3-DevKitC-1 | Addressable | GPIO48 |

See [Development Board ESP32-S3-DevKitC-1](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/hw-reference/esp32s3/user-guide-devkitc-1.html) for more information about it.

See [WS2812](http://www.world-semi.com/Certifications/WS2812B.html) for more information about the addressable LED.

See [LED Strip Example](https://github.com/espressif/esp-idf/tree/master/examples/peripherals/rmt/led_strip) to review other examples on how to control LED peripherals with an ESP32 board.

## Development Environment

To configure the laptop with the required development environment, it is recommended to use VS Code and the Espressif IDF extension. 

Setting up the workspace in VS Code for the current project, requires only the following steps:

* Install the Espressif IDF Extension in VS Code [(link)](https://github.com/espressif/vscode-esp-idf-extension/blob/59a99375c36c2d72e1f78ed477627ea8a4976c14/docs/tutorial/install.md)
* Clone the repository `https://github.com/ands23/esp32_led.git`
* Open the repository folder in VS Code
* Launch **ESP-IDF: Add vscode configuration folder**

To build, flash, monitor and debug the project using the extension, see the [VS Code Espressif IDF Onboarding](https://github.com/espressif/vscode-esp-idf-extension/blob/HEAD/docs/ONBOARDING.md).

Alternatively the following [ESP-IDF Getting Started Guide](https://idf.espressif.com/) explains how to configure, build and flash the project by using directly the `idf.py` commands.





