# HomeKit Manufacturing Utility

## Introduction

HomeKit Specifications require that each accessory should have a unique setup id and setup code, which is generated randomly. Moreover, the setup code cannot be stored directly on the accessory, and instead the salt-verifier combination for the given code must be stored. The ESP HomeKit SDK has a concept of factory\_nvs partition, which can hold this information, so that the same firmware can be used across multiple instances of an accessory. Only the factoy\_nvs partition image will change. Some accessories may also require some additional information like firmware decryption/verification keys (for Firmware Upgrades), serial number, non-iOS provisioning/control, password, etc. which can be a part of the factory\_nvs.

One important consideration here is how to create such individual images for larger number of accessories. This utility called as HomeKit Manufacturing Utility can be used to meet this requirement. It can be used for 2 different use cases:

1. HomeKit-only Data
2. HomeKit + Custom Data

## Prerequisites

* Operating System requirements:
	-	Linux / MacOS / Windows (standard distributions)

* The following packages are needed for using this utility:
	-	'srp' package.
		-	Command to install: ``pip install srp``
	-	Python version: 2.7 (minimum) is required.
		-	Link to install python: <https://www.python.org/downloads/>


**Note::** Make sure the python path is set in the PATH environment variable before using this utility.



## Usage

The HomeKit Manufacturing Utility is a python script. If run with the -h option, it will give the usage information as follows:

```
./hk_mfg_gen.py [-h] [--conf CONFIG_FILE] [--values VALUES_FILE]
                     [--prefix PREFIX] [--fileid FILEID] [--outdir OUTDIR]
                     [--cid CID] [--count COUNT] [--size PART_SIZE]
                     [--version {v1,v2}] [--keygen {true,false}]
                     [--encrypt {true,false}] [--keyfile KEYFILE]

Generate HomeKit specific data and create binaries and HomeKit setup info

optional arguments:
  -h, --help            show this help message and exit
  --conf CONFIG_FILE    the input configuration csv file
  --values VALUES_FILE  the input values csv file (the number of accessories
                        to create HomeKit data for is equal to the data
                        entries in this file)
  --prefix PREFIX       the accessory name as each filename prefix
  --fileid FILEID       the unique file identifier(any key in values file) as
                        each filename suffix (Default: setup_code)
  --outdir OUTDIR       the output directory to store the files created
                        (Default: current directory)
  --cid CID             the accessory category identifier
  --count COUNT         the number of accessories to create HomeKit data for,
                        applicable only if --values is not given
  --size PART_SIZE      Size of NVS Partition in bytes (must be multiple of
                        4096)
  --version {v1,v2}     Set version. Default: v2
  --keygen {true,false}
                        Generate keys for encryption. Creates an `encryption_keys_<timestamp>.bin` file
                        (in `keys/` directory in current path). Default: False
  --encrypt {true,false}
                        Set encryption mode. Default: False
  --keyfile KEYFILE     File having key for encryption (Applicable only if
                        encryption mode is true)

```

As mentioned earlier, this utility can be used for 2 different use cases:

### HomeKit-only Data

This is the simplest use case, wherein no custom data is required and the requirement is to just create random setup ids and setup codes. As an example, here is the usage to generate 5 such factory-nvs images for a Fan accessory:

```
mkdir images
./hk_mfg_gen.py --prefix Fan --count 5 --outdir images --cid 3 --size 0x3000
```

**Note**:: To generate images with HomeKit-only data --count, --prefix --cid, and --size arguments are mandatory.

Output

```
Version:  v2 - Multipage Blob Support Enabled
Binary created.
CSV Generated:  images/csv/Fan-985-05-005.csv
NVS Flash Binary Generated:  images/bin/Fan-985-05-005.bin
Version:  v2 - Multipage Blob Support Enabled
Binary created.
CSV Generated:  images/csv/Fan-344-47-290.csv
NVS Flash Binary Generated:  images/bin/Fan-344-47-290.bin
Version:  v2 - Multipage Blob Support Enabled
Binary created.
CSV Generated:  images/csv/Fan-946-46-242.csv
NVS Flash Binary Generated:  images/bin/Fan-946-46-242.bin
Version:  v2 - Multipage Blob Support Enabled
Binary created.
CSV Generated:  images/csv/Fan-521-88-048.csv
NVS Flash Binary Generated:  images/bin/Fan-521-88-048.bin
Version:  v2 - Multipage Blob Support Enabled
Binary created.
CSV Generated:  images/csv/Fan-875-45-449.csv
NVS Flash Binary Generated:  images/bin/Fan-875-45-449.bin
HomeKit config data generated is added to file:  images/homekit_config_10-30_22-58.csv
HomeKit values data generated is added to file:  images/homekit_values_10-30_22-58.csv
HomeKit Setup Info File Generated:  images/homekit_setup_info/Fan-985-05-005.txt
HomeKit Setup Info File Generated:  images/homekit_setup_info/Fan-344-47-290.txt
HomeKit Setup Info File Generated:  images/homekit_setup_info/Fan-946-46-242.txt
HomeKit Setup Info File Generated:  images/homekit_setup_info/Fan-521-88-048.txt
HomeKit Setup Info File Generated:  images/homekit_setup_info/Fan-875-45-449.txt
```

