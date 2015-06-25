#include "DHT.h"
#include "OneWire.h"
#include "Console.h"

// hardware
int lightPin = 0; // photoresistor
int dhtPin = 2;   // dht11
int probePin = 3; // temp probe

DHT dht(dhtPin, DHT11);
OneWire ds(probePin);

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

void setup() {
  Serial.begin(9600); 
  Serial.println("PlantStation is warming up\n");
  dht.begin();

  Bridge.begin();
  Console.begin();

  while (!Console) { ; }
  Console.print("PlantStation is warming up.\n");
  Console.println("Yun Console is warming up.\n");
}

void loop() {
  delay(1000);
  am = getAirMeasurements();
  sm = getSoilMeasurements();
  lightResistor = analogRead(lightPin);
  
  // wanna have one of these for sending to ip:port option
  //outputSerial(am, lightResistor, sm);
  outputConsole(am, lightResistor, sm);
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

void outputSerial(struct my_airmeasurement, int lightResistor, struct my_soilmeasurement) {
  Serial.print("HumidityPercent:");
  Serial.print(am.humidityAir);
  Serial.print(";AirTemperatureCelcius:");
  Serial.print(am.tempAirC);
  Serial.print(";AirTemperatureFahrenheit:");
  Serial.print(am.tempAirF);
  Serial.print(";HeatIndex:");
  Serial.print(am.heatIndex);
  Serial.print(";Light:");
  Serial.print(lightResistor); 
  Serial.print(";ProbeTemperatureCelcius:");
  Serial.print(sm.tempSoilC);
  Serial.print(";ProbeTemperatureFahrenheit:");
  Serial.println(sm.tempSoilF);
}   

void outputConsole(struct my_airmeasurement, int lightResistor, struct my_soilmeasurement) {
  Console.print("HumidityPercent:");
  Console.print(am.humidityAir);
  Console.print(";AirTemperatureCelcius:");
  Console.print(am.tempAirC);
  Console.print(";AirTemperatureFahrenheit:");
  Console.print(am.tempAirF);
  Console.print(";HeatIndex:");
  Console.print(am.heatIndex);
  Console.print(";Light:");
  Console.print(lightResistor); 
  Console.print(";ProbeTemperatureCelcius:");
  Console.print(sm.tempSoilC);
  Console.print(";ProbeTemperatureFahrenheit:");
  Console.println(sm.tempSoilF);
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

