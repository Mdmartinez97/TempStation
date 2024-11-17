// Intervalo de consulta
unsigned long WTPrevMillis = 0;
unsigned long WTInterval = 1800000; // 30 minutos

// Función de consulta a Open Weather Map
void getWeatherData(int &Temp, int &feelTemp, float &lat, float &lon) {
  HTTPClient http; // Iniciación

  // URL web
  String url = "https://api.openweathermap.org/data/2.5/weather?lat=" + String(lat) + "&lon=" + String(lon) + "&appid=" + OpenWeatherMap_ApiKey + "&units=metric&lang=es";
  //Serial.println(url); // for DEBUG

  // Consulta a URL
  http.begin(url);
  int httpCode = http.GET();

  if(httpCode > 0) {
    String payload = http.getString();
    // Parseo de JSON
      StaticJsonDocument<1024> doc;
      DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
      return;
    }

    // Busca temperatura actual en el JSON
    Temp = doc["main"]["temp"];

    // Busca sensación térmica en el JSON
    feelTemp = doc["main"]["feels_like"];

  } else {
    Serial.println("Error en la solicitud HTTP");
  }

  http.end();
}