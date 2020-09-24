# HomeKit Emulator

This is a test example which can be used to emulate many standard HomeKit accessories. Set it up as you would do for any of the other examples, Once it boots up, use the console to try out various options

## Console

The prompt `esp32>` would appear on the serial console. Enter `help` to see all the available commands & their usage in the emulator.

### Available Commands

- `profile-list` : Lists all the supported profiles by this homekit sdk.
- `profile <profile-name>` : Entering a valid profile Id from the profile list will lead to the board emulating the accessory selected after rebooting.
- `read-char <aid>(optional) <iid>(optional)`: Without any arguments it prints the _characteristics and services of all the accessories_. With an aid it prints all the _characteristics and services pertaining to the given accessory._ With an aid and an iid, it will print the _given characteristic or all characteristics from the given service_.
- `write-char <aid> <iid> <value>` : Writes the value to the particular characteristic
- `reset` : Factory resets the device erasing the pairing and provisioning information.
- `reset-pairings` : Erases the pairing information from the device.
- `reset-network`: Erases the network credentials from the device.
- `reboot` : Reboots the accessory.
- `auth <authid>` : Sets the authentication mechanism (None, Hardware or Software auth).
