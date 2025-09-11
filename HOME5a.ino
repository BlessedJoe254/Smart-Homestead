#include <Servo.h>

// ------------------- AUTOMATIC DUSTBIN -------------------

// Dustbin servo
Servo dustbinServo;
int dustbinServoPin = 3;

// Ultrasonic sensors for dustbin
const int trigBinPin = 4;
const int echoBinPin = 5;       // For object detection (lid)
const int trigLevelPin = 6;
const int echoLevelPin = 7;     // For fill level detection

// Dustbin variables
const int dustbinHeight = 8; // in cm
int dustbinLevel = 0; // 0: empty, 1: quarter, 2: half, 3: three-quarter, 4: full
bool lidOpen = false;

// ------------------- SMART PARKING -------------------

// Parking servos
Servo entryServo;
Servo exitServo;
int entryServoPin = 8;
int exitServoPin = 9;

// PIR sensors
const int pirEntry1 = 10;
const int pirEntry2 = 11;
const int pirExit1 = 12;
const int pirExit2 = 13;

// Parking variables
int totalSlots = 4;
int occupiedSlots = 0;
const int buzzerPin = 2;

// ------------------- SMART IRRIGATION -------------------

// Water/soil sensor
const int waterSensorPin = A0;
const int pumpRelayPin = 7; // Relay connected to pump

// ------------------- SETUP -------------------
void setup() {
  Serial.begin(9600);

  // Dustbin setup
  dustbinServo.attach(dustbinServoPin);
  pinMode(trigBinPin, OUTPUT);
  pinMode(echoBinPin, INPUT);
  pinMode(trigLevelPin, OUTPUT);
  pinMode(echoLevelPin, INPUT);

  // Parking setup
  entryServo.attach(entryServoPin);
  exitServo.attach(exitServoPin);
  pinMode(pirEntry1, INPUT);
  pinMode(pirEntry2, INPUT);
  pinMode(pirExit1, INPUT);
  pinMode(pirExit2, INPUT);
  pinMode(buzzerPin, OUTPUT);

  // Irrigation setup
  pinMode(waterSensorPin, INPUT);
  pinMode(pumpRelayPin, OUTPUT);

  // Initialize servos
  dustbinServo.write(0);
  entryServo.write(0);  // Closed
  exitServo.write(0);   // Closed
}

// ------------------- HELPER FUNCTIONS -------------------

// Measure distance from ultrasonic sensor
long measureDistance(int trigPin, int echoPin){
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH);
  long distance = duration * 0.034 / 2;
  return distance;
}

// Dustbin lid control
void controlDustbinLid() {
  long objectDist = measureDistance(trigBinPin, echoBinPin);
  long fillDist = measureDistance(trigLevelPin, echoLevelPin);
  
  // Determine fill level
  if(fillDist >= dustbinHeight*0.75) dustbinLevel = 0; // empty
  else if(fillDist >= dustbinHeight*0.5) dustbinLevel = 1; // quarter
  else if(fillDist >= dustbinHeight*0.25) dustbinLevel = 2; // half
  else if(fillDist > 0) dustbinLevel = 3; // three-quarter
  else dustbinLevel = 4; // full

  Serial.print("Dustbin level: ");
  Serial.println(dustbinLevel);

  // Control lid
  if(objectDist <= 5 && dustbinLevel < 4) {
    if(!lidOpen) {
      dustbinServo.write(90);
      lidOpen = true;
    }
  } else {
    if(lidOpen) {
      dustbinServo.write(0);
      lidOpen = false;
    }
  }
}

// Smart parking control
void controlParking() {
  bool entryDetected = digitalRead(pirEntry1) || digitalRead(pirEntry2);
  bool exitDetected = digitalRead(pirExit1) || digitalRead(pirExit2);

  // Entry logic
  if(entryDetected) {
    if(occupiedSlots < totalSlots) {
      entryServo.write(90); // Open
      delay(2000);
      entryServo.write(0);  // Close
      occupiedSlots++;
    } else {
      digitalWrite(buzzerPin, HIGH); // Parking full
      delay(500);
      digitalWrite(buzzerPin, LOW);
    }
  }

  // Exit logic
  if(exitDetected && occupiedSlots > 0) {
    exitServo.write(90); // Open
    delay(2000);
    exitServo.write(0);  // Close
    occupiedSlots--;
  }

  Serial.print("Occupied slots: ");
  Serial.println(occupiedSlots);
}

// Smart irrigation control
void controlIrrigation() {
  int waterLevel = analogRead(waterSensorPin);

  if(waterLevel < 500) { // adjust threshold depending on sensor
    digitalWrite(pumpRelayPin, HIGH); // Pump ON
    Serial.println("Pump ON - Soil dry");
  } else {
    digitalWrite(pumpRelayPin, LOW);  // Pump OFF
    Serial.println("Pump OFF - Soil wet");
  }
}

// ------------------- LOOP -------------------
void loop() {
  controlDustbinLid();
  controlParking();
  controlIrrigation();
  delay(500); // Small delay for stability
}
