/* walks through a charlieplex, lighting each pixel in turn */

#define START_PIN 2
#define END_PIN 12


void setup(){ 
  Serial.begin(9600);
  for(int i = START_PIN; i<=END_PIN; i++) {
    pinMode(i, INPUT);
  }
  

/*nothing*/ 
}

void loop(){
  for(int out = START_PIN; out<=END_PIN; out++) {
    for(int in = START_PIN; in<=END_PIN; in++) {
      for(int i = START_PIN; i<=END_PIN; i++) {
	pinMode(i, INPUT);
      }
      if(out == in) { continue; }
      Serial.print(out);
      Serial.print(" ");
      Serial.println(in);
      pinMode(out, OUTPUT);
      digitalWrite(out, HIGH);
      pinMode(in, OUTPUT);
      digitalWrite(in, LOW);
      delay(500);
    }
  }
}
