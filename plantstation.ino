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
// todo - find right value for these
// Dry  0 - 300 
// Humid 300 - 700
// Wet 700 - 950
// But freaking backwards!
const int moistureMaxVal = 255;
const int moistureMinVal = 982; 
const byte moistureThreshold = 30; // <- made this up, to %

//===== For water level sensor ===================
byte waterLevelPercent;
const byte waterSensorPin = A3;
int waterLevelVal;
String waterLevelStatus = "OK";

//===== For light switch =========================
const byte lightSwitchPin = 4;
const byte lightOnThreshold = 10;
const int lightMaxVal = 250;  
const int lightMinVal = 0; 
boolean lightIsOn = false;

//===== For water pump ===========================
const byte waterPumpPin = A5;
// todo - find right value for this
const int waterLevelThreshold = 20; // <- made this up    
boolean waterPumpIsOn = false;
unsigned long wateringSeconds = 61000;
unsigned long prevWatering = 0;
int waterMaxVal = 690;
int waterMinVal = 345;   

//===== For timed events =========================
// todo - timed events use regule marers for 1 min 1 hour etc and divide blah
unsigned long prevMinOneMillis = 0;    
unsigned long minOneInterval = 60000;
unsigned long prevMinFiveMillis = 0;
unsigned long minFiveInterval = 300000;
unsigned long prevHourTwelveMillis = 0;
unsigned long hourTwelveInterval = 43200000;

void setup() {
  pinMode(lightSwitchPin, OUTPUT);
  pinMode(waterPumpPin, OUTPUT);
  pinMode(hygroPower, OUTPUT);

  digitalWrite(waterPumpPin, LOW);

  myDHT.begin();
  Bridge.begin();
  Console.begin();    

  while (!Console);
  Console.print("\nPlantStation is warming up.\nYun Console is warming up.\nKeen is warming up.\n\n");

  // Because we need initial moisture readings
  digitalWrite(hygroPower, HIGH);
  delay(5000);
  readMoistureSensor();  
  digitalWrite(hygroPower, LOW);
  // Because end
}

void loop() {
  unsigned long currentMillis = millis();
  
  readAirMeasurements();
  readSoilMeasurements();
  readLightResistor();
  readWaterLevelSensor();  
  
  outputEvent();

  // these happen every minute (or so)
  if (currentMillis - prevMinOneMillis > minOneInterval) {
    prevMinOneMillis = currentMillis;
    
    sendToKeen();  
  }
  
  // these happen every five minutes (or so)
  if (currentMillis - prevMinFiveMillis > minFiveInterval) {
    prevMinFiveMillis = currentMillis;
    
    waterPlantStarts();
    switchLights();
  }

  // these happen every twelve hours (or so)
  if (currentMillis - prevHourTwelveMillis > hourTwelveInterval) {
    prevHourTwelveMillis = currentMillis;
    
    readMoistureSensor();  
    waterPlantStarts();
  }
  waterPlantStops(currentMillis);  
}

void waterPlantStarts() {
  // todo - figure out how to send a water pump is on event for log even thou its off at time.
  if (!waterPumpIsOn && waterLevelStatus == "OK" && moistureThreshold > moisturePercent) {
    prevWatering = millis();
    waterPumpIsOn = true;
    digitalWrite(waterPumpPin, HIGH);
    Console.println("Watering started");
  }
}

void waterPlantStops(unsigned long currentMillis) {
    if (waterPumpIsOn && currentMillis >= prevWatering + wateringSeconds) {
    waterPumpIsOn = false;
    digitalWrite(waterPumpPin, LOW);
    Console.println("Watering stopped");    
  } else if (waterPumpIsOn && moistureThreshold < moisturePercent) {
    waterPumpIsOn = false;
    digitalWrite(waterPumpPin, LOW);
    Console.println("Watering stopped");
  } else if (waterPumpIsOn && waterLevelStatus == "LOW") {
    waterPumpIsOn = false;
    digitalWrite(waterPumpPin, LOW);
    Console.println("Watering stopped");
  }  
}

void switchLights() {
  if (lightOnThreshold > lightPercent && !lightIsOn) {
    digitalWrite(lightSwitchPin, HIGH);
    lightIsOn = true;
  } 
  if (lightOnThreshold < lightPercent && lightIsOn) {
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
}

void readWaterLevelSensor() {
  waterLevelVal = analogRead(waterSensorPin);
  waterLevelPercent = map(waterLevelVal, waterMinVal, waterMaxVal, 0, 100);
  
  if (waterLevelPercent < waterLevelThreshold) {
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
  my_output += ",\"WaterLevelPercent\":";           my_output += waterLevelPercent;
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
  //my_output += "AppId:";                        my_output += appId;
  //my_output += ";Location:";                    my_output += location;
  my_output += ";WaterLevelStatus:";            my_output += waterLevelStatus;
  // numerics
  //my_output += ";AirTemperatureCelcius:";       my_output += am.tempAirC;
  my_output += ";AirTemperatureFahrenheit:";    my_output += am.tempAirF;
  my_output += ";HeatIndex:";                   my_output += am.heatIndex;
  my_output += ";HumidityPercent:";             my_output += am.humidityAir;
  my_output += ";LightPercent:";                my_output += lightPercent;
  my_output += ";LightValue:";                  my_output += lightResistorVal;
  my_output += ";LightIsOn:";                   my_output += lightIsOn;
  my_output += ";MoisturePercent:";             my_output += moisturePercent;
  my_output += ";MoistureValue:";               my_output += moistureSensorVal;
  //my_output += ";ProbeTemperatureCelcius:";     my_output += sm.tempSoilC;  
  my_output += ";ProbeTemperatureFahrenheit:";  my_output += sm.tempSoilF;
  my_output += ";WaterLevelPercent:";           my_output += waterLevelPercent;
  my_output += ";WaterLevelValue:";             my_output += waterLevelVal;
  my_output += ";WaterPumptIsOn:";              my_output += waterPumpIsOn;
      
  Console.println(my_output);
}

long reversemap(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (in_min - x) * out_max / (in_min - in_max);
}

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
