# ESP ColdBoot v1.0

## Flexible ESP8266 and ESP32 initial device provisioning

 

### Background

When you deploy hundreds of devices as part of the IoT project, the question is: how to get them on customer's network and provision the latest specific firmware to the device. 

The issue is that you do not know where and when the device will be first booted up. 

The device would not know the WiFi parameters and its firmware may be outdated.

This is where **ColdBoot** comes to help. 

 


### What is ColdBoot?

ColdBoot is a pre-compiled firmware for ESP8266 or ESP32 microcontroller that is capable of quickly collecting:

- WiFi parameters (SSID and password) 
- URL of the latest configuration file
- URL of the OTA update server

from the end-user via a simple web-form. 

**ColdBoot's main objective** is to get to Wifi and provision the latest firmware for a particular device as quickly and easily as possible. 

ColdBoot provides step-by-step instructions via terminal window connected to USB or Serial port 
(115200 bps setting)




### How to use:

Option 1: Upload pre-compiled binaries directly to the chip

Option 2: Compile and upload provided sketch.

Optionally: edit *data/config.json* file and upload to SPIFFS using upload tool

 

### Process flow

#### STEP 1:  READ COFIGURATION FROM SPIFFS

ColdBoot tries to read a configuration file from the SPIFFS file system of the device. 

The configuration file should be named '*config.json*' and should reside in the root folder.

The config file is a simple JSON array and could define the following parameters:

- "Title" - title of the web form for collecting parameters from the user
- "SSID" - WiFi network SSID
- "Password" - WiFi password
- "Config" - URL to the configuration file if parameters are to be sourced from the web
- "OTA" - URL to the OTA firmware provisioning HTTP server
- "fields" - number of fields to be shown on the web form   

All or a subset of fields could be defined. Subsequent process flow could be influenced by this file.

##### An example use case: 

Configuration file provides a Title for the web-form, a URL for latest configuration, and limits number of web-form fields to 2 (SSID and password)

```json
{
  "Title"    : "My Amazing IoT device",
  "Config"   : "http://ota.home.lan/esp/config/",
  "fields"   : "2"
}
```

**Note**: To use SPIFFS configuration capability - edit the provided *data/config.json* file to your needs and upload SPIFFS image to the device using ESP8266 SPIFFS Upload Tool in the Arduino IDE. 

 

#### STEP 2:  CONNECT TO WIFI BASED ON SPIFFS CONFIGURATION

If reading configuration from the SPIFFS was successful, there is a chance an SSID and password were provided there, so ColdBoot attempts to connect to WiFi. 

If SPIFFS was not available **or** *config.json* file was not available, the step 2 is skipped. 

 

#### STEP 3:  PROVISION CONFIGURATION FROM THE USER

If device was able to get onto the WiFi network as part of step 2, this step is skipped. 

As this point, device creates a WiFi AP point and creates a simple web-form in order to collect configuration parameters from the user.  AP SSID is **ESP8266-\<device-id\>** or**ESP32-\<device-id\>**. 
(Example: ESP8266-dac26e)

End user needs to connect to the AP and navigate to http://10.1.1.1

