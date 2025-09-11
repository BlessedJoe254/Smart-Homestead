#include <LiquidCrystal.h>
#include <Servo.h>

// --- LCD Setup (RS, E, D4, D5, D6, D7)
LiquidCrystal lcd(22, 23, 24, 25, 26, 27);

// --- Servo for clothesline
Servo clothesServo;
const int clothesServoPin = 9;

// --- Servo for Garage Door
Servo garageDoorServo;
const int garageDoorServoPin = 40; // New servo for garage door

// --- Relays
const int relayTable    = 30;
const int relayBedroom  = 31;
const int relayReboot  = 32;
const int relayGarage   = 33;
const int relaySecurity = 34;
const int relayAlarm    = 35;
const int relayFan      = 36;
const int relayFence    = 38; // Electric fence relay

// --- Rain Sensor
const int rainSensor = 50;

// --- States
bool tableState       = false;
bool bedroomState     = false;
bool rebootState     = false;
bool garageState      = false;
bool securityState    = false;
bool alarmState       = false;
bool fanState         = false;
bool fenceState       = false; // Electric fence state
bool garageDoorOpen   = false; // State of new garage door servo

int lastRainReading = -1; // -1 = unknown

// ---------- Helpers
String normalize(String s) {
  s.trim();
  for (size_t i = 0; i < s.length(); i++) {
    s[i] = (char)tolower(s[i]);
  }
  return s;
}

void setDevice(bool on, int relayPin, bool &stateRef) {
  stateRef = on;
  digitalWrite(relayPin, on ? LOW : HIGH); // active LOW relays
}

void setAll(bool on) {
  setDevice(on, relayTable,    tableState);
  setDevice(on, relayBedroom,  bedroomState);
  setDevice(on, relayReboot,  rebootState);
  setDevice(on, relayGarage,   garageState);
  setDevice(on, relaySecurity, securityState);
  setDevice(on, relayAlarm,    alarmState);
  setDevice(on, relayFan,      fanState);
  setDevice(on, relayFence,    fenceState);
}

// ---------- Setup
void setup() {
  // Relay pins
  pinMode(relayTable, OUTPUT);
  pinMode(relayBedroom, OUTPUT);
  pinMode(relayReboot, OUTPUT);
  pinMode(relayGarage, OUTPUT);
  pinMode(relaySecurity, OUTPUT);
  pinMode(relayAlarm, OUTPUT);
  pinMode(relayFan, OUTPUT);
  pinMode(relayFence, OUTPUT);

  // Ensure all OFF initially
  digitalWrite(relayTable, HIGH);
  digitalWrite(relayBedroom, HIGH);
  digitalWrite(relayReboot, HIGH);
  digitalWrite(relayGarage, HIGH);
  digitalWrite(relaySecurity, HIGH);
  digitalWrite(relayAlarm, HIGH);
  digitalWrite(relayFan, HIGH);
  digitalWrite(relayFence, HIGH);

  // Rain sensor & servos
  pinMode(rainSensor, INPUT);
  clothesServo.attach(clothesServoPin);
  clothesServo.write(0);
  garageDoorServo.attach(garageDoorServoPin);
  garageDoorServo.write(0); // closed initially

  // Serial ports
  Serial.begin(9600);
  Serial1.begin(9600);  // Bluetooth on Mega pins TX1=18, RX1=19
  Serial.setTimeout(200);
  Serial1.setTimeout(200);

  // LCD
  lcd.begin(16, 2);
  lcd.print("WELCOME TO JP");
  lcd.setCursor(0, 1);
  lcd.print("SMART HOME");
  delay(3000);
  lcd.clear();
  updateLCD();
}

// ---------- Main loop
void loop() {
  if (Serial1.available()) {
    char peeked = Serial1.peek();

    if ((peeked >= '0' && peeked <= '9') || 
        peeked == 'A' || peeked == 'B' || 
        peeked == 'C' || peeked == 'D' || 
        peeked == 'E' || peeked == 'F' ||
        peeked == 'G' || peeked == 'H') {
      char c = Serial1.read();
      processBluetoothCommand(c);
      updateLCD();
    } else {
      String cmd = Serial1.readStringUntil('\n');
      if (cmd.length() == 0) cmd = Serial1.readStringUntil('\r');
      cmd = normalize(cmd);
      if (cmd.length() > 0) {
        processVoiceCommand(cmd);
        updateLCD();
      }
    }
  }

  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    if (cmd.length() == 0) cmd = Serial.readStringUntil('\r');
    cmd = normalize(cmd);
    if (cmd.length() > 0) {
      processVoiceCommand(cmd);
      updateLCD();
    }
  }

  // --- Rain Sensor & Clothesline Servo
  int r = digitalRead(rainSensor);
  if (r == HIGH) {
    clothesServo.write(0);   // No rain
  } else {
    clothesServo.write(100); // Rain
  }

  if (r != lastRainReading) {
    lastRainReading = r;
    lcd.clear();
    lcd.setCursor(0, 0);
    if (r == LOW) {
      lcd.print("No Rain: OPEN");
    } else {
      lcd.print("Rain Detected");
      lcd.setCursor(0, 1);
      lcd.print("Clothesline CLOSE");
    }
    delay(2000);
    updateLCD();
  }
}

