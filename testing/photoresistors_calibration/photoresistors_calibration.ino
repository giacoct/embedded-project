const int tl = 6;
const int tr = 5;
const int bl = 4;
const int br = 7;

unsigned int reads[4];
unsigned int val[4];
unsigned long cont = 0;

void setup() {
  Serial.begin(115200);

  pinMode(tl, INPUT);
  pinMode(tr, INPUT);
  pinMode(bl, INPUT);
  pinMode(br, INPUT);

  reads[0] = 0;
  reads[1] = 0;
  reads[2] = 0;
  reads[3] = 0;
}

void loop() {
  reads[0] = reads[0] + analogRead(tl);
  reads[1] = reads[1] + analogRead(tr);
  reads[2] = reads[2] + analogRead(bl);
  reads[3] = reads[3] + analogRead(br);
  cont++;

  for (int i=0; i<4; i++){
    val[i] = reads[i]/cont;
  }
  Serial.printf("tl: %d   tr: %d   bl: %d   br: %d \n", val[0], val[1], val[2], val[3]);
  delay(1);
}
