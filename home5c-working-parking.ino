#include <Servo.h>

// ------------------- AUTOMATIC DUSTBIN -------------------
Servo dustbinServo;
int dustbinServoPin = 3;

// Ultrasonic sensors for dustbin
const int trigBinPin = 22;
const int echoBinPin = 23;
const int trigLevelPin = 24;
const int echoLevelPin = 25;

const int dustbinHeight = 8; // cm
int dustbinLevel = 0;
bool lidOpen = false;

// ------------------- SMART PARKING -------------------
Servo entryServo;
Servo exitServo;
int entryServoPin = 4;
int exitServoPin = 5;

const int pirEntry1 = 26; // opens entry gate
const int pirEntry2 = 27; // closes entry gate & increments
const int pirExit1 = 28;  // opens exit gate
const int pirExit2 = 29;  // closes exit gate & decrements

int totalSlots = 4;
int occupiedSlots = 0;
const int buzzerPin = 6;
bool parkingFullMsgShown = false;

// PIR previous states
bool pirEntry1Prev = false;
bool pirEntry2Prev = false;
bool pirExit1Prev = false;
bool pirExit2Prev = false;

// Gate open flags
bool entryGateOpen = false;
bool exitGateOpen = false;

// ------------------- SMART IRRIGATION -------------------
const int waterSensorPin = A0;
const int pumpRelayPin = 30; // active LOW relay

// ------------------- SMART TOILET -------------------
const int trigToilet = 31;
const int echoToilet = 32;
const int toiletPumpRelayPin = 33; // active LOW relay

bool handPreviouslyDetected = false;
int flushStage = 0; // 0 = none yet, 1 = small flush done

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
  digitalWrite(pumpRelayPin, HIGH);

  // Toilet
  pinMode(trigToilet, OUTPUT);
  pinMode(echoToilet, INPUT);
  pinMode(toiletPumpRelayPin, OUTPUT);
  digitalWrite(toiletPumpRelayPin, HIGH);
}

// ------------------- HELPERS -------------------
long measureDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000);
  return (duration == 0) ? 0 : (duration * 0.034 / 2);
}

// ------------------- DUSTBIN -------------------
void controlDustbinLid() {
  long objectDist = measureDistance(trigBinPin, echoBinPin);
  long fillDist = measureDistance(trigLevelPin, echoLevelPin);

  if (fillDist > dustbinHeight || fillDist <= 0) dustbinLevel = 0;
  else if (fillDist > dustbinHeight * 0.75) dustbinLevel = 1;
  else if (fillDist > dustbinHeight * 0.5) dustbinLevel = 2;
  else if (fillDist > dustbinHeight * 0.25) dustbinLevel = 3;
  else dustbinLevel = 4;

  if (objectDist > 0 && objectDist <= 5 && dustbinLevel < 4) {
    if (!lidOpen) { dustbinServo.write(90); lidOpen = true; }
  } else {
    if (lidOpen) { dustbinServo.write(0); lidOpen = false; }
  }
}

// ------------------- PARKING -------------------
void controlParking() {
  bool e1 = (digitalRead(pirEntry1) == HIGH);
  bool e2 = (digitalRead(pirEntry2) == HIGH);
  bool x1 = (digitalRead(pirExit1)  == HIGH);
  bool x2 = (digitalRead(pirExit2)  == HIGH);

  // Entrance open
  if (e1 && !pirEntry1Prev) {
    if (occupiedSlots < totalSlots) {
      entryServo.write(0);
      entryGateOpen = true;
      Serial.println("Entrance gate opened");
    } else {
      digitalWrite(buzzerPin, HIGH); delay(200); digitalWrite(buzzerPin, LOW);
      if (!parkingFullMsgShown) {
        Serial.println("Parking FULL - Entry blocked!");
        parkingFullMsgShown = true;
      }
    }
  }

  // Entrance close & increment
  if (e2 && !pirEntry2Prev) {
    if (entryGateOpen && occupiedSlots < totalSlots) {
      entryServo.write(90);
      entryGateOpen = false;
      occupiedSlots++;
      Serial.print("Car entered. Occupied: ");
      Serial.println(occupiedSlots);
    }
  }

  // Exit open
  if (x1 && !pirExit1Prev) {
    if (occupiedSlots > 0) {
      exitServo.write(0);
      exitGateOpen = true;
      Serial.println("Exit gate opened");
    }
  }

  // Exit close & decrement
  if (x2 && !pirExit2Prev) {
    if (exitGateOpen && occupiedSlots > 0) {
      exitServo.write(90);
      exitGateOpen = false;
      occupiedSlots--;
      Serial.print("Car exited. Occupied: ");
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
  if (waterLevel > 500) {
    digitalWrite(pumpRelayPin, LOW);
    Serial.println("Irrigation Pump ON - Soil dry");
  } else {
    digitalWrite(pumpRelayPin, HIGH);
    Serial.println("Irrigation Pump OFF - Soil wet");
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
  return (duration == 0) ? 0 : (duration * 0.034 / 2);
}

void controlToilet() {
  long distance = getToiletDistance();
  bool handDetected = (distance > 0 && distance < 10);

  if (handDetected && !handPreviouslyDetected) {
    if (flushStage == 0) {
      digitalWrite(toiletPumpRelayPin, LOW); delay(2000);
      digitalWrite(toiletPumpRelayPin, HIGH);
      Serial.println("Smart Toilet: Small flush");
      
      flushStage = 1;
    } else {
      digitalWrite(toiletPumpRelayPin, LOW); delay(5000);
      digitalWrite(toiletPumpRelayPin, HIGH);
      Serial.println("Smart Toilet: Full flush");
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
  delay(150);
}
