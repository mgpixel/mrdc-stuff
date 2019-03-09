#define V_CENTER  177

static int expected = 0;
static int received = 0;
static unsigned char buf[4] = {V_CENTER, V_CENTER, V_CENTER};
static unsigned motor_l = 3, motor_r = 5, arm = A0;
static int timeout = 0;
static int micro_time = 0;
/**
 * buf[0] = left motor
 * buf[1] = right motor
 * buf[2] = arm
 */
void setup() {
  pinMode(motor_l, OUTPUT);
  //pinMode(motor_r, OUTPUT);
  pinMode(A0, OUTPUT);
  micro_time = micros();
  Serial.begin(9600);
}

void loop() {
  if (Serial.available()) {
    int data = Serial.read();
    if (received == 3) {
      received = 0;
    }
    else if (expected) {
      expected--;
      received++;
      buf[received] = data;
    }
    else if (data == 0x01) {
      expected = 3;
    }
    timeout = 0;
    // Ignore dropped packets, should go back to normal after a bit
  } else {
    timeout++;
  }
  // If we lose connection to laptop, emergency shutoff
  if (timeout > 100000) {
    Serial.println("TIMED OUT");
    buf[0] = V_CENTER;
    buf[1] = V_CENTER;
    buf[2] = V_CENTER;
  }
  analogWrite(motor_l, buf[1]);
  analogWrite(motor_r, buf[1]);
  analogWrite(A0, buf[1]);
//  if () {
//    digitalWrite(arm, HIGH);
//  } else {
//    digitalWrite(arm, LOW);
//  }
}
