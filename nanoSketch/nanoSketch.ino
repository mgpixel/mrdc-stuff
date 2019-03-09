#define voltageRead A7
#define V_CENTER    177
#define MAX_TIMEOUT 10000
const int relaySwitch = 2;
const int armPin = 3;
const int pin1 = 5;
const int pin2 = 6;
const int readArmDataPin = A0;

float voltage = 0;
int armData = V_CENTER;
int timeout = 0;

void setup() {
  Serial.begin(9600);
  pinMode(relaySwitch, OUTPUT);
  pinMode(armPin, OUTPUT);
  pinMode(readArmDataPin, INPUT);
  pinMode(voltageRead, INPUT);
}

void loop() {
  voltage = analogRead(voltageRead);
  voltage = voltage*5/1023;
  Serial.print(voltage);
  if(voltage < 3.14) {
    Serial.println(" low ");
    digitalWrite(relaySwitch, LOW);
  }
  else {
    Serial.println(" high");
    digitalWrite(relaySwitch, HIGH);
  }
  int data = analogRead(readArmDataPin);
  if (data >= 110 && data <= 244) {
    armData = data; 
  }
  analogWrite(armPin, armData);
}
