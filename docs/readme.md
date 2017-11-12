# Plantstation2 Logging with Logstash

## Background Info
Last time I fiddled with Arduino, I built <a href="https://mtalavera.wordpress.com/2015/09/13/maker-fair-orlando-plantstation-slides/" target="_blank">Plantstation</a>, a system to 'care' for plants.  

<img alt="Plantstation" src="https://raw.githubusercontent.com/mariotalavera/plantstation/master/docs/images/plantstation1.png" width="50%">

It kept track of ambient and soil temperature.  It also kept track of soil moisture, humidity and ambient light levels.  Based on this information, it used relays to manage both irrigation and artificial lights.  

<img alt="Chocolate Mint" src="https://raw.githubusercontent.com/mariotalavera/plantstation/master/docs/images/planstation2.png" width="50%">

Lastly, it had a real-time dashboard providing an overview of current and past conditions.  
<img alt="Keen Dashboard" src="https://raw.githubusercontent.com/mariotalavera/plantstation/master/docs/images/keen-dashboard.png" width="70%">

This was my introduction into the arduino ecosystem and the kids and I even showcased it at out local Makerfaire.

## Getting Back
After Makerfaire was over, however, the basil went wild (and consumed) and the arduino went into a box.  This time around, I’ve decided to reconfigure the arduino device to monitor a hydroponic station instead, Plantstation2.  

<img alt="Plantstation" src="https://raw.githubusercontent.com/mariotalavera/plantstation/master/docs/images/plantstation3.png" width="50%">

A slightly different sensor array will be deployed but there will be no relays to control anything.

Here is a brief overview of the mayor steps needed to collect all the sensor data for later processing.

### Arduino
The first step was to revise the arduino code to send the sensor readings using a POST request. 

```cpp
#include <ArduinoJson.h>
String raw_payload;

IPAddress server(192,168,1,5);
YunClient client;

void send_log() {
  Console.println("connecting...");

    if (client.connect(server, 8383)) {
    Console.println("connected");
    raw_payload = "{\"APPID\": \"" + appId + "\", \"LOCATION\": \"" + location + "\", \"TEMP_AMBIENT_AIR_F\": \"" + am.tempAirF + "\", \"TEMP_PIPE_AIR_F\":  \"" + am.tempAirThermsistorF + "\", \"TEMP_WATER_01_F\":  \"" + wm.tempWaterOneF + "\", \"TEMP_WATER_02_F\":  \"" + wm.tempWaterTwoF + "\", \"TEMP_WATER_03_F\":  \"" + wm.tempWaterThreeF + "\", \"HEAT_INDEX\": \"" + am.heatIndex + "\", \"HUMIDITY\": \"" + am.humidityAir + "\", \"LIGHT_PERCENT\":  \"" + lightPercent + "\", \"LIGHT_VALUES\": \"" + lightResistorVal + "\", \"IS_LED_ON\":  \"" + ledIsOn + "\"}";
    client.println("POST /plant_logger/plantstation2log/ HTTP/1.1");
    client.print("Content-length:");
    client.println(raw_payload.length());
    client.println("Connection: Close");
    client.println("Host:192.168.1.5");
    client.println("Content-Type: application/json; charset=UTF-8");
    client.println();
    client.println(raw_payload);
    Console.println("------------------------");
    Console.println(raw_payload);
    Console.println("------------------------");
    client.stop();
  } else {
    Console.println("connection failed");
    Console.println(server);
  }
```

This request is made once a minute to coincide with the current sensor reading interval.  This reading interval was picked randomly  and all the details are in the arduino code in the repo.

### Logstash
The endpoint is a <a href="https://www.elastic.co/products/logstash" target="_blank">Logstash Server</a> hosted on a local computer.  Using Logstash, gathering this information and sending it anywhere we want is very simple.  Along the way, I also fetch local weather data to enrich this data.

Logstash processing pipelines are separated into **intputs**, **filters** and **outputs**.  

#### Logstash Input
1. Both the sensor data request from Arduino and the weather information request (from Wunderground) are defined in the input section of the Logstash configuration file.

<img alt="Logstash Input" src="https://raw.githubusercontent.com/mariotalavera/plantstation/master/docs/images/input.PNG" width="70%">

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
2. Additionaly, a third input is defined which fetches information from a database server at a set interval.  The reason for this will become clear once we discuss the output for Amazon Web Services shorlty.

#### Logstash Filter
The filtering section usually contains the processing, if any, needed in the data pipeline.  In this case, only two actions are taken.
1. The incoming weather data, which comes in a json object, is parsed for information of interest.
2. A unique key is generated for later consumption.

<img alt="Logstash Filter" src="https://raw.githubusercontent.com/mariotalavera/plantstation/master/docs/images/filter.PNG" width="70%">

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
#### Logstash Output
For output, there are a few things going on.  
1. First, we take all the sensor data coming from the arduino device and log (insert) as a record in a table in both an MSSQL DB and a MySQL DB.  The only reason to log into separate database servers is to explore each product later on.
2. Next, when weather data comes in, this weather information is added to the existing sensor data recently logged.
3. Lastly, we take completed logs (records), those that have both sensor and weather data, and submit another POST request, this time to AWS for insertion into a DynamoDB table.

<img alt="Logstash Output" src="https://raw.githubusercontent.com/mariotalavera/plantstation/master/docs/images/output.PNG" width="70%">

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

### Table as Log
A simple table is used as a log both locally (on MSSQL and MySQL) and on AWS (DynamoDB).  In every case, the schema is the same.

plantstation2log | 
------------ | 
appid | 
location | 
temp_ambient_air_f  | 
temp_pipe_air_f | 
temp_water_01_f | 
temp_water_02_f | 
temp_water_03_f | 
heat_index | 
humidity | 
light_percent | 
light_values | 
is_led_on | 
date_added | 
temp_current_loc_f  | 
dew_point_f  | 
uv_index  | 
precipitation  | 
weather_text | 
humidity_current_loc |  
record_id | 

### DynamoDB 
The end of our pipeline is the DynamoDB version of this table as shown below.  
<img alt="Table on DynamoDB" src="https://raw.githubusercontent.com/mariotalavera/plantstation/master/docs/images/awslogs.PNG" width="70%">

This concludes the first step into this second iteration of arduino for me.  Having all the sensor data logged to AWS greatly expands the possibilities of where to go next.

### Analysis 
Not quite.  Previewing the information collected is very simple and can be used to get a general idea of future posibilities.

<img alt="Zeppelin Rocks" src="https://raw.githubusercontent.com/mariotalavera/plantstation/master/docs/images/zeppelin.PNG" width="70%">
