#define led1 2
#define led2 3

void setup() {
  Serial.begin(9600);
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);

  pinMode(A0, INPUT_PULLUP);
  pinMode(A1, INPUT_PULLUP);
}

void loop() {
  int sum1 = 0, sum2 = 0;
  // put your main code here, to run repeatedly:
  int dux1 = analogRead(A0);
  int dux2 = analogRead(A1);
  for (int i = 0; i < 1000; i++) {
    sum1 = sum1 + dux1;
    sum2 = sum2 + dux2;
  }
  sum1=sum1/1000;
  sum2=sum2/1000;

  if (sum1>sum2){
    digitalWrite(led1, HIGH);
    digitalWrite(led2, LOW);
  } else {
    digitalWrite(led1, LOW);
    digitalWrite(led2, HIGH);
  }

  Serial.print(sum1);
  Serial.print("\t");
  Serial.println(sum2);
}
