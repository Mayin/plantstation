#include "ApiClient.h"
#include "Bridge.h"
#include "Console.h"
#include "DHT.h"
#include "KeenClient.h"
#include "OneWire.h"
#include "params.h"

// hardware
int lightPin = 0; // photoresistor
int hygroPin = 1; // moisture sensor
int dhtPin = 2;   // dht11
int probePin = 3; // temp probe

DHT dht(dhtPin, DHT11);
OneWire ds(probePin);
KeenClient keen;

// todo make this array (of error codes)
//boolean error;

typedef struct my_airmeasurement {
  int humidityAir;
  int tempAirC;
  int tempAirF;
  int heatIndex;
} AirMeasurement;
AirMeasurement am;

typedef struct my_soilmeasurement {
  int tempSoilC;
  int tempSoilF;  
} SoilMeasurement;
SoilMeasurement sm;

int lightResistor;
int moistureSensor;

String appId = "Plant01";
String location = "Dinning Room Window";
boolean lightStatus = false;
String waterLevelStatus = "OK";
boolean waterPumpStatus = false;

void setup() {
  Serial.begin(9600); 
  Serial.println("PlantStation is warming up\n");
  dht.begin();

  Bridge.begin();
  Console.begin();

  while (!Console) { ; }
  Console.print("\nPlantStation is warming up.\nYun Console is warming up.\nNow With Keen.\n\n");

  while (!Serial);
}

void loop() {

  delay(60000);
  am = getAirMeasurements();
  sm = getSoilMeasurements();
  lightResistor = analogRead(lightPin);
  moistureSensor = analogRead(hygroPin);
  
  outputEvent(am, lightResistor, sm);
  
  // send to keen
  sendToKeen(am, lightResistor, sm);
}

struct my_airmeasurement getAirMeasurements() {
  AirMeasurement me;
  me.humidityAir = dht.readHumidity();
  me.tempAirC = dht.readTemperature();
  me.tempAirF = dht.readTemperature(true);
  me.heatIndex = dht.computeHeatIndex(me.tempAirF, me.humidityAir);
  
  if (me.humidityAir == 0  && me.tempAirC == 0) {
    Serial.println("Failed to read from DHT sensor.");
    Console.println("Failed to read from DHT sensor.");
  }
  return me;
}

struct my_soilmeasurement getSoilMeasurements() {
  SoilMeasurement me;

  byte data[12];
  byte addr[8];

  if ( !ds.search(addr)) {
      //no more sensors on chain, reset search
      ds.reset_search();
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

  ds.reset();
  ds.select(addr);
  ds.write(0x44,1); // start conversion, with parasite power on at the end

  byte present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE); // Read Scratchpad

  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = ds.read();
  }
  
  ds.reset_search();
  
  byte MSB = data[1];
  byte LSB = data[0];

  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  me.tempSoilC = tempRead / 16;
  me.tempSoilF = me.tempSoilC*9/5+32;

  return me;
}

void sendToKeen(struct my_airmeasurement, int lightResistor, struct my_soilmeasurement) {  
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
  my_output += "\",\"LightStatus\":";
  my_output += lightStatus;
  my_output += ",\"WaterPumptStatus\":";
  my_output += waterPumpStatus;
  my_output += "}";
    
  keen.setApiVersion(F("3.0"));
  keen.setProjectId(KEEN_PROJECT_ID);
  keen.setWriteKey(KEEN_WRITE_KEY);
 
  keen.addEvent("arduino_board_test", my_output);
  keen.printRequest();

  Console.println();

  while (keen.available()) {
    char c = keen.read();
    Console.print(c);
  }
  Console.println();

  Console.flush();
}

void outputEvent(struct my_airmeasurement, int lightResistor, struct my_soilmeasurement) {  
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
  my_output += ";ProbeTemperatureCelcius";
  my_output += sm.tempSoilC;
  my_output += ";ProbeTemperatureFahrenheit:";
  my_output += sm.tempSoilF;
  my_output += ";Moisture:";
  my_output += moistureSensor;
  my_output += ";WaterLevelStatus:";
  my_output += waterLevelStatus;
  my_output += ";LightStatus:";
  my_output += lightStatus;
  my_output += ";Location:";
  my_output += location;
  my_output += ";WaterPumptStatus:";
  my_output += waterPumpStatus;

  Serial.println(my_output);
  Console.println(my_output);
}

// DHT11
// Connect pin 1 (on the left) of the sensor to +5V
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor
// Photoresistor ?

// todo: add led and red led for status of whole thing!

// http://forum.arduino.cc/index.php?topic=42140.0
// http://www.arduino.cc/en/Tutorial/ConsoleRead

