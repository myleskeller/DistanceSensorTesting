#include "DHTesp.h"
DHTesp dht;



bool setupDHT11() {
  bool status = false;
  dht.setup(DHT11pin, DHTesp::DHT11);
  char o = dht.getStatusString()[0], k = dht.getStatusString()[1];
  if (o == 'O' && k == 'K') {
    status = true;
    //    Serial.print("DHT11 Sensor "); Serial.println(dht.getStatusString());
  }
  else {
    Serial.println("DHT11 Sensor Failed");
  }
  return status;
}

float getDHT11Temperature() {
  float temp = dht.getTemperature();
  //janky double check for that one time i got MAX_INT as the result
  if (dht.toFahrenheit(temp) > 100) temp = dht.getTemperature();
  return dht.toFahrenheit(temp);
}

float getDHT11Humidity() {
  float humidity = dht.getHumidity();
  //janky double check for that one time i got MAX_INT as the result
  if (humidity > 99) humidity = dht.getHumidity();
  return humidity;
}
