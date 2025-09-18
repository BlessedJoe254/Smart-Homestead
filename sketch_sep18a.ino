#include <Servo.h>

// ------------------- AUTOMATIC DUSTBIN -------------------
Servo dustbinServo;
int dustbinServoPin = 3;

// Ultrasonic sensors for dustbin
const int trigBinPin = 22;   // detects the hand
const int echoBinPin = 23;
const int trigLevelPin = 24; // detects trash fill level
const int echoLevelPin = 25;

const int dustbinHeight = 8; // cm from sensor to floor of the bin
int dustbinLevel = 0;        // 0=empty, 1=25%, 2=50%, 3=75%, 4=full
bool lidOpen = false;

// ------------------- SMART PARKING -------------------
Servo entryServo;
Servo exitServo;
int entryServoPin = 4;
int exitServoPin = 5;

const int pirEntry1 = 26; // open entry gate
const int pirEntry2 = 27; // close entry gate
const int pirExit1  = 28; // open exit gate
const int pirExit2  = 29; // close exit gate

int totalSlots = 4;
int occupiedSlots = 0;
const int buzzerPin = 6;
bool parkingFullMsgShown = false;

bool pirEntry1Prev = false;
bool pirEntry2Prev = false;
bool pirExit1Prev = false;
bool pirExit2Prev = false;

bool entryGateOpen = false;
bool exitGateOpen = false;

// ------------------- SMART IRRIGATION -------------------
const int waterSensorPin = A0;
const int pumpRelayPin = 30; // active LOW relay

// ------------------- SMART TOILET -------------------
const int trigToilet = 31;
const int echoToilet = 32;
const int toiletPumpRelayPin = 33;

bool handPreviouslyDetected = false;
int flushStage = 0;

// ------------------- SETUP -------------------
void setup() {
  Serial.begin(9600);

  // Dustbin
  dustbinServo.attach(dustbinServoPin);
  pinMode(trigBinPin, OUTPUT);
  pinMode(echoBinPin, INPUT);
  pinMode(trigLevelPin, OUTPUT);
  pinMode(echoLevelPin, INPUT);
  dustbinServo.write(0);

  // Parking
  entryServo.attach(entryServoPin);
  exitServo.attach(exitServoPin);
  pinMode(pirEntry1, INPUT);
  pinMode(pirEntry2, INPUT);
  pinMode(pirExit1, INPUT);
  pinMode(pirExit2, INPUT);
  pinMode(buzzerPin, OUTPUT);
  entryServo.write(90);
  exitServo.write(90);

  pirEntry1Prev = (digitalRead(pirEntry1) == HIGH);
  pirEntry2Prev = (digitalRead(pirEntry2) == HIGH);
  pirExit1Prev  = (digitalRead(pirExit1)  == HIGH);
  pirExit2Prev  = (digitalRead(pirExit2)  == HIGH);

  // Irrigation
  pinMode(pumpRelayPin, OUTPUT);
  digitalWrite(pumpRelayPin, HIGH); // pump OFF

  // Toilet
  pinMode(trigToilet, OUTPUT);
  pinMode(echoToilet, INPUT);
  pinMode(toiletPumpRelayPin, OUTPUT);
  digitalWrite(toiletPumpRelayPin, HIGH); // pump OFF
}

// ------------------- HELPERS -------------------
long measureDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000);
  return (duration == 0) ? -1 : (duration * 0.034 / 2);
}

// ------------------- DUSTBIN -------------------
void controlDustbinLid() {
  long objectDist = measureDistance(trigBinPin, echoBinPin);   // hand
  long fillDist   = measureDistance(trigLevelPin, echoLevelPin); // trash level

  if (fillDist < 0) fillDist = dustbinHeight; // default if error

  // Map levels
  if (fillDist >= 5) dustbinLevel = 0;       // empty
  else if (fillDist >= 4) dustbinLevel = 1;  // 25%
  else if (fillDist >= 3) dustbinLevel = 2;  // 50%
  else if (fillDist >= 2) dustbinLevel = 4;  // 75%
  else if (fillDist >= 1) dustbinLevel = 3;  // 100%
  else dustbinLevel <= 1;                     // full (≤1cm)

  // Print status
  Serial.print("Dustbin distance: ");
  Serial.print(fillDist);
  Serial.print(" cm -> Level: ");
  switch (dustbinLevel) {
    case 0: Serial.println("Empty (0%)"); break;
    case 1: Serial.println("Quarter (25%)"); break;
    case 2: Serial.println("Half (50%)"); break;
    case 3: Serial.println("Three-Quarter (75%)"); break;
    case 4: Serial.println("FULL (100%)"); break;
  }

  // Control lid
  if (objectDist > 0 && objectDist <= 5 && dustbinLevel < 4) {
    if (!lidOpen) {
      dustbinServo.write(90);
      lidOpen = true;
      Serial.println("Dustbin lid opened");
    }
  } else {
    if (lidOpen) {
      dustbinServo.write(0);
      lidOpen = false;
      Serial.println("Dustbin lid closed");
    }
  }
}

