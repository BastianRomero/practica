void setup() {
pinMode(7,OUTPUT);
pinMode(8,INPUT);
Serial.begin(9600);
}

void loop() {
  
int ON = digitalRead(8);

if(ON == LOW){
  digitalWrite(7,HIGH);
  Serial.println(ON);
}else{
    digitalWrite(7, LOW);
      Serial.println(ON);
}

  delay(500);

}
