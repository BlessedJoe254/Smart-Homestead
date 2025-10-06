#include <LiquidCrystal.h>
#include <Servo.h>
#include <DFRobotDFPlayerMini.h>

// --- LCD Setup (RS, E, D4, D5, D6, D7)
LiquidCrystal lcd(22, 23, 24, 25, 26, 27);

// --- Servo for clothesline
Servo clothesServo;
const int clothesServoPin = 9;

// --- Servo for Garage Door
Servo garageDoorServo;
const int garageDoorServoPin = 40;

// --- Relays
const int relayTable    = 30;
const int relayBedroom  = 31;
const int relayReboot   = 32;
const int relayGarage   = 33;
const int relaySecurity = 34;
const int relayAlarm    = 35;
const int relayFan      = 36;
const int relayFence    = 38;

// --- Rain Sensor
const int rainSensor = 50;

// --- DFPlayer
DFRobotDFPlayerMini myDFPlayer;
#define DFPLAYER_BUSY_PIN 6

// --- States
bool tableState       = false;
bool bedroomState     = false;
bool rebootState      = false;
bool garageState      = false;
bool securityState    = false;
bool alarmState       = false;
bool fanState         = false;
bool fenceState       = false;
bool garageDoorOpen   = false;
bool musicPlaying     = false;

int lastRainReading = -1;

// ---------- Song Lookup Table ----------
struct Song {
  String command;
  int track;
  String display;
};

Song songs[] = {
  {"play holy forever", 23, "Holy Forever"},
  {"play nina siri", 24, "Nina Siri"},
  {"play malengo", 25, "Malengo"},
  {"play wa ajabu", 26, "Wa Ajabu"}
  // 👉 Add more songs here easily: {"play <name>", trackNumber, "Display Name"}
};
int songCount = sizeof(songs) / sizeof(songs[0]);

// ---------- Helpers
String normalize(String s) {
  s.trim();
  for (size_t i = 0; i < s.length(); i++) s[i] = (char)tolower(s[i]);
  return s;
}

// --- Wait until audio finishes playing
void waitForAudio() {
  while (digitalRead(DFPLAYER_BUSY_PIN) == LOW) {
    delay(50);
  }
}

void setDevice(bool on, int relayPin, bool &stateRef, int audioNumOn, int audioNumOff) {
  stateRef = on;
  digitalWrite(relayPin, on ? LOW : HIGH);
  waitForAudio();
  myDFPlayer.play(on ? audioNumOn : audioNumOff);
}

// Set all devices at once
void setAll(bool on) {
  setDevice(on, relayTable, tableState, 1, 2);
  setDevice(on, relayBedroom, bedroomState, 3, 4);
  setDevice(on, relayReboot, rebootState, 5, 5);
  setDevice(on, relayGarage, garageState, 6, 7);
  setDevice(on, relaySecurity, securityState, 8, 9);
  setDevice(on, relayAlarm, alarmState, 10, 11);
  setDevice(on, relayFan, fanState, 12, 13);
  setDevice(on, relayFence, fenceState, 14, 15);
}

