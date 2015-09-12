  //===== General deployment defs =================
String appId = "Plant01";
//String location = "Dinning Room Window";
String location = "Living Room Side Table";

//===== For private keys, params ================
#include "params.h"

//===== For Keen integration ====================
#include "ApiClient.h"
#include "KeenClient.h"
KeenClient myKeen;

//===== For writing to the console =============
#include "Bridge.h"
#include "Console.h"

//===== For probe thermometer ===================
#include "OneWire.h"
const byte probePin = 3;  
OneWire myPT(probePin);

typedef struct my_soilmeasurement {
  float tempSoilC;
  float tempSoilF;  
} SoilMeasurement;
SoilMeasurement sm;

//===== For digital thermo/humidity sensor =====
#include "DHT.h"
const byte dhtPin = 2;
DHT myDHT(dhtPin, DHT11);

typedef struct my_airmeasurement {
  byte humidityAir;
  float heatIndex;
  float tempAirC;
  float tempAirF;
} AirMeasurement;
AirMeasurement am;

//===== For photoresistor =======================
byte lightPercent;
const byte lightPin = A0;
int lightResistorVal;

//===== For hygrometer ==========================
byte moisturePercent;
const byte hygroPin = A1;
const byte hygroPower = A15;
int moistureSensorVal;

//===== For moisture readings ====================
const int moistureMaxVal = 0; //255;
const int moistureMinVal = 950; //982; 
const byte moistureThreshold = 28; // ~ 326
String moistureStatus;

//===== For water level sensor ===================
byte waterLevelPercent;
const byte waterSensorPin = A3;
const byte waterSensorPower = A14;
int waterLevelVal;
String waterLevelStatus = "OK";

//===== For water pump ===========================
const byte waterPumpPin = A5;
// todo - find right value for this
const int waterLevelThreshold = 245;  
const int waterMaxVal = 386; 
const int waterMinVal = 80;   
boolean waterPumpIsOn = false;
unsigned long wateringSeconds = 15000;
unsigned long prevWatering = 0;

//===== For light switch =========================
const byte lightSwitchPin = 4;
const byte lightOnThreshold = 30;
const int lightMaxVal = 250;  
const int lightMinVal = 0; 
boolean lightIsOn = false;

//===== For timed events =========================
unsigned long prevMinOneMillis = 0;    
unsigned long minOneInterval = 1*60000;

unsigned long prevMinFiveMillis = 0;
unsigned long minFiveInterval = 5*60000;

unsigned long prevHourTwelveMillis = 0;
unsigned long hourTwelveInterval = 12*60*60000;

void setup() {
  pinMode(lightSwitchPin, OUTPUT);
  pinMode(waterPumpPin, OUTPUT);
  pinMode(hygroPower, OUTPUT);
  pinMode(waterSensorPower, OUTPUT);

  digitalWrite(waterPumpPin, LOW);

  myDHT.begin();
  Bridge.begin();
  Console.begin();    

  while (!Console);
  Console.print("\n PlantStation is warming up.\n\n 60 seconds to first event recording.\n\n");

  // Because we need initial moisture and water level readings
  digitalWrite(hygroPower, HIGH);
  digitalWrite(waterSensorPower, HIGH);
  delay(5000);
  readMoistureSensor();  
  readWaterLevelSensor();  
  digitalWrite(hygroPower, LOW);
  digitalWrite(waterSensorPower, LOW);
  // Because end
}

void loop() {
  unsigned long currentMillis = millis();

  // these happen every twelve hours (or so)
  if (currentMillis - prevHourTwelveMillis > hourTwelveInterval) {
    prevHourTwelveMillis = currentMillis;
    
    waterPlantStarts();
  }

  // these happen every five minutes (or so)
  if (currentMillis - prevMinFiveMillis > minFiveInterval) {
    prevMinFiveMillis = currentMillis;
    
    readMoistureSensor();  
    readWaterLevelSensor();  
    switchLights();
  }

  // these happen every minute (or so)
  if (currentMillis - prevMinOneMillis > minOneInterval) {
    prevMinOneMillis = currentMillis;

    readAirMeasurements();
    readSoilMeasurements();    
    readLightResistor();

    outputEvent();
    sendToKeen();  
  }
  
  waterPlantStops(currentMillis);  
}

void waterPlantStarts() {
  if (!waterPumpIsOn && waterLevelStatus == "OK" && moistureStatus == "Dry") {
    prevWatering = millis();
    waterPumpIsOn = true;
    digitalWrite(waterPumpPin, HIGH);
    Console.println("\n Watering started\n");
  }
}

