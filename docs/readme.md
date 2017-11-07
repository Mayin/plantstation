# Plantstation2 Monitoring with Logstash

## Summary
… what i am doing here
## Background Info
--- where I am coming from; highlights of previous (PS)
_image of last software stack_
… since then/ hydroponic system, ps2 is born
_hydro image_
## Overview
… go over the general system and define the focus of this post.
_general data flow thru logstash_
## Logstash
Brief about logstash.

Breaking logstash into input, filter and output.
Per each section, do 
snippet of code 
visual of pipe section.
Where it begins
Where it ends
… these are things like
Arduino code that sends data
Ms sql recipient
Mysql recipient
DynamodDB recipient

General output display for kicks using zeppelin
Where to from here, why

## Getting Back Into Arduino  
Last time I fiddled with Arduino, I had built <a href="https://mtalavera.wordpress.com/2015/09/13/maker-fair-orlando-plantstation-slides/" target="_blank">Plantstation</a>, a system to care for plants.  It kept track of ambient and soil temperature.  It also kept track of moisture, humidity and light levels.  Based on this information, it used relays to manage both irrigation and lights.  Finally, it had a real-time dashboard providing an overview of current and past conditions.  


This was my introduction into the arduino ecosystem and the kids and I even displayed it at out local Makerfaire.

https://drive.google.com/drive/folders/0B-zJHjL6eNEKZ2h5cFRON3h1OFk?usp=sharing

Recently, I have gotten interested again on Arduino devices.  
_here goes sentence about the future not getting here soon enough to devote all available time to this_ 

The resulting gizmo basically read a bunch of sensors at different intervals and used relays, and sensors, to control various functions.  Lastly, all this event information was being sent to Keen for creating some visualizations.

This time around, I’ve decided to reconfigure the arduino device to monitor a hydroponic station, Plantstation2.  A slightly different sensor array will be deployed and there will be no relays to control anything.  The focus of the (arduino) part will be to collect information in the system and its environment for later processing.

Using Logstash, gathering this information and sending it anywhere we want si rather easy.  Along the way, I fetch local weather data to enrich this data and, finally, write it out to our database(s) of choice.

Logstash is an ‘event processor’ that divides all possible tasks with events as either Inputs, Filters or Outputs.

![Logstash Input](https://lh5.googleusercontent.com/PG6_hjD5KVvw2QDGQg2LdAlSejCMq_fYp9jdEZKDWV-mqS35nh4x9wa85LY1m1s2EWswlDK7ApryRLSV1KiQ=w1920-h907-rw)

![Logstash Filter](https://lh6.googleusercontent.com/ZBh0PHCP-nTSUNAqpwtoM_MQw8jUUTYDsolN6752_BXJ1Qx_6H_UUhVzEnk6tOcOFeu_C5PWDW2yr9uuE9vE=w1920-h907-rw)

![Logstash Output](https://lh6.googleusercontent.com/XuyC67hYuQILoLNEAe6Woqo1bIhE79joSWKd3_9VafYqGOJwUWbjkPRsU-SMW0AxEdwy5LP1JN2WqI5ULNXE=w1920-h907-rw)

## Break effort by main areas:
1. Hardware
 - Arduino Mega (main)
 - Iduino Yun Shield (wifi module+)
 - Connector shield (coming soon)  Will allow better sensor connections to computer.
 - Sensors
 	 - Have now
 	 - In near future
 	
2. Software
 - Arduino software in Mega
 - Ilos
 	 - Logstash
 	 - Zeppelin
 - Lucy
 	 - MySQL DB
 	 - MSSQL DB
 - AWS
 	 - API Gateway
 	 - DynamoDB
  
3. Data Flow - Everything goes thru Logstash
 - Inputs
 	-- **http poller** Wunderweather API, Current conditions.
 	-- **http** Arduino HTTP POST. 
    -- **jdbc**
 	   - grab last completed record from db (mysql) 
 - Filters
 	-- **json** 'message' source defined 
 	-- **mutate** Parsing Wunderweather json
 - Outputs
 	-- **jdbc** 
 	    - Insert arduino measurements into databases (MSSQL and MySQL Servers)
 	    - Cleanup of funny business 
 	    - Update db records with current weather conditions
 	-- **http** 
 	    - Insert into aws dynamodb

4. Monitoring with Zeppelin
 - Sensor dashboard
 	- Temperature, air (including current conditions) and water
 	- Heat index
 	- Humidity (arduino and local)
 	- Light %, value
 	- is led on? - figure out if its dark from this
 	- dew point
 	- uv index
 	- precipitation
 	- weather text
 - Event dashboard
 	- Location, app id.
 	- Events
 		- Total
 		- Daily
 		- Hourly 

#### Notes
 - Mention Accuweather
 - have to fix pipe air temperature

