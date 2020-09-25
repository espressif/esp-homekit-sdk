# Homekit AWS-IoT Integration

This example demonstrates the secure **AWS-IoT** communication for **HomeKit Accessories**.

## 1. Setup
 - Create a thing and generate certificates for the device from **AWS-IoT** and activate them for the thing.
 - Download and place the Certificate and Private Key in the **main/certs** directory in your project, renaming them as **device.crt** and **device.key**. The AWS-IoT root CA certificate has already been placed as server.crt in the same folder. This is the RSA 2048 bit key: Amazon Root CA 1 taken from [here](https://docs.aws.amazon.com/iot/latest/developerguide/managing-device-certs.html#server-authentication).
 - Attach an appropriate policy to the certificates to enable the accessing this thing.
 - Set the AWS IoT endpoint in `CONFIG_AWS_IOT_MQTT_HOST`. This can be found under Settings tab of the IoT Core section on AWS Dashboard. The value will be something like `xxxxxxxxxxxxx-ats.iot.us-east-1.amazonaws.com`
 - Set/change the Client ID using `CONFIG_AWS_EXAMPLE_CLIENT_ID` and also choose if you want to use Thing-Shadow or Publish-Subscribe.
 - If you choose Publish-Subscribe, set/change the Publish/Subscribe topics using `CONFIG_AWS_EXAMPLE_PUBLISH_TOPIC` and `CONFIG_AWS_EXAMPLE_SUBSCRIBE_TOPIC` respectively.
 - Build and Flash your firmware after making these changes.


> **Note:** Although the certificates are embedded into the firmware in this example, in production the certificates would normally be stored in NVS as binary blobs.

## 2. Testing
 - This code enables the accessory to report any local changes to AWS-IoT and also accept change requests from it.

### Thing-Shadow
 - If you are using Thing-Shadow, you can view the values reported by the accessory using the AWS Dashboard and going to IoT Core -> Manage -> Things -> \<Thing Name\> -> Shadow.
 - The shadow document looks something like this:

```
{
     "desired": {
         "on": false,
         "brightness": 28
     },
     "reported": {
         "on": false,
         "brightness": 28
     }
}
```

 - The reported values reflect the current state of the accessory.
 - If you want to request any changes, change the values in the "desired" object and save the new JSON document.

### Publish-Subscribe
 - Use the AWS IoT Core "Test" tab and subscribe to the topic to which the device is programmed to publish, in order to observe state changes.
 - To change the values from AWS-IoT, publish the following JSON to the topic subscribed by the device. 
```
{
    "on":false,
    "brightness":50
}
```