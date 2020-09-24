# Factory NVS Generator

## Introduction

factory\_nvs\_gen is a Python script that can be used to generate the Factory NVS Partition image with
accessory setup information like setup\_id, setup\_salt and setup\_verifier. Using such images, the same
firmware can be used for multiple accessories, as the setup information is no more a part of the
firmware, but read from a different partition.

Using this needs the factory\_nvs partition in partition table. Please check the partitions\_hap.csv 
file in the example accessories for reference. Using the same file for your accessory is recommended.

## Usage

This utility requires the python library "srp". If not available already, please install using

```
pip install srp
```

The Utility can then be used as below:

```
usage: ./factory_nvs_gen.py [-h] [--infile INFILE] [--size SIZE]
                            [--version {v1,v2}] [--keygen {true,false}]
                            [--encrypt {true,false}] [--keyfile KEYFILE]
                            setup_code setup_id outfile

Factory NVS Generator

optional arguments:
  -h, --help            show this help message and exit

To generate Factory NVS partition:
  setup_code            the input setup code without hyphens Eg.: 11122333
  setup_id              the input setup identifer Eg.: ES32
  --infile INFILE       the input csv file with extension(full path) to add
                        HomeKit data generated to this file Eg.: fan_data.csv
  outfile               the output csv and bin filename without extension Eg.:
                        fan_factory
  --size SIZE           Size of NVS Partition in bytes (must be multiple of
                        4096)
  --version {v1,v2}     Set version. Default: v2
  --keygen {true,false}
                        Generate keys for encryption. Creates an
                        `encryption_keys.bin` file. Default: false
  --encrypt {true,false}
                        Set encryption mode. Default: false
  --keyfile KEYFILE     File having key for encryption (Applicable only if
                        encryption mode is true)
```

where

- *setup\_code* is the pairing code, without the hyphen (-). (Eg. 11122333)
- *Setup\_id* is a 4 character alphanumeric string configured for an accessory (Eg. 7OSX)
- *target\_filename\_without\_extension* is a name like fan\_factory, to which .csv and .bin extensions are added for creating the files

## Example

```
~# ./factory_nvs_gen.py 11122333 ES32 fan_factory
CSV Generated: fan_factory.csv
NVS Flash Binary Generated: fan_factory.bin
```

```
./factory_nvs_gen.py 11122333 ES32 fan_factory --version v2 --encrypt true --keygen true
CSV Generated: fan_factory.csv
Version:  v2 - Multipage Blob Support Enabled
NVS binary created: /path/esp_homekit_sdk_src/tools/factory_nvs_gen/fan_factory.bin
Encryption keys binary created: /path/esp_homekit_sdk_src/tools/factory_nvs_gen/keys/encryption_keys_04-11_18-32.bin
NVS Flash Binary Generated: fan_factory.bin
```

## Using the Binary

As mentioned, earlier, using this requires the factory\_nvs partition in your partition table.
A `make` target has been added for flashing this, however it requires that

- The .bin image is present in your accessory folder (where you will execute make) with the name
    $(PROJECT_NAME)\_factory.bin (Eg. fan\_factory.bin)
- The appropriate value is set for CONFIG\_APP\_FACTORY\_NVS\_OFFSET, which is basically the offset for
    the factory_nvs partition in the partition table

If the above requirements are met, first flash all the components, including the partition table and then flash the NVS partition as given below:

```
make flash
make factory-nvs-flash
```

## Adding custom data

For certain applications, some custom NVS data may be required in addition to the HomeKit data.
Such data can be included by writing it into a CSV file, the format for which should be as per
the requirements of nvs\_partition\_generator. Please refer the documentation under tools/nvs\_partition\_generator
for details about formatting. The file with the custom data (say app.csv) should then be passed to
the factory\_nvs\_gen as below:

```
~# ./factory_nvs_gen.py 11122333 ES32 fan_factory --infile app_data.csv
CSV Generated: fan_factory.csv
NVS Flash Binary Generated: fan_factory.bin
```

*Note that the custom data cannot use hap\_setup as the namespace, unless it is the UUID/Token for Software Authentication*
