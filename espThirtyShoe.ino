#include "BluetoothSerial.h"
#include <Wire.h>
#include "esp_bt.h"
#include <ctype.h>
BluetoothSerial SerialBT;

//pins that i have potentially destroyed: D16, D17, D20, D21, D34, VP(MAYBE), VN(MAYBE)
const uint8_t DHT11pin =        0;  //D3;
const uint8_t VL6180pin =       2;  //D8;
const uint8_t GP2Y0A21YK0Fpin = 4;  //D4;
const uint8_t ZXpin =           32; //D5;
unsigned char a0pin =           35; //A0;
unsigned char LEDpin =          19; //built-in led
unsigned char i2cA0pin =        27; //D2; SDA (ZX)
unsigned char i2cA1pin =        25; //D1; SCL (ZX)
unsigned char i2cB0pin =        26; //D6; SDA (VL6180)
unsigned char i2cB1pin =        33; //D7; SCL (VL6180)

#define GP2Y0A21YK0F_MOSFET 1
#define VL6180_MOSFET 3
#define ZX_SENSOR_MOSFET 2
#define ALL_MOSFETS 0
#define GP2Y0A21YK0F_INTEGRATION_PERIOD 38 //ms, based on datasheet spec of 26Hz (38ms period)
#define ZX_INTEGRATION_PERIOD 20 //ms, based on datasheet spec of 50 samples per sec (20ms period)
#define VL6180_INTEGRATION_PERIOD 5 //ms, based on paranoia, because i'm pretty sure it's handled internally
#define NOISE_AVERAGE_PERIOD 5000 //ms
#define TESTING_PERIOD 5000
#define ERROR_VAL -2
#define OUT_OF_RANGE_VAL -1

#define COLUMNS 21
#define WHITE   0
#define BLACK   1
#define RECAL_INTERVAL 5 //min
#define I2C_A 0
#define I2C_B 1

bool ZXstatus = false;
bool VL6180status = false;
bool GP2Y0A21YK0Fstatus = false;

bool GP2Y0A21YK0FmosfetStatus = false;
bool VL6180mosfetStatus = false;
bool ZXmosfetStatus = false;

char SerialdefaultByte;
char BTdefaultByte;
int row[COLUMNS];
int dummy = -1;
bool i2cChannel;

#define AVERAGE_ITERATIONS 25

#define _TIME       0
#define _COLOR      1
#define _TEMP       2
#define _HUMID      3
#define _LUX        4

#define _LIDAR_ALL  5
#define _6180_ALL   6
#define _ZXZ_ALL    7
#define _ZXX_ALL    8

#define _LIDAR_P1   9
#define _6180_P1    10

#define _LIDAR_P2   11
#define _ZXZ_P2     12
#define _ZXX_P2     13

#define _6180_P3    14
#define _ZXZ_P3     15
#define _ZXX_P3     16

#define _LIDAR      17

#define _6180       18

#define _ZXZ        19
#define _ZXX        20

#define TOGGLE 3
#define FAST_PULSE 2
#define PULSE 1
#define OFF 0


bool debugging = false;
bool bluetooth_enabled = false;
bool led_enabled = true;
bool color_verbose = true;



void setup() {
  if (debugging) {
    debugSetup();
  }
  else {
    Serial.begin(115200);
    delay(1000);
    SerialdefaultByte = Serial.read();
    if (bluetooth_enabled) {
      SerialBT.begin("ESPThirtyShoe");
      BTdefaultByte = SerialBT.read();
    }
    else
      esp_bt_controller_disable();
    Serial.println();

    setupMosfets();
    enableMosfet(ALL_MOSFETS);
    Wire.begin(i2cA0pin, i2cA1pin);
    Wire1.begin(i2cB0pin, i2cB1pin);
    setupDHT11();
    setupLED();

    //    Serial.println("Enter [r] to take initial calibration.");
    Serial.println("Enter [r] when LED is pulsing RAPIDLY to update noise profile.");
    Serial.println("Enter [any alphanumeric character] when LED is pulsing SLOWLY to begin next reading.");
    waitForInput('r');
    Serial.println("\n!\tTEMP\tHUMID\tLUX\tLIDAR\tLIDAR_h\t6180\t6180_h\tZXZ\tZXZ_h\tTIME\t!");
    noiseAverageRoutine();
    //    Serial.println("\nEnter [c] to take begin taking readings.");
    //    waitForInput('c');
    waitForInput();
    Serial.println("\n\n!\tENVIRONMENTAL\t|\tSIMULTANEOUS\t|\tLIDAR & 6180\t|\tLIDAR & ZX\t|\t6180 & ZX\t|\tINDIVIDUAL\t!");
    Serial.println("TEMP\tHUMID\tLUX\tLIDAR\t6180\tZXZ\tLIDAR\t6180\tLIDAR\tZXZ\t6180\tZXZ\tLIDAR\t6180\tZXZ\tCOLOR\tTIME");
  }
}

