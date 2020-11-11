# LilyGO TTGO T-HiGrow
## TTGO T-HIGrow MQTT autodiscover interface for Homeassistant

![](https://github.com/pesor/TIGO-T-HIGrow/blob/master/images/T-Higrow.jpg)

<a href="https://github.com/pesor/TIGO-T-HIGrow/releases"><img src="https://img.shields.io/github/v/release/pesor/TIGO-T-HIGrow?style=plastic"/></a> <a href="https://github.com/pesor/TIGO-T-HIGrow/blob/master/LICENSE"><img src="https://img.shields.io/github/license/pesor/TIGO-T-HIGrow?style=plastic"/></a>  <a href="https://github.com/pesor/TIGO-T-HIGrow/stargazers"><img src="https://img.shields.io/github/stars/pesor/TTGO-T-Higrow?style=plastic"/></a>  <a href="https://github.com/pesor/TTGO-T-Higrow/releases"><img src="https://img.shields.io/github/downloads/pesor/TTGO-T-Higrow/total?style=plastic"/></a>

### Getting started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. See deployment for notes on how to deploy the project on a live system.

### Prerequisites

What things you need to install the software and how to install them

1. LilyGo T-Higrow V1.1 (available from AliExpress)  (make sure it is V1.1 19-8-22 or later)

2. Windows 10, with installed Arduino EDI (my version 1.8.12)

3. USB Cable with USB-C to attatch to the LilyGo

4. MQTT server (I am running on a Synology NAS in docker)
   If you have a Synology NAS, I can recommend to follow [BeardedTinker](https://www.youtube.com/channel/UCuqokNoK8ZFNQdXxvlE129g) on YouTube, he makes a very intuitive explanation how to setup the whole environment on Synology.   

   [](https://https://www.youtube.com/channel/UCuqokNoK8ZFNQdXxvlE129g)

### Installing

Below a step by step that tell you how to get a development/production environment up and running, and to make things even more easy,  [BeardedTinker](https://www.youtube.com/channel/UCuqokNoK8ZFNQdXxvlE129g)  have created the two tutorials on YouTube, which gives a detailed instruktion how to get it all to work. 

He has performed a tremendous task in doing this.

First video is how to setup the whole environment: https://www.youtube.com/watch?v=7w6_ZkLDxko&t=231s

I highly recommend that your see and follow these two videos, as then you only will have success in setting this up.

If you have many sensors and choose to use the autodiscover function, then in the video look away from the updating of the configuration.yaml file, as this is not needed when using Autodiscover. If you only have one or two modules, it might be easier just to update the configuration.yaml file.

After seeing the videos, remenber to give a "Thumbs Up" to support BeardedThinker in his work.

### The INO Part - The Arduino Sketch

The main program here is the:

​			**TTGO-HiGrow-mqtt-master-mac-id-autodisc.ino**

You just use this .ino as a master, and upload it to every  *LILYGO TTGO T-Higrow* module you have.

Few things of importants:

 1. First identify the ***// Start user defined data*** in the sketch

 2. If you have problems in seeing problems where the module is situated, you can activate the logging on the board it self, by setting the variable **logging = true**. (Default is false)

 3. Then you have to tell which DHT sensor you have on your module, by uncomment the ***#define DHT_TYPE variable*** which matches your sensor, and comment the others out.

 4. This step is **important**, you need to calibrate the sensor for the *SOIL HUMIDITY*, just follow the description in the program, and set the variables: **soil_min and soil_max** to the measured values. When you have the values right and placed in the sketch, you update them on the board by setting **calibrate_soil = true**, and when uploaded and run once, you set it back to false.

 5. The variables for the **SALT, aka Fertilizer**, is set, and you do not need to change them, unless you have very special soil conditions.1

 6. Now you give the module a plant name. Check the variable update_plant_name, if set to true, the plant name is stored on the module, after that you set the update_plant_name to false. If you want to use the module on another plant, you just repeat the step "true/false".

 7. Now you have to define your SSID's, you can have as many as you like, I have at the moment 4, probably going to 5 soon. You update the variable **ssidArr** with your access points, each separated by a comma. The variable **ssidArrNo** must be filled with the number of SSID's given.

 8. You then gives the Password for your SSID's (expected to all have the same).

 9. You now adjust to your time zone, by giving the numbers of hours multiplied by 3600.

 10. The **device_name**, and the **next two variables**, you should **not** change.

 11. The last thing to do, is to give in the information for your MQTT broker.

     

     Upload your sketch to the module, and

     ​																															**YOU ARE DONE with first part**

     

## The Python Part - The Autodiscover - MAGIC

In home-assistant/custom_components, you place the Python file, and create folder sensors, where you place the sensors.yaml file.

The sensors.yaml file will containe the identified modules, with their MAC_ID, and on next line the Plant name you have given to the module.

The sensors.yaml (in my case) look like this:

```
info: This file contains the MAC_ID of the sensors already registred for Autodiscover
info: You can reinitiate Autodiscover for a single MAC_ID, by removing it from the list, or you can initiate a full renewal of sensors to Autodiscover,
info: By removing all MAC_IDs below these comments lines
info: DO NOT REMOVE THESE FOUR INFO LINES.
mac_id: 246f28b2107c
name: Cattleya
mac_id: 3c71bff17118
name: Begonie_1_winter
mac_id: 3c71bff16b30
name: Padron_2_winter
mac_id: 3c71bff16cb8
name: Padron_1_winter
mac_id: 3c71bff17054
name: Begonie_2_winter
mac_id: fcf5c40cf614
name: Begonie_3_winter
```

**NEVER remove the four first info lines, as it will make the Autodiscover mailfuction.**

The Python script named: 

### 																		**TTGO-T-HIGrow-aut.py**

is unfortunately not an integrated part in Home-Assistant. There is (in my case) a reason for this. Home-Assistant does not accept the Import command in scripts. As I am using functions which do not exists in Home-Assistant as services, I need to import, and thus, the program has to run outside Home-Assistant.

I therefore run this program in Windows 10, and I do it in the Eric API for Python. The good thing is that you only need to run this program, when you add new modules. When it has run, it have made the module able to be Autodiscovered by Home-Assistant MQTT Autodiscover function.

You can always reinstall the module in Home Assistant, by deleting it in Integrations MQTT, and delete the mac_id and name in the sensors.yaml file, run the Python program, and voila, the sensors are back in Home Assistant MQTT.

## [Battery StateCard](https://github.com/maxwroc/battery-state-card)

In release 3.0.4 I have made changes, so that the aboce Battery State Card, which is part of the HACS custom cards, can be utilized.

As I have many TTGO-T-HIGROW modules, it is most confortable to have all these modules battery states in one card on the Home Assistant.

It will look like this:

​												![](https://github.com/pesor/TIGO-T-HIGrow/blob/master/images/battery-state-card-view.JPG)

I did have some problems in understand the explanation on the Github belonging to the card, so I have included an example on how to get it to work here:



![](https://github.com/pesor/TIGO-T-HIGrow/blob/master/images/battery-state-card.JPG)



### Running

The LilyGo T-Higrow V1.1, will wake up every (in this case) hour and report status to the MQTT server, and at the same time it will be updated in Homeassistant. It will run for approx. 2 months on a 3.7V, 800mA, Lithium battery.

I have had a couple of units, which did not last that long. It turns out that one were using 5mA, and the other 14.6 mA when in sleep mode. They are not suitable for battery, and I have no clue why they consume this amount of power. The average consumption for my other boards are around 0.250 mA, which is according to factory specifications.

You can set the wakeup time as you want. Lower time higher battery consumption, Higher time lower battery consumption.

All sensors from the LILYGO TTGO T-Higrow V1.1 are updated, so you can easy include them in Homeassistant if you should wish so.

### Alarm for low soil humidity

You can make the Homeassistant give you an alarm for low Soil Humidity, you will have to add the following to your automations.yaml. (example), this is a manual example, but if you use the Autodiscover function, you can add Alarms by using the "Configuration/Automations" function in the control panel of Home-Assistant.

```yaml
- alias: 'nord_window_soil_8'
  trigger:
    platform: numeric_state
    entity_id: sensor.north_window_soil_8
    below: 30
    for:
      minutes: 30
  action:
  - service: notify.mobile_app_xxxxxxx_iphone
    data:
      message: 'North window plant 8, needs watering'

```

### Deployment

See instructions under **Prerequisites**

### Versioning

3.0.2 Major release update, introducing Home Assistant Autodiscover for MQTT

3.0.1 Implemented Home-Assistant MQTT Autodiscover, Salt calibration and advice included.

3.0.2 DST switch over now works

3.0.3 Small error corrections

3.0.4 Adapting to HACS frontend card: Battery State Card

3.0.5 The plant name, is now added to the modules SPIFFS file system, so that it part of the constant data of the module.


### Authors

* **Per Rose** 
* BeardedTinker (contributer)

### License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

### Acknowledgments

* **Xinyuan-LilyGO / TTGO-HiGrow**  [https://github.com/Xinyuan-LilyGO/TTGO-HiGrow]( https://github.com/Xinyuan-LilyGO/TTGO-HiGrow) 
* I changed the above from a WEB server to MQTT, with integration into Homeassistant



**If you like and use my program, then** 

​       [![BMC](https://www.buymeacoffee.com/assets/img/custom_images/white_img.png)](https://www.buymeacoffee.com/pesor)

**it will be appriciated.**




## 3D printed case for the sensor

In the following you will find instructions on how to download and use the casing for the LILYGO HI-Grow sensor.

### Prerequisites

What things you need to print the case:

1. FreeCad, available from https://www.freecadweb.org/downloads.php
2. A 3D printer
3. Some optic cable, like this from AliExpress: https://www.aliexpress.com/item/32495181964.html?spm=a2g0s.9042311.0.0.27424c4dFTciIW
4. A 5mm and 8mm drill, for aligning the holes for optic cable and charging plug.
5. A C-charging plug, eg. like this from AliExpress: https://www.aliexpress.com/item/4000634865136.html?spm=a2g0s.9042311.0.0.27424c4dmTM92r
6. A 800mA Lithium-ION battery, like this from AliExpress: https://www.aliexpress.com/item/33022823001.html?spm=a2g0s.9042311.0.0.27424c4dmElaxC
7. A tube of Aquarium Silicone, for making expecially outdoor sensors watertight.

Here are a series of images, showing the process of assemble the case after print.



![Case1](https://github.com/pesor/LILYGO-TTGO-T-Higrow/blob/master/images/Case-1.jpg?raw=true)

![Case1](https://github.com/pesor/LILYGO-TTGO-T-Higrow/blob/master/images/Case-2.jpg?raw=true)

![Case1](https://github.com/pesor/LILYGO-TTGO-T-Higrow/blob/master/images/Case-3.jpg?raw=true)

![Case1](https://github.com/pesor/LILYGO-TTGO-T-Higrow/blob/master/images/Case-4.jpg?raw=true)



![Case1](https://github.com/pesor/LILYGO-TTGO-T-Higrow/blob/master/images/Case-5.jpg?raw=true)

I have included the original file for the cases in FreeCad format.
