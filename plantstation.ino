//=====for Keen Integration====================
#include "ApiClient.h"
#include "KeenClient.h"
KeenClient myKeen;

//=====for writting to the console=============
#include "Bridge.h"
#include "Console.h"

//=====for Probe Thermometer===================
#include "OneWire.h"
const byte probePin = 3;
OneWire myPT(probePin);

typedef struct my_soilmeasurement {
  float tempSoilC;
  float tempSoilF;  
} SoilMeasurement;
SoilMeasurement sm;

//=====for Digital Thermo, Humidity Sensor=====
#include "DHT.h"
const byte dhtPin = 2;
DHT myDHT(dhtPin, DHT11);

typedef struct my_airmeasurement {
  byte humidityAir;
  float tempAirC;
  float tempAirF;
  float heatIndex;
} AirMeasurement;
AirMeasurement am;

//=====for keys, params========================
#include "params.h"

//=====for photoresistor=======================
const byte lightPin = A0;
int lightResistor;

//=====for hygrometer==========================
const byte hygroPin = A1;
int moistureSensor;

//=====these i am still thinking about=========
String appId = "Plant01";
String location = "Dinning Room Window";
boolean lightIsOn = false;
String waterLevelStatus = "OK";
boolean waterPumpIsOn = false;

void setup() {
  Serial.begin(9600); 
  Serial.println("PlantStation is warming up\n");
  myDHT.begin();

  Bridge.begin();
  Console.begin();

  while (!Console) { ; }
  Console.print("\nPlantStation is warming up.\nYun Console is warming up.\nKeen is warming up.\n\n");

  while (!Serial);
}

void loop() {
  readAirMeasurements();
  readSoilMeasurements();
  readLightResistor();
  readMoistureSensor();
  outputEvent();
  sendToKeen();

  // We run this once a minute, approximately
  delay(60000);
}

void waterPlants() {
}

void switchLights() {
}

void readLightResistor() {
  lightResistor = analogRead(lightPin);
}

void readMoistureSensor() {
  moistureSensor = analogRead(hygroPin);
}

void readAirMeasurements() {
  am.humidityAir = myDHT.readHumidity();
  am.tempAirC = myDHT.readTemperature();
  am.tempAirF = myDHT.readTemperature(true);
  am.heatIndex = myDHT.computeHeatIndex(am.tempAirF, am.humidityAir);
  
  if (am.humidityAir == 0  && am.tempAirC == 0) {
    Serial.println("Failed to read from DHT sensor.");
    Console.println("Failed to read from DHT sensor.");
  }
}

void readSoilMeasurements() {

  byte data[12];
  byte addr[8];

  if ( !myPT.search(addr)) {
      //no more sensors on chain, reset search
      myPT.reset_search();
      Serial.println("No probe sensor detected");
      Console.println("No probe sensor detected");
  }

  if ( OneWire::crc8( addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      Console.println("CRC is not valid!");
  }

  if ( addr[0] != 0x10 && addr[0] != 0x28) {
      Serial.print("Device is not recognized");
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
  my_output += "\"AppId\":\"";
  my_output += appId;
  my_output += "\",\"HumidityPercent\":";
  my_output += am.humidityAir;
  my_output += ",\"AirTemperatureCelcius\":";
  my_output += am.tempAirC;
  my_output += ",\"AirTemperatureFahrenheit\":";
  my_output += am.tempAirF;
  my_output += ",\"HeatIndex\":";
  my_output += am.heatIndex;
  my_output += ",\"Light\":";
  my_output += lightResistor;
  my_output += ",\"ProbeTemperatureCelcius\":";
  my_output += sm.tempSoilC;
  my_output += ",\"ProbeTemperatureFahrenheit\":";
  my_output += sm.tempSoilF;
  my_output += ",\"Moisture\":";
  my_output += moistureSensor;
  my_output += ",\"WaterLevelStatus\":\"";
  my_output += waterLevelStatus;
  my_output += "\",\"Location\":\"";
  my_output += location;
  my_output += "\",\"LightIsOn\":";
  my_output += lightIsOn;
  my_output += ",\"WaterPumptIsOn\":";
  my_output += waterPumpIsOn;
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
  my_output += "AppId:";
  my_output += appId;
  my_output += ";HumidityPercent:";
  my_output += am.humidityAir;
  my_output += ";AirTemperatureCelcius:";
  my_output += am.tempAirC;
  my_output += ";AirTemperatureFahrenheit:";
  my_output += am.tempAirF;
  my_output += ";HeatIndex:";
  my_output += am.heatIndex;
  my_output += ";Light:";
  my_output += lightResistor;
  my_output += ";ProbeTemperatureCelcius:";
  my_output += sm.tempSoilC;
  my_output += ";ProbeTemperatureFahrenheit:";
  my_output += sm.tempSoilF;
  my_output += ";Moisture:";
  my_output += moistureSensor;
  my_output += ";WaterLevelStatus:";
  my_output += waterLevelStatus;
  my_output += ";LightIsOn:";
  my_output += lightIsOn;
  my_output += ";Location:";
  my_output += location;
  my_output += ";WaterPumptIsOn:";
  my_output += waterPumpIsOn;

  Serial.println(my_output);
  Console.println(my_output);
}

