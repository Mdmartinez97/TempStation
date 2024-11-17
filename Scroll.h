// Función de cambio de pantalla con botón táctil
int Scroll(){
  int N_opciones = 3;
  static int opcion;
  int touchValue = touchRead(touchPin);
  //Serial.println(touchValue);

  const int touchThreshold = 80; // Valor umbral de sensibilidad touchPin
  if (touchValue < touchThreshold) {
    digitalWrite(touchLED, HIGH);  // Enciende el LED
      if(opcion < N_opciones - 1){
        opcion++;
      } else {
        opcion=0;
        }
    
  } else {
    digitalWrite(touchLED, LOW);   // Apaga el LED
    }
  delay(200);

  return opcion;
}