void loop() {
  if (debugging) {
    debugLoop();
  }
  else {
    unsigned long end = RECAL_INTERVAL * 60000 + millis();
    while (millis() < end) {
      //-------------white------------
      row[_COLOR] = WHITE;
      readSensorRoutine();
      printRow();
      printRowBT();
      //      displayRow();
      //    Serial.println("White Obstacle Readings Taken. Enter [c] to continue.");
      //    waitForInput('c');
      waitForInput();
      //-------------black------------
      clearRow();
      row[_COLOR] = BLACK;
      readSensorRoutine();
      printRow();
      //      displayRow();
      //    Serial.println("Black Obstacle Readings Taken. Enter [c] to continue.\n");
      //    waitForInput('c');
      waitForInput();
    }
    //    Serial.println("Noise Calibration Required. \nClear the measurement surface and enter [r].");
    waitForInput('r');
    noiseAverageRoutine();
  }
}

void readSensorRoutine() {
  changeModeLED(TOGGLE);
  enableMosfet(ALL_MOSFETS);

  row[_TIME] = millis();
  row[_TEMP] = getDHT11Temperature();
  row[_HUMID] = getDHT11Humidity();
  row[_LUX] = getVL6180Lux();
  readSensorsforDuration(TESTING_PERIOD, row[_LIDAR_ALL], row[_6180_ALL], row[_ZXZ_ALL]); //all
  readSensorsforDuration(TESTING_PERIOD, row[_LIDAR_P1], row[_6180_P1], dummy); //lidar/vl6180
  readSensorsforDuration(TESTING_PERIOD, row[_LIDAR_P2], dummy, row[_ZXZ_P2]); //lidar/zx
  readSensorsforDuration(TESTING_PERIOD, dummy, row[_6180_P3], row[_ZXZ_P3]); //zx/vl6180
  readSensorsforDuration(TESTING_PERIOD, row[_LIDAR], dummy, dummy); //lidar
  readSensorsforDuration(TESTING_PERIOD, dummy, row[_6180], dummy); //vl6180
  readSensorsforDuration(TESTING_PERIOD, dummy, dummy, row[_ZXZ]); //zx

  disableMosfet(ALL_MOSFETS);
  //  Serial.println();
}

void noiseAverageRoutine() {
  clearRow();
  changeModeLED(TOGGLE);
  disableMosfet(ALL_MOSFETS);

  //  Serial.println("Getting Sensor Noise Profiles");
  row[_TIME] = millis();
  row[_TEMP] = getDHT11Temperature();
  row[_HUMID] = getDHT11Humidity();

  Serial.print('!'); Serial.print(",\t");

  Serial.print(row[_TEMP]); Serial.print(",\t");
  Serial.print(row[_HUMID]); Serial.print(",\t");

  enableMosfet(VL6180_MOSFET);
  row[_LUX] = getVL6180Lux();
  Serial.print(row[_LUX]); Serial.print(",\t");

  //-------------lidar------------
  disableMosfet(VL6180_MOSFET);
  disableMosfet(ZX_SENSOR_MOSFET);
  enableMosfet(GP2Y0A21YK0F_MOSFET);
  row[_LIDAR] = getGP2Y0A21YK0FNoiseAverage();
  //  Serial.println();

  //---------------vl-------------
  disableMosfet(GP2Y0A21YK0F_MOSFET);
  enableMosfet(VL6180_MOSFET);
  row[_6180] = getVL6180NoiseAverage();
  //  Serial.println();

  //--------------zx--------------
  disableMosfet(VL6180_MOSFET);
  enableMosfet(ZX_SENSOR_MOSFET);
  row[_ZXZ] = getZXNoiseAverage();
  //  Serial.println();

  disableMosfet(ALL_MOSFETS);

  Serial.print(row[_TIME]); Serial.print(",\t");
  Serial.print('!');
  Serial.println();
}

