#include "Adafruit_VL6180X.h"
Adafruit_VL6180X vl = Adafruit_VL6180X();
float VL6180_noise_average = 0, VL6180_histeresis = 0, VL6180Lux = ERROR_VAL;
int VL6180readings[ARRAY_BOUND];
unsigned long VL6180iterations = 0;
unsigned long VL6180start = 0;
bool VL6180_range_error_detected = false;


bool setupVL6180() {
  if (! vl.begin(&Wire1)) {
    Serial.println("VL6180 Sensor Failed");
    VL6180status = false;
  }
  else {
    //    Serial.println("VL6180 Sensor Enabled");
    VL6180status = true;
  }
  return VL6180status;
}

int getVL6180NoiseAverage() {
  setupVL6180ifNotAlready();


  //  Serial.println("-VL6180");

  unsigned long end = NOISE_AVERAGE_PERIOD + millis();
  unsigned long lastRefreshTime = 0;

  while (millis() < end) {
    readVL6180();
    //    Serial.print(readVL6180());
    //    Serial.print(',');
    if (millis() - lastRefreshTime >= NOISE_AVERAGE_PERIOD / 5)  {
      lastRefreshTime += NOISE_AVERAGE_PERIOD / 5;
      // Serial.print('.');
    }
  }
  //  Serial.println();

  sort(VL6180readings, VL6180iterations);

  int minimum = VL6180readings[0];
  int maximum = VL6180readings[VL6180iterations - 1];

  VL6180_noise_average = getVL6180Distance();

  //  Serial.print("Average: ");
  //  Serial.print(VL6180_noise_average);
  //  Serial.print(", Histeresis: ");
  //  Serial.println(maximum - minimum);

  Serial.print(VL6180_noise_average); Serial.print(",\t");
  Serial.print(maximum - minimum); Serial.print(",\t");

  return (int)VL6180_noise_average;
}


int readVL6180() {
  setupVL6180ifNotAlready();
  if (millis() - VL6180start >= VL6180_INTEGRATION_PERIOD) { //if time since last readings >= 100ms
    //  VL6180Lux =  vl.readLux(VL6180X_ALS_GAIN_5);
    if (vl.readRangeStatus() == VL6180X_ERROR_NONE) {
      toggleLED();
      VL6180readings[VL6180iterations] = vl.readRange();
      //    Serial.println(vl.readRange());
      VL6180start = millis(); //set timer to current time
      VL6180iterations++;
    }
    else if (vl.readRangeStatus() != VL6180X_ERROR_NONE) {
      VL6180_range_error_detected = true;
    }
  }
  return VL6180readings[VL6180iterations - 1];
}

int getVL6180Distance() {
  setupVL6180ifNotAlready();

  sort(VL6180readings, VL6180iterations);

  unsigned long sum = 0;
  for (int i = 0; i < VL6180iterations; i++) {
    sum += VL6180readings[i];
  }

  if (VL6180iterations == 0) {//warn if no readings in array
    //    Serial.println("Not enough time given to take VL6180 readings.");
    if (VL6180_range_error_detected) {
      VL6180_range_error_detected = false;
      return OUT_OF_RANGE_VAL;
    }
    else
      return ERROR_VAL;
  }
  else {
    float output = sum / VL6180iterations; //compute result
    VL6180iterations = 0; //reset iteration counter
    VL6180_range_error_detected = false;
    return output;
  }
}

float getVL6180Lux() { //if this works, you don't have an i2c issue.
  setupVL6180ifNotAlready();
  return vl.readLux(VL6180X_ALS_GAIN_5);
  //  return VL6180Lux;
}

void displayError(uint8_t status) {
  if  ((status >= VL6180X_ERROR_SYSERR_1) && (status <= VL6180X_ERROR_SYSERR_5))
    Serial.println("System error");
  else if (status == VL6180X_ERROR_ECEFAIL)
    Serial.println("ECE failure");
  else if (status == VL6180X_ERROR_NOCONVERGE)
    Serial.println("No convergence");
  else if (status == VL6180X_ERROR_RANGEIGNORE)
    Serial.println("Ignoring range");
  else if (status == VL6180X_ERROR_SNR)
    Serial.println("Signal/Noise error");
  else if (status == VL6180X_ERROR_RAWUFLOW)
    Serial.println("Raw reading underflow");
  else if (status == VL6180X_ERROR_RAWOFLOW)
    Serial.println("Raw reading overflow");
  else if (status == VL6180X_ERROR_RANGEUFLOW)
    Serial.println("Range reading underflow");
  else if (status == VL6180X_ERROR_RANGEOFLOW)
    Serial.println("Range reading overflow");
}

void setupVL6180ifNotAlready() {
  if (VL6180status == false)
    setupVL6180();
}
