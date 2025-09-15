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

const int pirEntry1 = 26;
const int pirEntry2 = 27;
const int pirExit1 = 28;
const int pirExit2 = 29;

int totalSlots = 4;
int occupiedSlots = 0;
const int buzzerPin = 6;

// ------------------- SMART IRRIGATION -------------------
const int waterSensorPin = A0;
const int pumpRelayPin = 30; // active LOW relay

// ------------------- SMART TOILET -------------------
const int trigToilet = 31;
const int echoToilet = 32;
const int toiletPumpRelayPin = 33; // active LOW relay

bool toiletFirstDetected = false;

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
  entryServo.write(90);  // closed
  exitServo.write(90);   // closed

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
  return duration * 0.034 / 2;
}

// ------------------- DUSTBIN -------------------
void controlDustbinLid() {
  long objectDist = measureDistance(trigBinPin, echoBinPin);
  long fillDist = measureDistance(trigLevelPin, echoLevelPin);

  // Map fill distance to bin level
  if (fillDist > dustbinHeight || fillDist <= 0) dustbinLevel = 0;       // empty
  else if (fillDist > dustbinHeight * 0.75) dustbinLevel = 1;            // quarter
  else if (fillDist > dustbinHeight * 0.5) dustbinLevel = 2;             // half
  else if (fillDist > dustbinHeight * 0.25) dustbinLevel = 3;            // three-quarter
  else dustbinLevel = 4;                                                 // full

  Serial.print("Dustbin level: "); Serial.println(dustbinLevel);

  // Control lid
  if (objectDist <= 5 && dustbinLevel < 4) {
    if (!lidOpen) {
      dustbinServo.write(90); // open
      lidOpen = true;
    }
  } else {
    if (lidOpen) {
      dustbinServo.write(0); // close
      lidOpen = false;
    }
  }
}

// ------------------- PARKING -------------------
void controlParking() {
  bool entryDetected = digitalRead(pirEntry1) || digitalRead(pirEntry2);
  bool exitDetected = digitalRead(pirExit1) || digitalRead(pirExit2);

  if (entryDetected) {
    if (occupiedSlots < totalSlots) {
      entryServo.write(0);   // open
      delay(2000);
      entryServo.write(90);  // close
      occupiedSlots++;
    } else {
      digitalWrite(buzzerPin, HIGH);
      delay(500);
      digitalWrite(buzzerPin, LOW);
    }
  }

  if (exitDetected && occupiedSlots > 0) {
    exitServo.write(0);
    delay(2000);
    exitServo.write(90);
    occupiedSlots--;
  }

  Serial.print("Occupied slots: "); Serial.println(occupiedSlots);
}

// ------------------- IRRIGATION -------------------
void controlIrrigation() {
  int waterLevel = analogRead(waterSensorPin);

  if (waterLevel > 500) { // dry
    digitalWrite(pumpRelayPin, LOW);  // pump ON
    Serial.println("Irrigation Pump ON - Soil dry");
  } else {
    digitalWrite(pumpRelayPin, HIGH); // pump OFF
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
  return duration * 0.034 / 2;
}

void controlToilet() {
  long distance = getToiletDistance();

  if (distance > 0 && distance < 10) {
    if (!toiletFirstDetected) {
      // First detection → small flush
      digitalWrite(toiletPumpRelayPin, LOW);
      delay(2000);
      digitalWrite(toiletPumpRelayPin, HIGH);

      Serial.println("Smart Toilet: Small flush");
      toiletFirstDetected = true;
      delay(2000);
    } else {
      // Second detection → full flush
      digitalWrite(toiletPumpRelayPin, LOW);
      delay(5000);
      digitalWrite(toiletPumpRelayPin, HIGH);

      Serial.println("Smart Toilet: Full flush");
      toiletFirstDetected = false; // reset
      delay(2000);
    }
  }
}

// ------------------- LOOP -------------------
void loop() {
  controlDustbinLid();
  controlParking();
  controlIrrigation();
  controlToilet();
  delay(500);
}