void displayRow() {
  Serial.print("Time:");
  Serial.print(row[_TIME]); Serial.print(',');
  Serial.print(" Temp:");
  Serial.print(row[_TEMP]); Serial.print(',');
  Serial.print(" Humid:");
  Serial.print(row[_HUMID]); Serial.print(',');
  Serial.print(" Lux:");
  Serial.print(row[_LUX]); Serial.print(',');
  Serial.print(" Color:");
  printColor(row[_COLOR]); Serial.print(',');

  Serial.print("\nSimultaneous Readings:");
  Serial.print(" Lidar:");
  Serial.print(row[_LIDAR_ALL]); Serial.print(',');
  Serial.print(" VL6180:");
  Serial.print(row[_6180_ALL]); Serial.print(',');
  Serial.print(" ZX:");
  Serial.print(row[_ZXZ_ALL]); Serial.print(',');
  //  Serial.println(row[_ZXX_ALL]);

  Serial.print("\nLidar/VL6180 Readings:");
  Serial.print(" Lidar:");
  Serial.print(row[_LIDAR_P1]); Serial.print(',');
  Serial.print(" VL6180:");
  Serial.print(row[_6180_P1]); Serial.print(',');

  Serial.print("\nLidar/ZX Sensor Readings:");
  Serial.print(" Lidar:");
  Serial.print(row[_LIDAR_P2]); Serial.print(',');
  Serial.print(" ZX:");
  Serial.print(row[_ZXZ_P2]); Serial.print(',');
  //  Serial.println(row[_ZXX_P2]);

  Serial.print("\nVL6180/ZX sensor Readings:");
  Serial.print(" VL6180:");
  Serial.print(row[_6180_P3]); Serial.print(',');
  Serial.print(" ZX:");
  Serial.print(row[_ZXZ_P3]); Serial.print(',');
  //  Serial.println(row[_ZXX_P3]);

  Serial.print("\nIndividual Readings:");
  Serial.print(" Lidar:");
  Serial.print(row[_LIDAR]); Serial.print(',');
  Serial.print(" VL6180:");
  Serial.print(row[_6180]); Serial.print(',');
  Serial.print(" ZX:");
  Serial.print(row[_ZXZ]); Serial.print(',');
  //  Serial.println(row[_ZXX]);
  Serial.println();
}

void printRow() {
  Serial.print(row[_TEMP]); Serial.print(",\t");
  Serial.print(row[_HUMID]); Serial.print(",\t");
  Serial.print(row[_LUX]); Serial.print(",\t");
  Serial.print(row[_LIDAR_ALL]); Serial.print(",\t");
  Serial.print(row[_6180_ALL]); Serial.print(",\t");
  Serial.print(row[_ZXZ_ALL]); Serial.print(",\t");
  //  Serial.print(row[_ZXX_ALL]);
  Serial.print(row[_LIDAR_P1]); Serial.print(",\t");
  Serial.print(row[_6180_P1]); Serial.print(",\t");
  Serial.print(row[_LIDAR_P2]); Serial.print(",\t");
  Serial.print(row[_ZXZ_P2]); Serial.print(",\t");
  //  Serial.print(row[_ZXX_P2]);
  Serial.print(row[_6180_P3]); Serial.print(",\t");
  Serial.print(row[_ZXZ_P3]); Serial.print(",\t");
  //  Serial.print(row[_ZXX_P3]);
  Serial.print(row[_LIDAR]); Serial.print(",\t");
  Serial.print(row[_6180]); Serial.print(",\t");
  Serial.print(row[_ZXZ]); Serial.print(",\t");
  //  Serial.println(row[_ZXX]);
  printColor(row[_COLOR]); Serial.print(",\t");
  Serial.print(row[_TIME]);
  Serial.println();

  if (bluetooth_enabled)
    printRowBT();
}

void printColor(int color) {
  if (color_verbose == true) {
    if (color == WHITE)
      Serial.print("white");
    if (color == BLACK)
      Serial.print("black");
  }
  else
    Serial.print(color);
}


