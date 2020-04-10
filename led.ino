#define led_freq 5000
#define led_channel 0
#define led_resolution 8
#define led_max_duty 255
int led_mode = OFF;
int led_duty = OFF;
int pulse_speed_reduction = 4;
int led_pulse_max = led_max_duty * pulse_speed_reduction;
int led_brightness_reduction = 8;
bool rising = true;

void setupLED() {
  if (led_enabled) {
    ledcSetup(led_channel, led_freq, led_resolution);
    ledcAttachPin(LEDpin, led_channel);
    led_duty = OFF;
  }
}

void changeModeLED(int mode) {
  if (led_enabled) {
    //    if (led_mode == OFF)
    setupLED();
    led_mode = mode;
  }
}

void updateLED() {
  if (led_enabled) {
    switch (led_mode) {
      case OFF:
        disableLED();
        break;
      case PULSE:
        pulseLED();
        break;
      case FAST_PULSE:
        pulseFastLED();
        break;
      case TOGGLE:
        toggleLED();
        break;

    }
    //    Serial.println(led_duty /  led_brightness_reduction);
  }
}

void pulseLED() {
  if (led_enabled) {
    if (rising == true) {
      if (led_duty >= led_pulse_max) {
        rising = false;
      }
      else {
        led_duty++;
        if (led_duty % pulse_speed_reduction == 0)
          ledcWrite(led_channel, (led_duty / pulse_speed_reduction) / led_brightness_reduction);
      }
    }
    else if (rising == false) {
      if (led_duty <= 0) {
        rising = true;
      }
      else {
        led_duty--;
        if (led_duty % pulse_speed_reduction == 0)
          ledcWrite(led_channel, (led_duty / pulse_speed_reduction) / led_brightness_reduction);
      }
    }
    delay(1); //esp32 too fast to reflect changes otherwise?
  }
}

void pulseFastLED() {
  if (led_enabled) {
    if (rising == true) {
      if (led_duty >= led_max_duty) {
        rising = false;
      }
      else {
        led_duty++;
        led_duty++;
        ledcWrite(led_channel, led_duty / led_brightness_reduction);
      }
    }
    else if (rising == false) {
      if (led_duty <= 0) {
        rising = true;
      }
      else {
        led_duty--;
        led_duty--;
        ledcWrite(led_channel, led_duty / led_brightness_reduction);
      }
    }
    //                Serial.println(led_duty);
    delay(1); //esp32 too fast to reflect changes otherwise?
  }
}

void toggleLED() {
  if (led_enabled) {
    if (led_duty == OFF) {
      led_duty = led_max_duty;// / led_brightness_reduction;
      ledcWrite(led_channel, led_duty);// / led_brightness_reduction);
    }
    else {
      led_duty = OFF;
      ledcWrite(led_channel, OFF);
    }
    //    Serial.print("LED:"); Serial.println(led_duty / led_brightness_reduction);
  }
}

void disableLED() {
  if (led_enabled) {
    led_duty = OFF;
    ledcWrite(led_channel, led_duty);
    pinMode(LEDpin, OUTPUT);
    digitalWrite(LEDpin, LOW);
  }
}