// ---------- Setup
void setup() {
  // Relay pins
  int relayPins[] = {relayTable, relayBedroom, relayReboot, relayGarage, relaySecurity, relayAlarm, relayFan, relayFence};
  for (int i = 0; i < 8; i++) pinMode(relayPins[i], OUTPUT);

  for (int i = 0; i < 8; i++) digitalWrite(relayPins[i], HIGH);

  pinMode(rainSensor, INPUT);
  clothesServo.attach(clothesServoPin);
  clothesServo.write(0);
  garageDoorServo.attach(garageDoorServoPin);
  garageDoorServo.write(0);

  pinMode(DFPLAYER_BUSY_PIN, INPUT);
  Serial.begin(9600);
  Serial1.begin(9600);   // Bluetooth
  Serial2.begin(9600);   // DFPlayer
  if (!myDFPlayer.begin(Serial2)) Serial.println("DFPlayer not detected!");
  else {
    Serial.println("DFPlayer online.");
    myDFPlayer.play(22); // Welcome
    waitForAudio();
  }

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
    if ((peeked >= '0' && peeked <= '9') || (peeked >= 'A' && peeked <= 'H')) {
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

  int r = digitalRead(rainSensor);
  if (r == HIGH) clothesServo.write(0);
  else clothesServo.write(100);

  if (r != lastRainReading) {
    lastRainReading = r;
    lcd.clear();
    lcd.setCursor(0, 0);
    if (r == LOW) {
      lcd.print("No Rain: OPEN");
      waitForAudio();
      myDFPlayer.play(21);
    } else {
      lcd.print("Rain Detected");
      lcd.setCursor(0,1);
      lcd.print("Clothesline CLOSE");
      waitForAudio();
      myDFPlayer.play(20);
    }
    delay(2000);
    updateLCD();
  }
}

// ---------- Bluetooth Commands
void processBluetoothCommand(char cmd) {
  switch(cmd) {
    case '1': setDevice(true, relayTable, tableState, 1,2); break;
    case '0': setDevice(false, relayTable, tableState, 1,2); break;
    case '2': setDevice(true, relayBedroom, bedroomState, 3,4); break;
    case '3': setDevice(false, relayBedroom, bedroomState, 3,4); break;
    case '4': setDevice(true, relayReboot, rebootState, 5,5); break;
    case '5': setDevice(false, relayReboot, rebootState, 5,5); break;
    case '6': setDevice(true, relayGarage, garageState, 6,7); break;
    case '7': setDevice(false, relayGarage, garageState, 6,7); break;
    case '8': setDevice(true, relaySecurity, securityState, 8,9); break;
    case '9': setDevice(false, relaySecurity, securityState, 8,9); break;
    case 'A': setDevice(true, relayAlarm, alarmState, 10,11); break;
    case 'B': setDevice(false, relayAlarm, alarmState, 10,11); break;
    case 'C': setDevice(true, relayFan, fanState, 12,13); break;
    case 'D': setDevice(false, relayFan, fanState, 12,13); break;
    case 'G': setDevice(true, relayFence, fenceState, 14,15); break;
    case 'H': setDevice(false, relayFence, fenceState, 14,15); break;
    case 'E': garageDoorServo.write(90); garageDoorOpen=true; waitForAudio(); myDFPlayer.play(16); break;
    case 'F': garageDoorServo.write(0); garageDoorOpen=false; waitForAudio(); myDFPlayer.play(17); break;
  }
}

// ---------- Voice Commands
void processVoiceCommand(String command) {
  if (!command.startsWith("jp ")) return;
  String rest = command.substring(3);

  if (rest == "all on") { setAll(true); waitForAudio(); myDFPlayer.play(18); return; }
  if (rest == "all off") { setAll(false); waitForAudio(); myDFPlayer.play(19); return; }

  if (rest == "stop music") {
    myDFPlayer.stop();
    musicPlaying = false;
    lcd.clear();
    lcd.print("Music Stopped");
    delay(2000);
    updateLCD();
    return;
  }

  // --- Check songs table ---
  for (int i = 0; i < songCount; i++) {
    if (rest == songs[i].command) {
      waitForAudio();
      myDFPlayer.play(songs[i].track);
      musicPlaying = true;
      lcd.clear();
      lcd.print("Playing Music:");
      lcd.setCursor(0,1);
      lcd.print(songs[i].display);
      return;
    }
  }

  bool turnOn = rest.endsWith(" on");
  bool turnOff = rest.endsWith(" off");

  if (rest == "open garage") { garageDoorServo.write(90); garageDoorOpen=true; waitForAudio(); myDFPlayer.play(16); return; }
  if (rest == "close garage") { garageDoorServo.write(0); garageDoorOpen=false; waitForAudio(); myDFPlayer.play(17); return; }

  String device = rest;
  if (turnOn || turnOff) device = rest.substring(0, rest.lastIndexOf(' '));

  if (device.indexOf("table")!=-1) setDevice(turnOn, relayTable, tableState, 1,2);
  else if (device.indexOf("bed")!=-1) setDevice(turnOn, relayBedroom, bedroomState, 3,4);
  else if (device.indexOf("reboot")!=-1) setDevice(turnOn, relayReboot, rebootState, 5,5);
  else if (device.indexOf("garage")!=-1) setDevice(turnOn, relayGarage, garageState, 6,7);
  else if (device.indexOf("security")!=-1) setDevice(turnOn, relaySecurity, securityState, 8,9);
  else if (device.indexOf("alarm")!=-1) setDevice(turnOn, relayAlarm, alarmState, 10,11);
  else if (device.indexOf("fan")!=-1) setDevice(turnOn, relayFan, fanState, 12,13);
  else if (device.indexOf("fence")!=-1) setDevice(turnOn, relayFence, fenceState, 14,15);
}

// ---------- LCD
void updateLCD() {
  if (musicPlaying) return;

  lcd.clear();
  int countOn = tableState + bedroomState + rebootState + garageState + securityState + alarmState + fanState + fenceState;

  lcd.setCursor(0,0);
  if (countOn == 0 && !garageDoorOpen) lcd.print("Hey Joe, All Off");
  else if (countOn >= 2) { lcd.print("Hey Joe, "); lcd.print(countOn); lcd.print(" ON"); }
  else {
    if(tableState) lcd.print("TableLight ON");
    else if(bedroomState) lcd.print("Bedroom ON");
    else if(rebootState) lcd.print("System reboot ON");
    else if(garageState) lcd.print("GarageLight ON");
    else if(securityState) lcd.print("Security ON");
    else if(alarmState) lcd.print("Alarm ON");
    else if(fanState) lcd.print("Fan ON");
    else if(fenceState) lcd.print("Fence ON");
  }

  lcd.setCursor(0,1);
  if(tableState) lcd.print("Tbl ");
  if(bedroomState) lcd.print("Bed ");
  if(rebootState) lcd.print("Reboot ");
  if(garageState) lcd.print("GarL ");
  if(securityState) lcd.print("Sec ");
  if(alarmState) lcd.print("Alm ");
  if(fanState) lcd.print("Fan ");
  if(fenceState) lcd.print("Fence ");
  if(garageDoorOpen) lcd.print("Garage OPEN");
  else lcd.print("Garage CLSD");
}
