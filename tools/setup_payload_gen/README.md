# Setup Payload Generator

## Introduction
setup\_payload\_gen is a Python script that can be used to generate the Setup Payload required
for generating the QR code for HomeKit Accessory Setup

## Usage

```
~# ./setup_payload_gen.py <AccessoryCategory> <setupCode> <SetupID>
```

where

- *AccessoryCategory* is a number from the "Accessory Categories" table of HAP Spec. (Eg. 3 for fan)
- *setupCode* is the pairing code, without the hyphen (-). (Eg. 11122333)
- *SetupID* is a 4 character alphannumeric string configured for an accessory (Eg. 7OSX)

## Example

```
~# ./setup_payload_gen.py 7 51808582 7OSX
Setup Payload: X-HM://007JNU5AE7OSX
```

## Generating QR
Any standard QR code generator can be used for generating the QR code for
HomeKit Acessory Setup using the Payload generated above.

Eg. [http://www.qr-code-generator.com/](http://www.qr-code-generator.com/)
