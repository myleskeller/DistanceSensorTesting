void setupMosfets() {
  pinMode(VL6180pin, OUTPUT);
  pinMode(GP2Y0A21YK0Fpin, OUTPUT);
  pinMode(ZXpin, OUTPUT);

  //  Serial.println("mosfet control enabled");
  disableMosfet(ALL_MOSFETS);
}

void enableMosfet(uint8_t device) {
  switch (device) {
    case GP2Y0A21YK0F_MOSFET:
      digitalWrite(GP2Y0A21YK0Fpin, HIGH);
      addDelayIfChanging(GP2Y0A21YK0FmosfetStatus);
      break;
    case VL6180_MOSFET:
      digitalWrite(VL6180pin, HIGH);
      addDelayIfChanging(VL6180mosfetStatus);
      break;
    case ZX_SENSOR_MOSFET:
      digitalWrite(ZXpin, HIGH);
      addDelayIfChanging(ZXmosfetStatus);
      break;
    case ALL_MOSFETS:
      digitalWrite(VL6180pin, HIGH);
      digitalWrite(GP2Y0A21YK0Fpin, HIGH);
      digitalWrite(ZXpin, HIGH);
      addDelayIfChanging(GP2Y0A21YK0FmosfetStatus);
      addDelayIfChanging(VL6180mosfetStatus);
      addDelayIfChanging(ZXmosfetStatus);
      break;
  }
}

void disableMosfet(uint8_t device) {
  switch (device) {
    case GP2Y0A21YK0F_MOSFET:
      digitalWrite(GP2Y0A21YK0Fpin, LOW);
      GP2Y0A21YK0FmosfetStatus = false;
      break;
    case VL6180_MOSFET:
      digitalWrite(VL6180pin, LOW);
      VL6180mosfetStatus = false;
      VL6180status = false;
      break;
    case ZX_SENSOR_MOSFET:
      digitalWrite(ZXpin, LOW);
      ZXmosfetStatus = false;
      break;
    case ALL_MOSFETS:
      digitalWrite(VL6180pin, LOW);
      digitalWrite(GP2Y0A21YK0Fpin, LOW);
      digitalWrite(ZXpin, LOW);
      GP2Y0A21YK0FmosfetStatus = false;
      VL6180mosfetStatus = false;
      ZXmosfetStatus = false;
      break;
  }
}

void addDelayIfChanging(bool &mosfetStatus) {
  if (mosfetStatus == false) {
    mosfetStatus = true;
    delay(100);
    //    Serial.println("waited.");
  }
}
