//===== General deployment defs =================
String appId = "Plant01";
String location = "Dinning Room Window";

//===== For private keys, params ================
#include "params.h"

//===== For Keen integration ====================
#include "ApiClient.h"
#include "KeenClient.h"
KeenClient myKeen;

//===== For writting to the console =============
#include "Bridge.h"
#include "Console.h"

//===== For 4 relay module ======================
#define LightSwitch 4

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
const byte lightOnThreshold = 90;
const byte lightPin = A0;
boolean lightIsOn = false;
int lightMaxVal = 550;
int lightMinVal = 6; 
int lightResistorVal;

//===== For hygrometer ==========================
byte moisturePercent;
const byte hygroPin = A1;
// todo - find right value for this
const int moistureThreshold = 1000; // <- made this up
int moistureMaxVal = 960;
int moistureMinVal = 50; 
int moistureSensorVal;

//===== For water level sensor ===================
const byte waterSensorPin = A2;
// todo - find right value for this
const int waterLevelThreshold = 300; // <- made this up
int waterLevelVal;
int waterMaxVal = 310;
int waterMinVal = 90; 
String waterLevelStatus = "OK";

//===== For water pump ===========================
boolean waterPumpIsOn = false;
unsigned long wateringSeconds = 61000;
unsigned long prevWatering = 0;

//===== For timed events =========================
// todo - timed events use regule marers for 1 min 1 hour etc and divide blah
unsigned long prevMinOneMillis = 0;
unsigned long minOneInterval = 60000;
unsigned long prevMinFiveMillis = 0;
unsigned long minFiveInterval = 300000;
unsigned long prevHourTwelveMillis = 0;
unsigned long hourTwelveInterval = 43200000;

void setup() {
  pinMode(LightSwitch, OUTPUT);

  myDHT.begin();
  Bridge.begin();
  Console.begin();

  while (!Console);
  Console.print("\nPlantStation is warming up.\nYun Console is warming up.\nKeen is warming up.\n\n");

  delay(5000);
}

void loop() {
  unsigned long currentMillis = millis();
  
  readAirMeasurements();
  readSoilMeasurements();
  readLightResistor();
  readMoistureSensor();  
  readWaterLevelSensor();  
  
  outputEvent();
  
  // these happen every minute (or so)
  if (currentMillis - prevMinOneMillis > minOneInterval) {
    prevMinOneMillis = currentMillis;
    
    sendToKeen();  
    waterPlantStarts();
  }
  
  // these happen every five minutes (or so)
  if (currentMillis - prevMinFiveMillis > minFiveInterval) {
    prevMinFiveMillis = currentMillis;
    
    switchLights();
  }

  // these happen every twelve hours (or so)
  if (currentMillis - prevHourTwelveMillis > hourTwelveInterval) {
    prevHourTwelveMillis = currentMillis;
    
    waterPlantStarts();
  }
  waterPlantStops(currentMillis);  
}

void waterPlantStarts() {
  // todo - figure out how to send a water pump is on event for log even thou its off at time.
  if (!waterPumpIsOn && waterLevelStatus == "OK" && moistureThreshold > moistureSensorVal) {
    prevWatering = millis();
    waterPumpIsOn = true;
    Console.println("Watering started");
  }
}

void waterPlantStops(unsigned long currentMillis) {
    if (waterPumpIsOn && currentMillis >= prevWatering + wateringSeconds) {
    waterPumpIsOn = false;
    Console.println("Watering stopped");    
  } else if (waterPumpIsOn && moistureThreshold < moistureSensorVal) {
    waterPumpIsOn = false;
    Console.println("Watering stopped");
  } else if (waterPumpIsOn && waterLevelStatus == "LOW") {
    waterPumpIsOn = false;
    Console.println("Watering stopped");
  }  
}

void switchLights() {
  if (lightOnThreshold > lightResistorVal && !lightIsOn) {
    digitalWrite(LightSwitch, HIGH);
    lightIsOn = true;
  } 
  if (lightOnThreshold < lightResistorVal && lightIsOn) {
    digitalWrite(LightSwitch, LOW);
    lightIsOn = false;
  }   
}

void readLightResistor() {
  lightResistorVal = analogRead(lightPin);
  lightPercent = map(lightResistorVal, lightMinVal, lightMaxVal, 0, 100);
}

void readMoistureSensor() {
  moistureSensorVal = analogRead(hygroPin);
  moisturePercent = map(moistureSensorVal, moistureMinVal, moistureMaxVal, 0, 100);
}

void readWaterLevelSensor() {
  int val = analogRead(waterSensorPin);
  waterLevelVal = map(val, waterMinVal, waterMaxVal, 0, 100);
  
  if (val < waterLevelThreshold) {
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

  String my_output = "{";
  // strings
  my_output += "\"AppId\":\"";                      my_output += appId;             my_output += "\"";
  my_output += ",\"Location\":\"";                  my_output += location;          my_output += "\"";
  my_output += ",\"WaterLevelStatus\":\"";          my_output += waterLevelStatus;  my_output += "\"";
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
  my_output += ",\"WaterLevelValue\":";             my_output += waterLevelVal;
  my_output += ",\"WaterPumptIsOn\":";              my_output += waterPumpIsOn;
  my_output += "}";

  myKeen.setApiVersion(F("3.0"));
  myKeen.setProjectId(KEEN_PROJECT_ID);
  myKeen.setWriteKey(KEEN_WRITE_KEY);
 
  myKeen.addEvent("arduino_board_test", my_output);
  myKeen.printRequest();

  Console.println();

  while (myKeen.available()) {
    char c = myKeen.read();
    Console.print(c);
  }

  Console.println();
  Console.flush();
}

void outputEvent() {  
  String my_output = "";
  // strings
  my_output += "AppId:";                        my_output += appId;
  my_output += ";Location:";                    my_output += location;
  my_output += ";WaterLevelStatus:";            my_output += waterLevelStatus;
  // numerics
  my_output += ";AirTemperatureCelcius:";       my_output += am.tempAirC;
  my_output += ";AirTemperatureFahrenheit:";    my_output += am.tempAirF;
  my_output += ";HeatIndex:";                   my_output += am.heatIndex;
  my_output += ";HumidityPercent:";             my_output += am.humidityAir;
  my_output += ";LightPercent:";                my_output += lightPercent;
  my_output += ";LightValue:";                  my_output += lightResistorVal;
  my_output += ";LightIsOn:";                   my_output += lightIsOn;
  my_output += ";MoisturePercent:";             my_output += moisturePercent;
  my_output += ";MoistureValue:";               my_output += moistureSensorVal;
  my_output += ";ProbeTemperatureCelcius:";     my_output += sm.tempSoilC;  
  my_output += ";ProbeTemperatureFahrenheit:";  my_output += sm.tempSoilF;
  my_output += ";WaterLevelValue:";             my_output += waterLevelVal;
  my_output += ";WaterPumptIsOn:";              my_output += waterPumpIsOn;
  
  Console.println(my_output);
}