void waterPlantStops(unsigned long currentMillis) {
    if (waterPumpIsOn && currentMillis >= prevWatering + wateringSeconds) {
    waterPumpIsOn = false;
    digitalWrite(waterPumpPin, LOW);
    Console.println("\n Watering stopped\n");    
  } else if (waterPumpIsOn && moistureThreshold < moisturePercent) {
    waterPumpIsOn = false;
    digitalWrite(waterPumpPin, LOW);
    Console.println("\n Watering stopped\n");
  } else if (waterPumpIsOn && waterLevelStatus == "LOW") {
    waterPumpIsOn = false;
    digitalWrite(waterPumpPin, LOW);
    Console.println("\n Watering stopped\n");
  }  
}

void switchLights() {
  if (lightOnThreshold > lightPercent && !lightIsOn) {
    Console.println("\n Turning lights on.\n");
    digitalWrite(lightSwitchPin, HIGH);
    lightIsOn = true;
  } 
  if (lightOnThreshold < lightPercent && lightIsOn) {
    Console.println("\n Turning lights off.\n");
    digitalWrite(lightSwitchPin, LOW);
    lightIsOn = false;
  }   
}

void readLightResistor() {
  lightResistorVal = analogRead(lightPin);
  lightPercent = map(lightResistorVal, lightMinVal, lightMaxVal, 0, 100);
}

void readMoistureSensor() {
  digitalWrite(hygroPower, HIGH);
  delay(1000); // nasty hack to wait for moisture drivers
  moistureSensorVal = analogRead(hygroPin);
  digitalWrite(hygroPower, LOW);
  moisturePercent = reversemap(moistureSensorVal, moistureMinVal, moistureMaxVal, 0, 100);

  // moistureStatus
  if (moistureSensorVal > 0 && moistureSensorVal < 300) {
    moistureStatus = "Wet";
  } else if (moistureSensorVal >= 300 && moistureSensorVal < 700) {
    moistureStatus = "Humid";
  } else if (moistureSensorVal >= 700) {
    moistureStatus = "Dry";
  } else {
    moistureStatus = "ERR";
  }

}

void readWaterLevelSensor() {
  digitalWrite(waterSensorPower, HIGH);
  delay(1000); // nasty hack to wait for moisture drivers
  waterLevelVal = analogRead(waterSensorPin);
  digitalWrite(waterSensorPower, LOW);
  waterLevelPercent = map(waterLevelVal, waterMinVal, waterMaxVal, 0, 100);
  
  if (waterLevelVal < waterLevelThreshold) {
   waterLevelStatus = "LOW";
  } else {
   waterLevelStatus = "OK";
  }    
}

void readAirMeasurements() {
  am.humidityAir = myDHT.readHumidity();
  am.tempAirC = myDHT.readTemperature();
  am.tempAirF = myDHT.readTemperature(true);
  am.heatIndex = myDHT.computeHeatIndex(am.tempAirF, am.humidityAir);
  
  if (am.humidityAir == 0  && am.tempAirC == 0) {
    Console.println("Failed to read from DHT sensor.");
  }
}

void readSoilMeasurements() {
//  Console.println("\nReading soil measurements.");
  byte data[12];
  byte addr[8];

  if ( !myPT.search(addr)) {
      //no more sensors on chain, reset search
      myPT.reset_search();
      Console.println("No probe sensor detected");
  }

  if ( OneWire::crc8( addr, 7) != addr[7]) {
      Console.println("CRC is not valid!");
  }

  if ( addr[0] != 0x10 && addr[0] != 0x28) {
      Console.print("Device is not recognized");
  }

  myPT.reset();
  myPT.select(addr);
  myPT.write(0x44,1); // start conversion, with parasite power on at the end

  byte present = myPT.reset();
  myPT.select(addr);    
  myPT.write(0xBE); // Read Scratchpad

  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = myPT.read();
  }
  
  myPT.reset_search();
  
  byte MSB = data[1];
  byte LSB = data[0];

  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  sm.tempSoilC = tempRead / 16;
  sm.tempSoilF = sm.tempSoilC*9/5+32;
}