This generates 5 images in images/bin/ which can be flashed on the factory NVS. The corresponding files in images/homekit\_setup\_info give the setup code and setup payload, which can be used to prepare the stickers to be pasted on the physical HomeKit accessory.
The version used to create the images is also displayed. For further details refer to README of tools/nvs\_partition\_utility.

The configuration file \(homekit\_config\_10-30_22-58.csv\) generated gives information of all the NVS variables and their data types. The various values generated are stored in the values file (homekit\_values\_10-30\_22-58.csv). The values file can be considered as a master database as it has all the information for all the images, including metadata like the setup\_code used to generate setup\_salt and setup\_verifier, the value of setup\_id and even the setup\_payload which can be used to generate the QR code for accessory setup. Of these, only setup\_id, setup\_salt and setup\_verifier are actually used to generate the images. This logic is decided by the configuration file mentioned above, as it has entries only for these.


### HomeKit + Custom Data
If some custom data is required, a user will have to write 2 files:
#### CSV Configuration File:
This file contains the configuration of the NVS data for the accessories to be manufactured.

The data in configuration file **must** have the following format:

	name1,namespace,	   <-- First entry should be of type "namespace"
	key1,type1,encoding1
	key2,type2,encoding2
	name2,namespace,
	key3,type3,encoding3
	key4,type4,encoding4

**Note::** First entry in this file should always be ``namespace`` entry.

Each row should have these 3 parameters: ``key,type,encoding`` separated by comma.

*Please refer to README of tools/nvs\_partition\_utility for detailed description of each parameter.*


Below is a sample example of such a configuration file:


	app,namespace,
	firmware_key,data,hex2bin
	serial_no,data,i32


**Note::** Make sure there are no spaces before and after ',' or at the end of each line in the configuration file.

--

#### Master CSV Values File:
This file contains information of each device to be manufactured. Each row in this file corresponds to a device instance.

The data in values file **must** have the following format:

	key1,key2,key3,.....
	value1,value2,value3,....

**Note::** First row in this file should always be the keys. The keys must exist in this file in the **same order** as present in the configuration file. This file can even have additional columns(keys) anywhere in between and they will act like metadata and would not be part of final binary files.

Each row should have the ``value`` of the corresponding keys, separated by comma. Below is the description of this parameter:

``value``
	Data value.

Below is a sample example of such a values file:

	id,firmware_key,serial_no
	1,1a2b3c4d5e6faabb,101
	2,1a2b3c4d5e6fccdd,102

--

**Note::** *Intermediate CSV files are created by this utility which are input to the nvs partition utility to generate the binary files.
The format of this intermediate csv file will be:*

	key,type,encoding,value
	key,namespace, ,
	key1,type1,encoding1,value1
	key2,type2,encoding2,value2

The utility can then be run as follows:

```
$ ./hk_mfg_gen.py --conf sample_config.csv --values sample_values.csv --prefix Fan --cid 3 --outdir images --size 0x3000
```

**Note::** To generate images with HomeKit + Custom data --conf, --values, --prefix --cid, and --size arguments are mandatory. 

The Manufacturing utlity will add HomeKit related data (setup id, salt, verifier, etc.) to the configuration and master values file and create the individual binaries.

--

**Note::** *``bin/``, ``homekit_setup_info/`` and ``csv/`` sub-directories are created in the ``outdir`` directory specified while running this utility. The binary files generated will be stored in ``bin/``, information to be printed on accessories will be available in homekit\_setup\_info and the intermediate CSVs generated will be saved in csv/.*

*If you want the files to have a custom identifier, say the serial number, you can provide it as the fileid argument. As an example, running the utility as below will generate files with serial number as the suffix:*

```
./hk_mfg_gen.py --conf sample_config.csv --values sample_values.csv --prefix Fan --cid 3 --outdir images --fileid serial_no --size 0x3000
```

For the input values given in the examples above, files Fan-101.bin and Fan-102.bin will be generated.


*If you want to use encryption mode you can run the utility as given below:*

```
./hk_mfg_gen.py --conf sample_config.csv --values sample_values.csv --prefix Fan --cid 3 --outdir images --fileid
serial_no --size 0x3000 --encrypt true --keygen true
```

```
./hk_mfg_gen.py --prefix Fan --count 5 --outdir images --cid 3 --size 0x3000 --encrypt true --keygen true
```

This will generate encryption keys and generate encrypted binaries.

**Note::** *You can also provide --keyfile argument, generated encrypted binaries will expect this --keyfile file to have encryption keys.*

**Note::** The --outdir directory is created if not present. 

**Note::** *You can also run the below command to use the utility to* **only** *generate encryption keys binary file (created in `keys/`), which can further be used to encrypt the images:*
    
    $ ./hk_mfg_gen.py --keygen true
    
    $ ./hk_mfg_gen.py --keygen true --outdir tmp --keyfile encr_keys.bin

**Note::** When running utility to generate only ``keys``, if --keyfile is given it will generate encryption keys with filename given in --keyfile argument. ``keys/`` directory is generated with the encryption keys filename of the form ``prefix-fileid-keys.bin``.

**Note::** The file path given in the ``file`` type in the values file is expected to be relative to the current directory from which you are running the utility.

**Note::** *Make sure the parameters set are consistent with application's configurations.*
