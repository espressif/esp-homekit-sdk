# Setup Payload Generator

## Introduction
setup\_payload\_gen is a Python script that can be used to generate the Setup Payload required
for generating the QR code for HomeKit Accessory Setup

## Usage

```
~# usage: setup_payload_gen.py <AccessoryCategory> <SetupCode> <SetupID> [<ProductData>]
```

where

- *AccessoryCategory* is a number from the "Accessory Categories" table of HAP Spec. (Eg. 3 for fan)
- *setupCode* is the pairing code, without the hyphen (-). (Eg. 11122333)
- *SetupID* is a 4 character alphanumeric string configured for an accessory (Eg. 7OSX)
- *ProductData* is the unique 16 characters long Product Data assigned by Apple for your product which can be copied from the MFi Portal (Eg. 1122334455667788)

## Example

```
~# ./setup_payload_gen.py 7 51808582 7OSX
Setup Payload Header Encoding: 007JNU5AE
Setup Payload Data Encoding: 7OSX
Setup Payload: X-HM://007JNU5AE7OSX
```
```
~# ./setup_payload_gen.py 7 51808582 7OSX 1122334455667788
Setup Payload Header Encoding: 0E8NKO6YU
Setup Payload Data Encoding: 7OSX0CZ067T6WCYA44AY9S
Setup Payload: X-HM://0E8NKO6YU7OSX0CZ067T6WCYA44AY9S
```


## Generating QR
Any standard QR code generator can be used for generating the QR code for
HomeKit Accessory Setup using the Payload generated above.

Eg. [http://www.qr-code-generator.com/](http://www.qr-code-generator.com/)
