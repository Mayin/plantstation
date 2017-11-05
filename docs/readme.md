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

