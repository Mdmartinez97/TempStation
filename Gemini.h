// Consulta a Gemini IA 1.5 flash API
String Gemini(int &feelTemp){
  const char* Gemini_Max_Tokens = "100";
  String StrTemp = String(feelTemp);

  // Consulta vestimenta adecuada segun la sensación térmica, usando jerga argentina:
  String Prompt = "\"Afuera hace " + StrTemp + " °C. Cómo debo vestirme para salir si soy algo friolento? Responde en 30 caracteres como máximo usando jerga argentina, sin usar caracteres acentuados ni comas\"";

  HTTPClient https;

  if (https.begin("https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent?key=" + (String)Gemini_ApiKey)) {  // HTTPS

    https.addHeader("Content-Type", "application/json");
    String payload = String("{\"contents\": [{\"parts\":[{\"text\":" + Prompt + "}]}],\"generationConfig\": {\"maxOutputTokens\": " + (String)Gemini_Max_Tokens + "}}");
    
    //Serial.println(payload); // PROMPT DEBUG

    // start connection and send HTTP header
    int httpCode = https.POST(payload);


    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
      String payload = https.getString();
      
      DynamicJsonDocument doc(1024);

      deserializeJson(doc, payload);
      String Answer = doc["candidates"][0]["content"]["parts"][0]["text"];
      
      // Filtrar caracteres no deseados
      Answer.trim();
      String filteredAnswer = "";
      for (size_t i = 0; i < Answer.length(); i++) {
        char c = Answer[i];
        if (isalnum(c) || isspace(c)) {
          filteredAnswer += c;
        } else {
          filteredAnswer += ' ';
          }
      }
      Answer = filteredAnswer;

      return Answer;
    } else {
      Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }
    https.end();
  } else {
    Serial.printf("[HTTPS] Unable to connect\n");
    }
}