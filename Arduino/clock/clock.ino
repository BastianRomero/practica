int ON = HIGH;
int OFF = HIGH;
int boton = 0;

void setup() {
pinMode(7,OUTPUT);
pinMode(8,INPUT);
pinMode(9,INPUT);
Serial.begin(9600);
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
  delay(5);
  digitalWrite(7,LOW);
  delay(5);
  Serial.println(ON);
  boton = 1;
}

}
