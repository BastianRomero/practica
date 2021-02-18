int ON = HIGH;
int OFF = HIGH;
int boton = 0;
long segundos = 1000000;
int frames = 500;


void setup() {
pinMode(7,OUTPUT);
pinMode(8,INPUT);
pinMode(9,INPUT);
Serial.begin(9600);
}

void loop() {
  
ON = digitalRead(8);
OFF = digitalRead(9);

/*if (OFF == LOW){
  Serial.println(OFF);
  boton = 0;
}*/

if(ON == LOW || boton == 1){
  digitalWrite(7,HIGH);
  delayMicroseconds((segundos/(frames*2)));
  digitalWrite(7,LOW);
  delayMicroseconds((segundos/(frames*2)));
  boton = 1;
  Serial.println(ON);
  
}
delay(1000/frames);
}
