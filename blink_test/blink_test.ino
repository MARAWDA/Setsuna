void setup() {
  Serial.begin(115200);
  pinMode(2, OUTPUT);
}

void loop() {
  Serial.println("alive");
  digitalWrite(2, !digitalRead(2));
  delay(500);
}
