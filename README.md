# govee-gateway
![GitHub last commit](https://img.shields.io/github/last-commit/w1gx/govee-ble-scanner?style=for-the-badge) ![Top language](https://img.shields.io/github/languages/top/w1gx/govee-ble-scanner?style=for-the-badge) ![Language count](https://img.shields.io/github/languages/count/w1gx/govee-ble-scanner?style=for-the-badge) ![Issues](https://img.shields.io/github/issues/w1gx/govee-ble-scanner?style=for-the-badge) ![GitHub release](https://img.shields.io/github/v/release/w1gx/govee-ble-scanner?style=for-the-badge)
>A BLE Gateway to MQTT/InfluxDB for Govee H5074 temperature sensors.

![terminalscreen](./img/govee-gateway.png)


## General info
The Govee Gateway uses a HCI connection to scan for packets of type HCI_EVENT_PKT (04) with event types of EVT_LE_META_EVENT and extracts all advertising data from them. It then looks for AD type identifiers of 0xFF (manufacturer data), and within those for the company identifier 0x88EC, which contain the encoded temperature and humidity.For more detail about this process please refer to the wiki and the Bluetooth Core Specification at https://www.bluetooth.com/specifications/bluetooth-core-specification/.
Depending on the configuration, the Govee Gateway sends temperature and humidity data in configurable intervals to MQTT and/or InfluxDB.

## Technology

The Govee BLE Gateway is written in C++ and tested on Raspberry Pis. Besides the standard libraries, it requires libbluetooth and - if MQTT is required - paho.mqtt.cpp.

It uses the following modules:
- INIReader (https://github.com/benhoyt/inih)
- influxdb-cpp (https://github.com/orca-zhang/influxdb-cpp)
- optional: paho.mqtt.cpp (https://github.com/eclipse/paho.mqtt.cpp)

It has two main classes: a BLEPacket, which stores all packet information and BLEScan, the scanner that looks for HCI_EVENT packets.

## Setup
To run the scanner, do the following:

	sudo apt-get install bluetooth bluez libbluetooth-dev
	make release MQTT=1
	sudo ./build/release/goveeBLE



Optionally the doxygen documentation can be created with

	sudo apt-get install doxygen graphviz
	make doc		


## Status
