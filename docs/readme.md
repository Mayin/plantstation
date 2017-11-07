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

Recently, I have gotten interested again on Arduino devices.  
_here goes sentence about the future not getting here soon enough to devote all available time to this_ 

The resulting gizmo basically read a bunch of sensors at different intervals and used relays, and sensors, to control various functions.  Lastly, all this event information was being sent to Keen for creating some visualizations.

This time around, I’ve decided to reconfigure the arduino device to monitor a hydroponic station, Plantstation2.  A slightly different sensor array will be deployed and there will be no relays to control anything.  The focus of the (arduino) part will be to collect information in the system and its environment for later processing.

Using Logstash, gathering this information and sending it anywhere we want si rather easy.  Along the way, I fetch local weather data to enrich this data and, finally, write it out to our database(s) of choice.

Logstash is an ‘event processor’ that divides all possible tasks with events as either Inputs, Filters or Outputs.


![alt text](https://lh5.googleusercontent.com/PG6_hjD5KVvw2QDGQg2LdAlSejCMq_fYp9jdEZKDWV-mqS35nh4x9wa85LY1m1s2EWswlDK7ApryRLSV1KiQ=w1920-h907-rw "Logstash Input")
### Logstash Input

```javascript
input {
  # Arduino computer send sensor data (every minute).
  http {
    type => "arduino-sensors"
    host => "192.168.1.5"
    port => 8383
    response_headers => {"Content-Type"=>"application/json"}
  }

  # Local weather data fetched (every five minutes).
  http_poller {
    type => "weather-local"
    urls=> {
      myurl => "http://api.wunderground.com/api/xxxxx/conditions/q/FL/Orlando.json"
    }
    # Every five minutes to comply with Wunderground free service terms.
    schedule => {cron => "*/5 * * * * UTC"}
  }

  # Fetching latest completed record from MySQL log table (every fixe minutes).
  jdbc {
    type => "reading-local-db"
    jdbc_driver_class => "com.mysql.jdbc.Driver"
    jdbc_connection_string => "jdbc:mysql://192.168.1.3:3306/plantstation"
    jdbc_user => "xxxxx"
    jdbc_password => "xxxxx"
    # Every five minutes to match weather update service.
    schedule => "*/5 * * * *"
    statement => "SELECT * FROM plantstation2log WHERE temp_current_loc_f IS NOT NULL ORDER BY date_added desc LIMIT 1"
  }
}
```

![alt text](https://lh6.googleusercontent.com/ZBh0PHCP-nTSUNAqpwtoM_MQw8jUUTYDsolN6752_BXJ1Qx_6H_UUhVzEnk6tOcOFeu_C5PWDW2yr9uuE9vE=w1920-h907-rw "Logstash Filter")
### Logstash Filter

```javascript
filter {

  # Defining source and type of incoming message.
  json {source => "message"}

  # If incoming is weather information, parse and define fields of interest.
  if [type] == "weather-local" {
    mutate {
      add_field => {"TEMP_CURRENT_LOC_F" => "%{[current_observation][temp_f]}"}
      add_field => {"HUMIDITY_CURRENT_LOC" => "%{[current_observation][relative_humidity]}"}
      add_field => {"DEW_POINT_F" => "%{[current_observation][dewpoint_f]}"}
      add_field => {"UV_INDEX" => "%{[current_observation][UV]}"}
      add_field => {"PRECIPITATION" => "%{[current_observation][precip_1hr_in]}"}
      add_field => {"WEATHER_TEXT" => "%{[current_observation][weather]}"}
    }
  }
	
  # If incoming is from arduino sensors, and a UUID.
  if [type] == "arduino-sensors" {
    uuid {
      target => "UUID"
    }
  }
}
```

![alt text](https://lh5.googleusercontent.com/ClBS7JjVsbbTydiLiraW0dK-S3kWMTKly6-4bzF9S_g8L7FwYY50gvoK3flW15YDtYTunJsNDiCvI8yzsrP0=w1920-h907-rw "Logstash Output")
### Logstash Output

```javascript
output {
  # If incoming is from arduino sensors, insert sensor data into MySQL and MSSQL tables.
  if [type] == "arduino-sensors" {
    jdbc {
      driver_class => "com.mysql.jdbc.Driver"
      connection_string => "jdbc:mysql://192.168.1.3:3306/plantstation?user=xxxxx&password=xxxxx"
      statement => [ "INSERT INTO plantstation2log (record_id, appid, location, temp_ambient_air_f, temp_pipe_air_f, temp_water_01_f, temp_water_02_f, temp_water_03_f, heat_index, humidity, light_percent, light_values, is_led_on, date_added, temp_current_loc_f, humidity_current_loc, dew_point_f, uv_index, precipitation, weather_text) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", "UUID" ,"APPID", "LOCATION", "TEMP_AMBIENT_AIR_F", "TEMP_PIPE_AIR_F", "TEMP_WATER_01_F", "TEMP_WATER_02_F", "TEMP_WATER_03_F", "HEAT_INDEX", "HUMIDITY", "LIGHT_PERCENT", "LIGHT_VALUES", "IS_LED_ON", "@timestamp", "TEMP_CURRENT_LOC_F", "HUMIDITY_CURRENT_LOC", "DEW_POINT_F", "UV_INDEX", "PRECIPITATION", "WEATHER_TEXT" ]
    }

    jdbc {
      driver_class => "com.microsoft.sqlserver.jdbc.SQLServerDriver"
      connection_string => "jdbc:sqlserver://192.168.1.3:1433;databaseName=plantstation;user=xxxxx;password=xxxxx"
      statement => [ "INSERT INTO plantstation2log (record_id, appid, location, temp_ambient_air_f, temp_pipe_air_f, temp_water_01_f, temp_water_02_f, temp_water_03_f, heat_index, humidity, light_percent, light_values, is_led_on, date_added, temp_current_loc_f, humidity_current_loc, dew_point_f, uv_index, precipitation, weather_text) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", "UUID" ,"APPID", "LOCATION", "TEMP_AMBIENT_AIR_F", "TEMP_PIPE_AIR_F", "TEMP_WATER_01_F", "TEMP_WATER_02_F", "TEMP_WATER_03_F", "HEAT_INDEX", "HUMIDITY", "LIGHT_PERCENT", "LIGHT_VALUES", "IS_LED_ON", "@timestamp", "TEMP_CURRENT_LOC_F", "HUMIDITY_CURRENT_LOC", "DEW_POINT_F", "UV_INDEX", "PRECIPITATION", "WEATHER_TEXT" ]
    }
  }
  
  # If incoming is weather information, enrich MySQL and MSSQL records with such.
  if [type] == "weather-local" {

    jdbc {
      driver_class => "com.mysql.jdbc.Driver"
      connection_string => "jdbc:mysql://192.168.1.3:3306/plantstation?user=xxxxx&password=xxxxx"
      statement => [ "UPDATE plantstation2log SET temp_current_loc_f = ?, humidity_current_loc = left(?, length(?)-1), dew_point_f = ?, uv_index = ?, precipitation = ?, weather_text = ? WHERE weather_text is null", "TEMP_CURRENT_LOC_F", "HUMIDITY_CURRENT_LOC", "HUMIDITY_CURRENT_LOC", "DEW_POINT_F", "UV_INDEX", "PRECIPITATION", "WEATHER_TEXT" ]
    }

    jdbc {
      driver_class => "com.microsoft.sqlserver.jdbc.SQLServerDriver"
      connection_string => "jdbc:sqlserver://192.168.1.3:1433;databaseName=plantstation;user=xxxxx;password=xxxxx"
      statement => [ "UPDATE plantstation2log SET temp_current_loc_f = CAST(? AS DECIMAL), humidity_current_loc = left(?, len(?)-1), dew_point_f = ?, uv_index = ?, precipitation = ?, weather_text = ? WHERE weather_text is null", "TEMP_CURRENT_LOC_F", "HUMIDITY_CURRENT_LOC", "HUMIDITY_CURRENT_LOC", "DEW_POINT_F", "UV_INDEX", "PRECIPITATION", "WEATHER_TEXT" ]
    }
  }

  # If a completed record (sensor and weather) has been fetched from MySQL table, send complete record to DynamoDB.
  if [type] == "reading-local-db" {
    http {
      url => "https://xxxxx.execute-api.us-east-1.amazonaws.com/001/plantstation2log"
      http_method => "post"
      message => '{
        "record_id":"record_id",
        "date_added":"date_added",
        "appid":"appid",
        "location":"location",
        "temp_ambient_air_f":"temp_ambient_air_f",
        "temp_pipe_air_f":"temp_pipe_air_f",
        "heat_index":"heat_index",
        "temp_water_01_f":"temp_water_01_f",
        "temp_water_02_f":"temp_water_02_f",
        "temp_water_03_f":"temp_water_03_f",
        "humidity":"humidity",
        "light_percent":"light_percent",
        "light_values":"light_values",
        "is_led_on":"is_led_on",
        "temp_current_loc_f":"temp_current_loc_f",
        "humidity_current_loc":"humidity_current_loc",
        "dew_point_f":"dew_point_f",
        "uv_index":"uv_index",
        "precipitation":"precipitation",
        "weather_text":"weather_text"
      }'
    }
  }
}
```

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

