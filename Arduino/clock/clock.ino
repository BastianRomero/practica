int ON = HIGH;
int OFF = HIGH;
int boton = 0;

void setup() {
pinMode(7,OUTPUT);
pinMode(8,INPUT);
pinMode(9,INPUT);
Serial.begin(38400);
}

void loop() {
  
ON = digitalRead(8);
OFF = digitalRead(9);

if (OFF == LOW){
  Serial.println(OFF);
  boton = 0;
}

if(ON == LOW || boton == 1){
  digitalWrite(7,HIGH);
  delayMicroseconds(100);
  digitalWrite(7,LOW);
  delayMicroseconds(100);
  boton = 1;
  Serial.println(ON);

}

}