// ---------- Single-char Bluetooth commands
void processBluetoothCommand(char cmd) {
  if (cmd == '1') setDevice(true,  relayTable,    tableState);
  else if (cmd == '0') setDevice(false, relayTable,    tableState);

  else if (cmd == '2') setDevice(true,  relayBedroom,  bedroomState);
  else if (cmd == '3') setDevice(false, relayBedroom,  bedroomState);

  else if (cmd == '4') setDevice(true,  relayReboot,  rebootState);
  else if (cmd == '5') setDevice(false, relayReboot,  rebootState);

  else if (cmd == '6') setDevice(true,  relayGarage,   garageState);
  else if (cmd == '7') setDevice(false, relayGarage,   garageState);

  else if (cmd == '8') setDevice(true,  relaySecurity, securityState);
  else if (cmd == '9') setDevice(false, relaySecurity, securityState);

  else if (cmd == 'A') setDevice(true,  relayAlarm,    alarmState);
  else if (cmd == 'B') setDevice(false, relayAlarm,    alarmState);

  else if (cmd == 'C') setDevice(true,  relayFan,      fanState);
  else if (cmd == 'D') setDevice(false, relayFan,      fanState);

  else if (cmd == 'G') setDevice(true,  relayFence,    fenceState);  // Electric fence ON
  else if (cmd == 'H') setDevice(false, relayFence,    fenceState);  // Electric fence OFF

  else if (cmd == 'E') { garageDoorServo.write(90);  garageDoorOpen = true;  }  // Garage Door Open
  else if (cmd == 'F') { garageDoorServo.write(0);   garageDoorOpen = false; }  // Garage Door Close
}

// ---------- Voice phrases
void processVoiceCommand(String command) {
  if (!command.startsWith("jp ")) return;
  String rest = command.substring(3);

  if (rest == "all on")  { setAll(true);  return; }
  if (rest == "all off") { setAll(false); return; }

  bool turnOn = rest.endsWith(" on");
  bool turnOff = rest.endsWith(" off");

  if (rest == "open garage") {
    garageDoorServo.write(90);  // Door Open
    garageDoorOpen = true;
    return;
  }
  if (rest == "close garage") {
    garageDoorServo.write(0);   // Door Close
    garageDoorOpen = false;
    return;
  }

  String device = rest;
  if (turnOn || turnOff) {
    device = rest.substring(0, rest.lastIndexOf(' '));
  }

  if (device.indexOf("table") != -1)
    setDevice(turnOn, relayTable, tableState);
  else if (device.indexOf("bed") != -1)
    setDevice(turnOn, relayBedroom, bedroomState);
  else if (device.indexOf("reboot") != -1)
    setDevice(turnOn, relayReboot, rebootState);
  else if (device.indexOf("garage") != -1)
    setDevice(turnOn, relayGarage, garageState);
  else if (device.indexOf("security") != -1)
    setDevice(turnOn, relaySecurity, securityState);
  else if (device.indexOf("alarm") != -1)
    setDevice(turnOn, relayAlarm, alarmState);
  else if (device.indexOf("fan") != -1)
    setDevice(turnOn, relayFan, fanState);
  else if (device.indexOf("fence") != -1)
    setDevice(turnOn, relayFence, fenceState);
}

// ---------- LCD
void updateLCD() {
  lcd.clear();
  int countOn = 0;

  if (tableState)    countOn++;
  if (bedroomState)  countOn++;
  if (rebootState)  countOn++;
  if (garageState)   countOn++;
  if (securityState) countOn++;
  if (alarmState)    countOn++;
  if (fanState)      countOn++;
  if (fenceState)    countOn++;

  lcd.setCursor(0, 0);
  if (countOn == 0 && !garageDoorOpen) {
    lcd.print("Hey Joe, All Off");
    return;
  }
  if (countOn >= 2) {
    lcd.print("Hey Joe, ");
    lcd.print(countOn);
    lcd.print(" ON");
  } else {
    if (tableState)   { lcd.print("TableLight ON"); return; }
    if (bedroomState) { lcd.print("Bedroom ON");   return; }
    if (rebootState) { lcd.print("System reboot ON");   return; }
    if (garageState)  { lcd.print("GarageLight ON");return; }
    if (securityState){ lcd.print("Security ON");  return; }
    if (alarmState)   { lcd.print("Alarm ON");     return; }
    if (fanState)     { lcd.print("Fan ON");       return; }
    if (fenceState)   { lcd.print("Fence ON");     return; }
  }

  lcd.setCursor(0, 1);
  if (tableState)    lcd.print("Tbl ");
  if (bedroomState)  lcd.print("Bed ");
  if (rebootState)  lcd.print("Reboot ");
  if (garageState)   lcd.print("GarL ");
  if (securityState) lcd.print("Sec ");
  if (alarmState)    lcd.print("Alm ");
  if (fanState)      lcd.print("Fan ");
  if (fenceState)    lcd.print("Fence ");
  if (garageDoorOpen) lcd.print("Garage OPEN");
  else lcd.print("Garage CLSD");
}