void sendToKeen() {  
  Console.print("| Sending data to Keen.io\t\t\t|");
  String my_output = "{";
  // strings
  my_output += "\"AppId\":\"";                      my_output += appId;             my_output += "\"";
  my_output += ",\"Location\":\"";                  my_output += location;          my_output += "\"";
  my_output += ",\"WaterLevelStatus\":\"";          my_output += waterLevelStatus;  my_output += "\"";
  my_output += ",\"MoistureStatus\":\"";              my_output += moistureStatus;  my_output += "\"";
  // numerics
  my_output += ",\"AirTemperatureCelcius\":";       my_output += am.tempAirC;
  my_output += ",\"AirTemperatureFahrenheit\":";    my_output += am.tempAirF;
  my_output += ",\"HeatIndex\":";                   my_output += am.heatIndex;
  my_output += ",\"HumidityPercent\":";             my_output += am.humidityAir;
  my_output += ",\"LightPercent\":";                my_output += lightPercent;
  my_output += ",\"LightValue\":";                  my_output += lightResistorVal;
  my_output += ",\"LightIsOn\":";                   my_output += lightIsOn;
  my_output += ",\"MoisturePercent\":";             my_output += moisturePercent;
  my_output += ",\"MoistureValue\":";               my_output += moistureSensorVal;
  my_output += ",\"ProbeTemperatureCelcius\":";     my_output += sm.tempSoilC;
  my_output += ",\"ProbeTemperatureFahrenheit\":";  my_output += sm.tempSoilF;
  my_output += ",\"WaterLevelPercent\":";           my_output += waterLevelPercent;
  my_output += ",\"WaterLevelValue\":";             my_output += waterLevelVal;
  my_output += ",\"WaterPumptIsOn\":";              my_output += waterPumpIsOn;
  my_output += "}";

  myKeen.setApiVersion(F("3.0"));
  myKeen.setProjectId(KEEN_PROJECT_ID);
  myKeen.setWriteKey(KEEN_WRITE_KEY);
 
  myKeen.addEvent("arduino_board_test", my_output);
  myKeen.printRequest();

  Console.println("");

  while (myKeen.available()) {
    char c = myKeen.read();
    Console.print(c);
  }

  Console.print("\t\t\t\t|");
//  Console.flush();
  Console.print("\n-------------------------------------------------\n");
}

void outputEvent() {  

  String newLine = "|\t\t\t\t\t\t|\n";
  String endline = "\t\t\t|\n";

  String my_output = "\n ";
  my_output += millis();
//  my_output += "\t\t\t\t\t|";
  my_output += "\n-------------------------------------------------\n"; 
//  my_output += "\n";
  my_output += "| App Id\t\t";          my_output += appId;               my_output += endline;
  my_output += "| Location\t\t";        my_output += location;            my_output += "\t|\n";
  my_output += newLine;
  my_output += "| Temp Air, C\t\t";     my_output += am.tempAirC;         my_output += endline;
  my_output += "| Temp Soil, C\t\t";    my_output += sm.tempSoilC;        my_output += endline;
  my_output += "| Temp Air, F\t\t";     my_output += am.tempAirF;         my_output += endline;
  my_output += "| Temp Soil, F\t\t";    my_output += sm.tempSoilF;        my_output += endline;
  my_output += newLine;
  my_output += "| Heat Index\t\t";      my_output += am.heatIndex;        my_output += endline;
  my_output += "| Humidity %\t\t";      my_output += am.humidityAir;      my_output += endline;
  my_output += newLine;
  my_output += "| Light %\t\t";         my_output += lightPercent;        my_output += endline;
  my_output += "| Light Val\t\t";       my_output += lightResistorVal;    my_output += endline;
  my_output += "| Light Is On\t\t";     my_output += lightIsOn;           my_output += endline;
  my_output += newLine;
  my_output += "| Moisture\t\t";        my_output += moistureStatus;      my_output += endline;
  my_output += "| Moisture %\t\t";      my_output += moisturePercent;     my_output += endline;
  my_output += "| Moisture\t\t";        my_output += moistureSensorVal;   my_output += endline;
  my_output += newLine;
  my_output += "| Water Level\t\t";     my_output += waterLevelStatus;    my_output += endline;
  my_output += "| Water Level %\t\t";   my_output += waterLevelPercent;   my_output += endline;
  my_output += "| Water Level Val\t";   my_output += waterLevelVal;       my_output += endline;
  my_output += "| Water Pump Is On\t";  my_output += waterPumpIsOn;       my_output += endline;
  my_output += newLine;
      
  Console.println();
  Console.print(my_output);
}

long reversemap(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (in_min - x) * out_max / (in_min - in_max);
}

/*
Watering Details
Watering time                  15 seconds
Estimated amount per watering   6 oz
Water tank capacity            64 oz
Watering Frequency             Every twelve hours (twice daily)
Estimated watering for         About 5 days
*/


/*
const int moistureMaxVal = 453;
const int moistureMinVal = 799; 
range = 346 (.5 = 173)
626

173
346

799 ------------------------626 ------------------------ 453
  0 ------------------------ 50 ------------------------ 100

*/
