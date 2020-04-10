#define AREF    3.3
#define ARES    4096
#define INTERVAL 250 //in ms
#define MIN_V 0.25
#define MAX_V 3.5
int GP2Y0A21YK0F_noise_average = 0;

#define ARRAY_BOUND 3000 //didn't ever go above ~2100 when testing.
int GP2Y0A21YK0Freadings[ARRAY_BOUND];
unsigned long GP2Y0A21YK0Fiterations = 0;
unsigned long GP2Y0A21YK0Fstart = 0;



bool setupGP2Y0A21YK0F() {
  adcAttachPin(a0pin);
  float voltage = analogReadCalibrated(a0pin) * AREF / ARES;
  if (voltage < MIN_V || voltage > MAX_V) {
    Serial.println("GP2Y0A21YK0F Sensor Failed");
    GP2Y0A21YK0Fstatus = false;
  }
  else {
    //    Serial.println("GP2Y0A21YK0F Sensor Enabled (maybe)");
    GP2Y0A21YK0Fstatus = true;
  }
  return GP2Y0A21YK0Fstatus;
}

int getGP2Y0A21YK0FNoiseAverage() {
  if (GP2Y0A21YK0Fstatus == false)
    setupGP2Y0A21YK0F();

  //  Serial.println("-GP2Y0A21YK0F");

  unsigned long end = NOISE_AVERAGE_PERIOD + millis();
  unsigned long lastRefreshTime = 0;

  while (millis() < end) {
    readGP2Y0A21YK0F();
    //    Serial.print(readGP2Y0A21YK0F());
    //    Serial.print(',');
    if (millis() - lastRefreshTime >= NOISE_AVERAGE_PERIOD / 5) {
      lastRefreshTime += NOISE_AVERAGE_PERIOD / 5;
      // Serial.print('.');
    }
  }
  //  Serial.println();

  sort(GP2Y0A21YK0Freadings, GP2Y0A21YK0Fiterations);

  int maximum = GP2Y0A21YK0Freadings[0];
  int minimum = GP2Y0A21YK0Freadings[GP2Y0A21YK0Fiterations - 1];

  GP2Y0A21YK0F_noise_average = getGP2Y0A21YK0FDistance();

  //  Serial.print("Average: ");
  //  Serial.print(GP2Y0A21YK0F_noise_average);
  //  Serial.print(", Histeresis: ");
  //  Serial.println((maximum - minimum) * 10);

  Serial.print(GP2Y0A21YK0F_noise_average); Serial.print(",\t");
  Serial.print(maximum - minimum); Serial.print(",\t");

  return GP2Y0A21YK0F_noise_average;
}

int readGP2Y0A21YK0F() {
  if (GP2Y0A21YK0Fstatus == false)
    setupGP2Y0A21YK0F();

  int output = 0;

  if (millis() - GP2Y0A21YK0Fstart >= GP2Y0A21YK0F_INTEGRATION_PERIOD + 2) { //if time since last readings >= 40ms, the 2ms is just adding wiggle room
    toggleLED();
    output = calculatedDistance();
    GP2Y0A21YK0Freadings[GP2Y0A21YK0Fiterations] = output; //take new reading
    GP2Y0A21YK0Fstart = millis(); //set timer to current time
    GP2Y0A21YK0Fiterations++;
  }
  return output;
}

int getGP2Y0A21YK0FDistance() {
  sort(GP2Y0A21YK0Freadings, GP2Y0A21YK0Fiterations);

  if (GP2Y0A21YK0Fiterations == 0) //warn if no readings in array
    Serial.println("Not enough time given to take GP2Y0A21YK0F readings.");

  int output = GP2Y0A21YK0Freadings[GP2Y0A21YK0Fiterations / 2];
  GP2Y0A21YK0Fiterations = 0; //reset iteration counter
  return output * 10; //convert from cm to mm
}


float calculatedDistance() {
  return 27.728 * pow(map(analogReadCalibrated(a0pin), 0, 4095, 0, 5000) / 1000.0, -1.2045);
  //  return 65 * pow((analogReadCalibrated(a0pin) * AREF / ARES), -1.10);
}

void sort(int a[], int size) {
  for (int i = 0; i < (size - 1); i++) {
    bool flag = true;
    for (int o = 0; o < (size - (i + 1)); o++) {
      if (a[o] > a[o + 1]) {
        int t = a[o];
        a[o] = a[o + 1];
        a[o + 1] = t;
        flag = false;
      }
    }
    if (flag) break;
  }
}
