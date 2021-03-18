# ESP HomeKit SDK
## Introduction

[HomeKit](https://developer.apple.com/homekit/) is a framework developed by Apple for communicating with and controlling connected accessories in a user’s home using iOS devices.
ESP HomeKit SDK has been developed in-house by Espressif to build Apple HomeKit compatible accessories using ESP32/ESP32-S2/ESP32-C3/ESP8266 SoCs.

> Note: If you want to use HomeKit for commercial products, please check [here](https://www.espressif.com/en/products/sdks/esp-homekit-sdk) for access to the MFi variant of this SDK. It has the exact same APIs as this and so, moving to it should be easy. However, commercial HomeKit products can be built only with ESP32/ESP32-S2/ESP32-C3 since ESP8266 cannot meet all the HomeKit Certification requirements.

> If you want to use a port of Apple's ADK instead, please check [here](https://github.com/espressif/esp-apple-homekit-adk)

Features of this SDK:

* Easy APIs to implement all standard HomeKit profiles defined by Apple.
* Facility to add customised accessory specific Services and Characteristics.
* Samples (Fan, Lightbulb, Outlet, Bridge, Data-TLV8, Ethernet) to enable quick accessory development.
* Support for ESP Unified Provisioning.

## Get Started

### Set up Host environment

Set up the host environment and ESP IDF (**master** branch) as per the steps given [here](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html).

If you are using **ESP8266**, set-up ESP8266-RTOS-SDK (**master** branch) as per the steps given [here](https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/get-started/).

### Get esp-homekit-sdk

Please clone this repository using the below command:

```text
git clone --recursive https://github.com/espressif/esp-homekit-sdk.git
```

> Note the --recursive option. This is required to pull in the JSON dependencies into esp-homekit-sdk. In case you have already cloned the repository without this option, execute this to pull in the submodules:
> `git submodule update --init --recursive`
>
> If you had already cloned the repository and submodules earlier, you may have to execute `git submodule sync --recursive` once since the submodule paths have changed.


### Compile and Flash

You can use esp-homekit-sdk with any ESP32, ESP32-S2, ESP32-C3 or ESP8266 board (though we have tested only with the ESP32-DevKit-C, ESP32-S2-Saola-1,  ESP32-C3-DevKit-M-1, ESP8266-DevKit-C). We have provided multiple examples for reference. Compile and flash as below (fan used as example):

**For ESP32/ESP32-S2/ESP32-C3**

```text
$ cd /path/to/esp-homekit-sdk/examples/fan
$ export ESPPORT=/dev/tty.SLAB_USBtoUART #Set your board's serial port here
$ idf.py set-target <esp32/esp32s2/esp32c3>
$ idf.py flash monitor
```

**For ESP8266**

```text
$ cd /path/to/esp-homekit-sdk/examples/fan
$ export ESPPORT=/dev/tty.SLAB_USBtoUART #Set your board's serial port here
$ make defconfig
$ make flash monitor
```


As the device boots up, you will see two QR codes, a small one for HomeKit and a larger one for Wi-Fi provisioning. Please use any of the [Espressif Provisioning Apps](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/provisioning.html#provisioning-tools) for Wi-Fi provisioning.

> Note: For the Open source HomeKit SDK, the HomeKit QR code cannot be used for provisioning from the Home app. It can be used only for pairing, after the Wi-Fi provisioning is done. For provisioning from Home app, you will need the MFi variant of the SDK.

> Note: If you want to use hard-coded credentials instead of Provisioning, please set the ssid and passphrase by navigating to `idf.py menuconfig -> App Wi-Fi -> Source of Wi-Fi Credentials -> Use Hardcoded`

After the device connects to your Home Wi-Fi network it can be added in the Home app

### Add acccessory in the Home app

Open the Home app on your iPhone/iPad and follow these steps

- Tap on "Add Accessory" and scan the small QR code mentioned above.
- If QR code is not visible correctly, you may use the link printed on the serial terminal or follow these steps:
    - Choose the "I Don't Have a Code or Cannot Scan" option.
    - Tap on "Esp-Fan-xxxxxx" in the list of Nearby Accessories.
    - Select the "Add Anyway" option for the "Uncertified Accessory" prompt.
    - Enter 11122333 as the Setup code.
- You should eventually see the "Esp-Fan-xxxxxx added" message.
- Give a custom name, assign to a room, create scenes as required and you are done.

> For ESP8266, the pairing may take a bit longer.

### Changing the Setup Code

The HomeKit setup code can be changed by using the `CONFIG_EXAMPLE_SETUP_CODE` config option (`idf.py menuconfig -> Example Configuration -> HomeKit Setup Code`). Please also refer the [HomeKit Setup Configuration](#homeKit-setup-configuration) below.


## Using Standard Apple HomeKit Profiles
The examples provided in the SDK are for fan, outlet, lightbulb. For using any other standard profiles, please refer the files under `components/homekit/hap_apple_profiles/`.

## Adding Custom Profiles
Please refer the Firmware Upgrades Custom Profile (`components/homekit/extras/`) to see how to add your own profile. You can use some UUID generator like [this](https://www.uuidgenerator.net/) to generate your own UUIDs for the services and characteristics.
 
## HomeKit Setup Configuration

As per the HomeKit requirements, the raw setup code should not be stored on the accessory. Instead, the SRP Salt and Verifier generated from the code should be stored. This salt-verifier combination, called as setup info in the ESP HomeKit SDK, needs to be unique for every accessory. This can be provided to the HAP Core using the `hap_set_setup_info()` API. Another parameter required for setup is the setup id, which also needs to be unique. It is provided to the HAP Core using `hap_set_setup_id()` API. 

In order to ease the task of providing unique setup info and setup id for each accessory, instead of using the above APIs, the same information can be stored in a separate flash partition called factory\_nvs (Please see the `partitions_hap.csv` file in any example). This allows to keep the same firmware image and just flash different factory\_nvs images for different accessories. The factory\_nvs image fan\_factory.bin has been provided for Fan, which can be flashed using esptool. Custom images can be created using the `factory_nvs_gen` utility in tools/ as below

Usage:

```
./factory_nvs_gen.py 11122333 ES32 factory
```

> Replace ES32 and 11122333 with the setup code and setup id of your choice. This has been tested with **Python 3.7.3**.

Flash this using the esptool as below.

```text
$ esptool.py -p $ESPPORT write_flash 0x340000 factory.bin
```

To use the setup info from this image, you will have to disable the hardcoded setup code by setting `CONFIG_EXAMPLE_SETUP_CODE=n` (`idf.py menuconfig -> Example Configuration -> Use hard-coded setup code -> disable`). 

The factory\_nvs partition can also hold other information like serial numbers, cloud credentials, firmware verification keys, etc. For that, just create a CSV file as per the requirements of [NVS Partition Generator Utility](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/nvs_partition_gen.html#) and provide it to the `factory_nvs_gen` using the `--infile` option. These variables (if stored as binary data) can then be read in the accessory code using `hap_factory_keystore_get()` API.

Sample CSV (say sample.csv)

```
key,type,encoding,value
app,namespace,,
serial_num,data,binary,12345
```

Usage:

```
./factory_nvs_gen.py --infile sample.csv 11122333 ES32 factory
```

Flash this using the same command given above.

```text
$ esptool.py -p $ESPPORT write_flash 0x340000 factory.bin
```

## Additional MFi requirements

If you have access to the MFi variant of esp-homekit-sdk, please check additional information [here](MFI_README.md).

## Reporting issues

You can report issues directly on [Github issues](https://github.com/espressif/esp-homekit-sdk/issues).