// ------------------- PARKING -------------------
void controlParking() {
  bool e1 = (digitalRead(pirEntry1) == HIGH);
  bool e2 = (digitalRead(pirEntry2) == HIGH);
  bool x1 = (digitalRead(pirExit1)  == HIGH);
  bool x2 = (digitalRead(pirExit2)  == HIGH);

  if (e1 && !pirEntry1Prev) {
    if (occupiedSlots < totalSlots) {
      entryServo.write(0);
      entryGateOpen = true;
      Serial.println("Entrance gate opened");
    } else {
      digitalWrite(buzzerPin, HIGH); delay(200); digitalWrite(buzzerPin, LOW);
      if (!parkingFullMsgShown) {
        Serial.println("Parking FULL - Entry denied!");
        parkingFullMsgShown = true;
      }
    }
  }

  if (e2 && !pirEntry2Prev) {
    if (entryGateOpen && occupiedSlots < totalSlots) {
      entryServo.write(90);
      entryGateOpen = false;
      occupiedSlots++;
      Serial.print("Vehicle entered. Occupied: ");
      Serial.println(occupiedSlots);
    }
  }

  if (x1 && !pirExit1Prev) {
    if (occupiedSlots > 0) {
      exitServo.write(0);
      exitGateOpen = true;
      Serial.println("Exit gate opened");
    }
  }

  if (x2 && !pirExit2Prev) {
    if (exitGateOpen && occupiedSlots > 0) {
      exitServo.write(90);
      exitGateOpen = false;
      occupiedSlots--;
      Serial.print("Vehicle exited. Occupied: ");
      Serial.println(occupiedSlots);
      parkingFullMsgShown = false;
    }
  }

  if (occupiedSlots < 0) occupiedSlots = 0;
  if (occupiedSlots > totalSlots) occupiedSlots = totalSlots;

  pirEntry1Prev = e1;
  pirEntry2Prev = e2;
  pirExit1Prev  = x1;
  pirExit2Prev  = x2;
}

// ------------------- IRRIGATION -------------------
void controlIrrigation() {
  int waterLevel = analogRead(waterSensorPin);

  // thresholds (calibrated from your readings)
  const int dryThreshold = 800; // dry soil
  const int wetThreshold = 600; // wet soil

  static bool pumpOn = false;

  Serial.print("Soil moisture reading: ");
  Serial.println(waterLevel);

  if (!pumpOn && waterLevel > dryThreshold) {
    digitalWrite(pumpRelayPin, LOW);  // pump ON (active LOW relay)
    pumpOn = true;
    Serial.println("Irrigation Pump ON - Soil dry");
  }
  else if (pumpOn && waterLevel < wetThreshold) {
    digitalWrite(pumpRelayPin, HIGH); // pump OFF
    pumpOn = false;
    Serial.println("Irrigation Pump OFF - Soil wet");
  }

  if (pumpOn) {
    Serial.println("Pump Status: RUNNING");
  } else {
    Serial.println("Pump Status: OFF");
  }
}

// ------------------- TOILET -------------------
long getToiletDistance() {
  digitalWrite(trigToilet, LOW);
  delayMicroseconds(2);
  digitalWrite(trigToilet, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigToilet, LOW);
  long duration = pulseIn(echoToilet, HIGH, 30000);
  return (duration == 0) ? -1 : (duration * 0.034 / 2);
}

void controlToilet() {
  long distance = getToiletDistance();
  bool handDetected = (distance > 0 && distance < 10);

  if (handDetected && !handPreviouslyDetected) {
    if (flushStage == 0) {
      digitalWrite(toiletPumpRelayPin, LOW); delay(2000);
      digitalWrite(toiletPumpRelayPin, HIGH);
      Serial.println("Smart Toilet: Initial flush");
      flushStage = 1;
    } else {
      digitalWrite(toiletPumpRelayPin, LOW); delay(5000);
      digitalWrite(toiletPumpRelayPin, HIGH);
      Serial.println("Smart Toilet: After flush");
      flushStage = 0;
    }
  }
  handPreviouslyDetected = handDetected;
}

// ------------------- LOOP -------------------
void loop() {
  controlDustbinLid();
  controlParking();
  controlIrrigation();
  controlToilet();
  delay(200);
}