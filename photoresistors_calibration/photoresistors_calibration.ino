const int pins[4] = {6, 5, 4, 7}; // TL, TR, BL, BR
const char* names[4] = {"TL", "TR", "BL", "BR"};

const int N = 1000;      // letture per caso
const int MAX_CASES = 4; // numero di casi di luce

unsigned long sum[4];
float caseValues[MAX_CASES][4];

void setup() {
  Serial.begin(115200);
  for (int i=0; i<4; i++) pinMode(pins[i], INPUT);

  Serial.println("Inizio calibrazione sensori (allineamento a media).");
  Serial.println("Per ogni livello di luce, scrivi 'ok' per iniziare la misurazione.");

  // 1. misurazioni
  for (int c=0; c<MAX_CASES; c++){
    // aspetta conferma
    String input = "";
    while(input != "ok") {
      if(Serial.available()){
        input = Serial.readStringUntil('\n');
        input.trim();
      }
    }

    Serial.printf("Caso %d: misurazione in corso...\n", c+1);

    // reset somme
    for(int i=0;i<4;i++) sum[i]=0;

    // campionamento
    for(int k=0;k<N;k++){
      for(int i=0;i<4;i++){
        sum[i] += analogRead(pins[i]);
      }
      delay(1);
    }

    // calcola medie
    float target = 0.0;
    for(int i=0;i<4;i++){
      caseValues[c][i] = sum[i]/(float)N;
      target += caseValues[c][i];
    }
    target /= 4.0; // media dei 4 sensori

    Serial.print("Medie sensori: ");
    for(int i=0;i<4;i++){
      Serial.printf("%s: %.1f   ", names[i], caseValues[c][i]);
    }
    Serial.printf("   target: %.1f\n", target);
  }

  // 2. Calcolo gain e offset
  float gain[4], offset[4];

  for(int i=0;i<4;i++){
    float xMean=0, yMean=0, sumXY=0, sumX2=0;

    // target = media dei sensori per ogni caso
    for(int c=0;c<MAX_CASES;c++){
      float target = 0;
      for(int j=0;j<4;j++) target += caseValues[c][j];
      target /= 4.0;

      xMean += caseValues[c][i];
      yMean += target;
    }
    xMean /= MAX_CASES;
    yMean /= MAX_CASES;

    for(int c=0;c<MAX_CASES;c++){
      float target = 0;
      for(int j=0;j<4;j++) target += caseValues[c][j];
      target /= 4.0;

      float dx = caseValues[c][i] - xMean;
      float dy = target - yMean;
      sumXY += dx*dy;
      sumX2 += dx*dx;
    }

    gain[i] = sumXY / sumX2;
    offset[i] = yMean - gain[i]*xMean;
  }

  // 3. Stampa valori pronti per LightControl
  Serial.println("\nValori per LightControl (allineati a media):");
  for(int i=0;i<4;i++){
    Serial.printf("%s: gain = %.4f, offset = %.2f\n", names[i], gain[i], offset[i]);
  }

  Serial.println("\nCalibrazione completata!");
}

void loop() {
}