void printRowBT() {
  SerialBT.print(row[_TEMP]); SerialBT.print(",\t");
  SerialBT.print(row[_HUMID]); SerialBT.print(",\t");
  SerialBT.print(row[_LUX]); SerialBT.print(",\t");
  SerialBT.print(row[_LIDAR_ALL]); SerialBT.print(",\t");
  SerialBT.print(row[_6180_ALL]); SerialBT.print(",\t");
  SerialBT.print(row[_ZXZ_ALL]); SerialBT.print(",\t");
  //  SerialBT.print(row[_ZXX_ALL]);
  SerialBT.print(row[_LIDAR_P1]); SerialBT.print(",\t");
  SerialBT.print(row[_6180_P1]); SerialBT.print(",\t");
  SerialBT.print(row[_LIDAR_P2]); SerialBT.print(",\t");
  SerialBT.print(row[_ZXZ_P2]); SerialBT.print(",\t");
  //  SerialBT.print(row[_ZXX_P2]);
  SerialBT.print(row[_6180_P3]); SerialBT.print(",\t");
  SerialBT.print(row[_ZXZ_P3]); SerialBT.print(",\t");
  //  SerialBT.print(row[_ZXX_P3]);
  SerialBT.print(row[_LIDAR]); SerialBT.print(",\t");
  SerialBT.print(row[_6180]); SerialBT.print(",\t");
  SerialBT.print(row[_ZXZ]); SerialBT.print(",\t");
  //  SerialBT.println(row[_ZXX]);
  SerialBT.print(row[_COLOR]); SerialBT.print(",\t");
  SerialBT.print(row[_TIME]);
  SerialBT.println();
}

void waitForInput(char input) { //loops until passed key detected
  changeModeLED(FAST_PULSE);
  while (checkInput() != input) {
    updateLED();
    yield();
  }
  changeModeLED(OFF);
}

void waitForInput() { //loops until any key detected
  changeModeLED(PULSE);
  while (!isalnum(checkInput())) {
    updateLED();
    yield();
  }
  changeModeLED(OFF);
}

char checkInput() {
  if (bluetooth_enabled) {
    if (SerialBT.available() > 0) {
      char incomingByte = SerialBT.read();
      if (incomingByte != BTdefaultByte) {
        return incomingByte;
      }
    }
  }
  else if (Serial.available() > 0) {
    char incomingByte = Serial.read();
    if (incomingByte != SerialdefaultByte) {
      return incomingByte;
    }
  }
  return 0;
}

void readSensorsforDuration(unsigned long period, int &GP, int &VL, int &ZX) {
  if (GP != -1)
    enableMosfet(GP2Y0A21YK0F_MOSFET);
  else
    disableMosfet(GP2Y0A21YK0F_MOSFET);
  if (VL != -1)
    enableMosfet(VL6180_MOSFET);
  else
    disableMosfet(VL6180_MOSFET);
  if (ZX != -1)
    enableMosfet(ZX_SENSOR_MOSFET);
  else
    disableMosfet(ZX_SENSOR_MOSFET);

  unsigned long end = period + millis();
  while (millis() < end) {
    if (GP != -1) {
      //      Serial.print('g');
      GP = readGP2Y0A21YK0F();
    }
    if (VL != -1) {
      //      Serial.print('v');
      VL = readVL6180();
    }
    if (ZX != -1) {
      //      Serial.print('z');
      ZX = readZX();
    }
  }
  if (GP != -1) {
    GP = getGP2Y0A21YK0FDistance();
    // Serial.print('.');
  }
  if (VL != -1) {
    VL = getVL6180Distance();
    // Serial.print('.');
  }
  if (ZX != -1) {
    ZX = getZXDistanceZ();
    // Serial.print('.');
  }
}

void debugSetup() {
  Serial.begin(115200);
  delay(1000);
  SerialdefaultByte = Serial.read();
  Serial.println();

  setupMosfets();
  enableMosfet(ZX_SENSOR_MOSFET);
  enableMosfet(VL6180_MOSFET);
  enableMosfet(GP2Y0A21YK0F_MOSFET);

  Wire.begin(i2cA0pin, i2cA1pin);
  Wire1.begin(i2cB0pin, i2cB1pin);
  setupDHT11();
}

void debugLoop() {
  Serial.print("temp: ");
  Serial.print(getDHT11Temperature());
  Serial.print(", hum: ");
  Serial.print(getDHT11Humidity());
  Serial.print(", lux: ");
  Serial.print(getVL6180Lux());
  Serial.print(", vl6180: ");
  readVL6180();
  readVL6180();
  Serial.print(getVL6180Distance());
  Serial.print(", zx: ");
  readZX();
  Serial.print(getZXDistanceZ()); Serial.print(','); Serial.print(getZXDistanceX());
  Serial.print(", GP2Y0A21YK0F: ");
  readGP2Y0A21YK0F();
  Serial.print(getGP2Y0A21YK0FDistance());
  Serial.println();
  delay(500);
}

void clearRow() {
  for (int i = 0; i < COLUMNS; i++) {
    row[i] = 0;
  }
}
