#include <ZX_Sensor.h>
const int ZX_ADDR = 0x10;  // ZX Sensor I2C address
ZX_Sensor zx_sensor = ZX_Sensor(ZX_ADDR);
float ZX_noise_averageZ = 0, ZX_histeresisZ = 0, ZX_noise_averageX = 0, ZX_histeresisX = 0;
int ZXreadingsZ[ARRAY_BOUND];
int ZXreadingsX[ARRAY_BOUND];
unsigned long ZXiterationsZ = 0;
unsigned long ZXiterationsX = 0;
unsigned long ZXstart = 0;

bool setupZX() {
  if ( zx_sensor.init() ) {
    //    Serial.println("ZX Sensor Enabled");
    ZXstatus = true;
  }
  else {
    Serial.println("ZX Sensor Failed");
    ZXstatus = false;
  }
  return ZXstatus;
}

void setupZXifNotAlready() {
  if (ZXstatus == false)
    setupZX();
}

float getZXNoiseAverage() {
  setupZXifNotAlready();

  //  Serial.println("-ZX Sensor");

  unsigned long end = NOISE_AVERAGE_PERIOD / 2 + millis();
  unsigned long lastRefreshTime = 0;

  ///////// this exists purey because readings start super high for some reason?? //////////
  while (millis() < end) {
    readZX();
    if (millis() - lastRefreshTime >= NOISE_AVERAGE_PERIOD / 5)  {
      lastRefreshTime += NOISE_AVERAGE_PERIOD / 5;
      // Serial.print('.');
    }
  }
  ZXiterationsZ = ZXiterationsX = 0;
  end = NOISE_AVERAGE_PERIOD / 2 + millis();
  lastRefreshTime = 0;
  //////////////////////////////////////////////////////////////////////////////////////////


  while (millis() < end) {
    readZX();
    //    Serial.print(readZX());
    //    Serial.print(',');
    if (millis() - lastRefreshTime >= (NOISE_AVERAGE_PERIOD / 5) * 2)  {
      lastRefreshTime += (NOISE_AVERAGE_PERIOD / 5) * 2;
      // Serial.print('.');
    }
  }
  //  Serial.println();

  sort(ZXreadingsZ, ZXiterationsZ);
  //  sort(ZXreadingsX, ZXiterationsX);

  int minimumZ = ZXreadingsZ[0];
  int maximumZ = ZXreadingsZ[ZXiterationsZ - 1];
  //  int minimumX = ZXreadingsX[0];
  //  int maximumX = ZXreadingsX[ZXiterationsX - 1];

  ZX_noise_averageZ = getZXDistanceZ();
  //  ZX_noise_averageX = getZXDistanceX();

  //  Serial.print("Average: ");
  //  Serial.print(ZX_noise_averageZ);
  //  Serial.print(", ");
  //  Serial.print(ZX_noise_averageX);
  //  Serial.print(", Histeresis: ");
  //  Serial.print(maximumZ - minimumZ);
  //  Serial.print(", ");
  //  Serial.print(maximumX - minimumX);
  //  Serial.println(']');

  Serial.print(ZX_noise_averageZ); Serial.print(",\t");
  Serial.print(maximumZ - minimumZ); Serial.print(",\t");

  return ZX_noise_averageZ;
}

int readZX() {
  setupZXifNotAlready();

  if (millis() - ZXstart >= ZX_INTEGRATION_PERIOD) { //if time since last readings >= 100ms
    if ( zx_sensor.positionAvailable() ) {
      toggleLED();
      ZXreadingsZ[ZXiterationsZ] = zx_sensor.readZ();
      ZXreadingsX[ZXiterationsX] = zx_sensor.readX();
      ZXstart = millis(); //set timer to current time
      ZXiterationsZ++;
      ZXiterationsX++;
    }
  }
  return ZXreadingsZ[ZXiterationsZ - 1];
}

int getZXDistanceZ() {
  setupZXifNotAlready();

  sort(ZXreadingsZ, ZXiterationsZ);

  unsigned long sum = 0;
  for (int i = 0; i < ZXiterationsZ; i++) {
    sum += ZXreadingsZ[i];
  }

  if (ZXiterationsZ == 0) { //warn if no readings in array
    //    Serial.println("Not enough time given to take ZX Sensor readings.");
    if (zx_sensor.readZ() != ZX_ERROR)
      return OUT_OF_RANGE_VAL;
    else
      return ERROR_VAL;
  }
  else {
    float output = sum / ZXiterationsZ; //compute result
    ZXiterationsZ = 0; //reset iteration counter
    return output;
  }
}

int getZXDistanceX() {
  setupZXifNotAlready();

  sort(ZXreadingsX, ZXiterationsX);

  unsigned long sum = 0;
  for (int i = 0; i < ZXiterationsX; i++) {
    sum += ZXreadingsX[i];
  }

  if (ZXiterationsX == 0) { //warn if no readings in array
    //    Serial.println("Not enough time given to take readings.");
    if (zx_sensor.readZ() != ZX_ERROR)
      return OUT_OF_RANGE_VAL;
    else
      return ERROR_VAL;
  }
  else {
    float output = sum / ZXiterationsX; //compute result
    ZXiterationsX = 0; //reset iteration counter
    return output;
  }
}