![Windows 10 Network Connect Example](https://github.com/arkhipenko/EspColdBoot/blob/master/pictures/ap.png)

 

A full web-form would look like this:

![](https://github.com/arkhipenko/EspColdBoot/blob/master/pictures/form.png)

A reduced web-form based on the step 1 example *config.json* file would look like this:

![](https://github.com/arkhipenko/EspColdBoot/blob/master/pictures/form-reduced.png)



Once user hits Submit button, device attempts to connect to WiFi again and again until successful. 

Device will reboot after 10 minutes of inactivity.

  

#### STEP 4:  READ CONFIGURATION FROM HTTP SERVER

In case a configuration URL is provided, device will attempt to read and parse configuration parameters from the http server. 

There are two choices here:

- You can provide a path to the server where specific configuration files are stored per each device, or
- you can provide a path to a configuration file directly (in this case a multiple devices could read the same file) 

If only a folder path is provided, **ColdBoot** will add device ID and version, so configuration requested is presumed to be unique. E.g., for device id ESP8266-dac26e, the URL will look like:

​		http://ota.home.lan/esp/config/coldboot-dac26e-1.0.0.json

(provided http://ota.home.lan/esp/config/ is your configuration server)

Alternatively, you can specify a direct file URL like:

​		http://ota.home.lan/esp/config/iotdevice.json

If **ColdBoot** determines that a valid URL was not provided, this step is skipped as **optional**. 

 

#### STEP 5: UPDATE FIRMWARE FROM AN OTA SERVER

At this point ColdBoot assumes there is a valid URL to an OTA update server available. 

Device will attempt to perform OTA firmware update based on the provided URL. 

There are two choices here:

- You can provide a path to the server where specific binary files are stored per each device, or
- you can provide a path to a binary file directly (in this case a multiple devices could update based on the same file) 

If only a folder path is provided, **ColdBoot** will add device ID and version, so OTA requested is presumed to be unique. E.g., for device id ESP8266-dac26e, the URL will look like:

​		http://ota.home.lan/esp/bin/coldboot-dac26e-1.0.0.bin

(provided http://ota.home.lan/esp/bin/ is your OTA server URL)

Alternatively, you can specify a direct file URL like:

​		http://ota.home.lan/esp/bin/iotdevice.bin


 
#### STEP 6: REBOOT

If everything goes well, step 6 should not be reached as device will reboot at the end of successful OTA update at step 5. 

However, if OTA update fails, device will wait for 10 seconds and reboot.

 

### FOLDERS:

**data** - SPIFFS image to be uploaded to the device if provisioning via SPIFFS is desired. Must contain a JSON file called *config.json* in the root folder. 

**binary** - a pre-compiled esp8266 binary (assuming 4Mb Flash size, 2Mb SPIFFS size, 1Mb OTA size) which could be flashed directly to the chip as part of the manufacturing process


 
### EXAMPLE:

Below is a terminal output of the real device being provisioned via ColdBoot.

Please note the application ID changes to **success-...** after OTA is done. 

```
ESP8266 ColdBoot v1.0.0

ESP8266 ID: ESP8266-dac26e

Full Application Version: coldboot-dac26e-1.0.0
---------------------------------------------

STEP 1:

 > Reading configuration file from SPIFFS if present
 > SUCCESS.

STEP 2:

 > Configuration read from SPIFFS:
 > Title	:	ColdBoot Initial Config
 > SSID	:	<your wifi ssid>
 > Password	:	<your wifi password>
 > Config	:	http://ota.home.lan/esp/config/
 > OTA	:	<path to OTA firmware>
 > fields	:	2
 > Current count = 6
 > Current size  = 157

 > Attempting WiFi connection ............................................................ WiFi connection timeout
 > WiFi connection unsuccessful.


STEP 3:

 > Provisioning configuration from the user

	Navigate to  http://10.1.1.1  and fill out the form
	Provide WiFi SSID and password
	as well as URLs for Configuration and/or OTA

 > Bootstrapping SUCCESS.
 > Configuration after Bootstrap:
 > Title	:	ColdBoot Initial Config
 > SSID	:	devices
 > Password	:	********
 > Config	:	http://ota.home.lan/esp/config/
 > OTA	:	<path to OTA firmware>
 > fields	:	2
 > Current count = 6
 > Current size  = 142

 > Attempting WiFi connection .............	WiFi connected
 > IP address: 192.168.122.65
 > SSID: devices
 > mac: 98:F4:AB:DA:C2:6E

STEP 4:

 > Checking if configuration should be read from an HTTP server
 > Attempting to read config from this URL:
   http://ota.home.lan/esp/config/coldboot-dac26e-1.0.0.json
 > SUCCESS.
 > Dictionary after HTTP config:
 > Title	:	ColdBoot Initial Config
 > SSID	:	devices
 > Password	:	********
 > Config	:	http://ota.home.lan/esp/config/
 > OTA	:	http://ota.home.lan/esp/bin/
 > fields	:	2
 > Current count = 6
 > Current size  = 148


STEP 5:

 > Checking for the OTA provisioning URL
 > Attempting OTA Update from this URL:
   http://ota.home.lan/esp/bin/coldboot-dac26e-1.0.0.bin

 ets Jan  8 2013,rst cause:2, boot mode:(3,7)

load 0x4010f000, len 3456, room 16 
tail 0
chksum 0x84
csum 0x84
va5432625
@cp:0
ld
ColdBoot v1.0.0

ESP8266 ID: ESP8266-dac26e
AppVersion: success-dac26e-1.0.0
```

