# "TimeStation: Reloj + Termómetro + IA"

## 📋 INTRODUCCIÓN

El proyecto TempStation trata sobre la creación de un dispositivo capaz de mostrar en un display la fecha y hora, la temperatura medida en una habitación, la temperatura exterior en una localización determinada y una frase relacionada generada con IA.

El usuario podrá configurar inicialmente el dispositivo mediante una interfaz web, donde ingresará las credenciales para conectarse a una red WiFi, las coordenadas donde se desea obtener la temperatura exterior y su uso horario.

La información presentada en la pantalla podrá ser recorrida mediante un comando táctil.

## ⚙️ HARDWARE

Se utilizó un microcontrolador ESP32 en su versión DEVKIT programado en Arduino IDE (C++), un sensor de temperatura DS18B20 y un display LCD 16x2 con comunicación I2C.

### Cableado🔌
| GPIO | Nombre | Propósito |
| :----- | :-----|:-----|
| GPIO 21 | LCD SDA | Comunicación I2C con Display |
| GPIO22 | LCD SCL | Comunicación I2C con Display |
| GPIO5 | SensorPin | DS18B20 data pin |
| GPIO4 | touchPin | Desplazamiento de pantalla |
| GPIO2 | touchLED | LED indicador touch |
| GPIO15 | RstWF | Reset configuración Wi-Fi |

<img src="./img/wiring.jpg" height="300">

### Gabinete💡

El producto final fue ideado como un reloj digital de pared que pueda ser colgado y quedar a la vista del usuario para su manipulación, pero también puede diseñarse para ser utilizado sobre un escritorio o como un dispositivo portátil.

La carcasa aún no fue diseñada.

## 📀 FIRMWARE

### WiFi Manager + Custom Parameters🛜


Se utiliza la librería [WiFiManager de tzapu](https://github.com/tzapu/WiFiManager). Para saber cómo implementarla recomiendo ver los ejemplos dentro de su repositorio.

Cuando el dispositivo inicia por primera vez (o se reestablece la configuración), se crea un punto de acceso WiFi local al que podemos conectarnos y, través de una interfaz web, elegir la red WiFi a la que deseamos conectarnos, introduciendo allí su contraseña para guardarla en la memoria del dispositivo. De esta manera, en los siguientes encendidos, se conectará automáticamente a la red configurada sin necesidad de volver a realizar el proceso. Además, se utiliza la capacidad de la librería de agregar parámetros personalizados a esta interfaz, para solicitar al usuario datos de su ubicación que serán usados para obtener fecha, hora y clima.

La interfaz de usuario queda de la siguiente manera:

<img src="./img/wm1.jpg" height="300">

Tras ingresar a la ocpión "Configure WiFi" se pedirán también datos sobre la ubicación, configurados de la siguiente manera:
```c++
void CustomWiFiManager(){
  // [...]
  WiFiManagerParameter custom_text("<p>Ingrese los datos de la ubicación deseada</p>"); //Texto HTML al pie
  wifiManager.addParameter(&custom_text);

  // Crea formulario en WM
  WiFiManagerParameter custom_param1("param1", "Longuitud (xx.xx):", 0, 6); // ID, label, default value, legth
  WiFiManagerParameter custom_param2("param2", "Latitud (xx.xx):", 0, 6);
  WiFiManagerParameter custom_param3("param3", "UTC (±xx):", 0, 3);
 
  // Obtener parametros ingresados en WM
  wifiManager.addParameter(&custom_param1);
  wifiManager.addParameter(&custom_param2);
  wifiManager.addParameter(&custom_param3);
  // [...]
}
```

<img src="./img/wm2.jpg" height="283">

Los datos de Longitud, Latitud y UTC serán guardados en la memoria EEPROM en las direcciones `address_lat`, `address_lon` y `address_utc`.

```c++
void CustomWiFiManager(){
  // [...]
  // Guardar parámetros de WM en variables globales
  String parameter1 = custom_param1.getValue();
  String parameter2 = custom_param2.getValue();
  String parameter3 = custom_param3.getValue();

  // Convertir datos a Float e Int
  lat = parameter1.toFloat();
  lon = parameter2.toFloat();
  utc = parameter3.toInt();

  // Guardar datos en la EEPROM si la bandera es true
  if (shouldSaveConfig) {
    EEPROM.put(address_lat, lat);
    EEPROM.put(address_lon, lon);
    EEPROM.put(address_utc, utc);
    EEPROM.commit();
  }
  // [...]
}
```
 Luego serán consultadas por el código para establecer la ubicación.
```c++
void setup(){
  // [...]
  // Buscar datos guardados
  EEPROM.get(address_lat, lat);
  EEPROM.get(address_lon, lon);
  EEPROM.get(address_utc, utc);
  // [...]
}
  ```
Para asegurarnos que los datos ingresados por el usuario permanezcan almacenados en la memoria del microcontrolador tras su reinicio, es necesario implementar una bandera dentro de la función que se alze si los datos han sido actualizados, para asi guardarlos y permanezca baja si los datos no han sufrido cambios, para así no sobreescribirlos con su valor por defecto.

```c++
// Función WiFi Manager Custom Parameters
bool shouldSaveConfig = false; // Bandera para guardar datos

void saveConfigCallback() {
  // Bandera true si se actualizan los datos de WM
  shouldSaveConfig = true;
}

void CustomWiFiManager(){
  // [...]
  wifiManager.setSaveConfigCallback(saveConfigCallback); // Llama a la función si se actualizaron los datos
  // [...]
  }
```

Por último, para reiniciar los ajustes WiFi y que el proceso WiFi Manager se inicie nuevamente, permitiendo así reubicar el dispositivo en otra red o ubicación geográfica, se dispone del botón pulsador `RstWF`.

```c++
void loop(){
  // [...]
  //BOTÓN DE RESET WIFI
  if(digitalRead(RstWF) == LOW){
    delay(50);
      if(digitalRead(RstWF) == LOW){
        wifiManager.resetSettings();
        Serial.println("---CREDENCIALES WIFI BORRADAS---");
        lcd.clear();
        lcd.backlight();
        lcd.setCursor(2, 0);
        lcd.print("WiFi RESET...");
        delay(1500);
        lcd.clear();
        ESP.restart();
      }
  }
  // [...]
}
```

### Fecha y Hora  (NTP server)🕒

Se utiliza la librería `Time.h` para obtener la fecha y hora basadas en el uso horario especificado por el usuario.
En el `setup()` se debe realizar la configuración inicial y luego llamar a nuestra función `TimeData()` para que se aplique.

```c++
void setup(){
 // [...]
 // Configurar fecha y hora
  configTime(utc*60*60, 0, "pool.ntp.org"); // UTC expresado en segundos
  TimeData();
 // [...]
}
```

La función `TimeData()` consulta al servidor día, mes y año y guarda los datos en la entructura `time[]`.

```c++
//Obtener Fecha y Hora desde NTP Server
String TimeData() {
 struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
  }
  char time[20];
  strftime(time, sizeof(time), "%d/%m/%y   %H:%M", &timeinfo);
  
  return time;
}
```

Esta será consultada desde el `loop()` cada 1 minuto, según intervalo predefinido `NtpInterval`.

### Medición de temperatura ambiente🏠

La librería `OneWire.h` Proporciona la comunicación básica a nivel de bits con el sensor, mientras que `DallasTemperature.h` Se encarga de los comandos específicos del DS18B20 y de convertir los datos en una temperatura legible.

Utilizando ambas librerías en conjunto, se crean objetos de las correspondientes clases para manejar la comunicación con el sensor.

```c++
void loop(){
 // [...]
// Consultar Sensor y API's
  if (millis() - WTPrevMillis > WTInterval || WTPrevMillis == 0){

  // Medir Temperatura interior
  sensors.requestTemperatures();
  localTemp = sensors.getTempCByIndex(0); // Consultar sensor #0   
      Serial.print("Local Temp: ");      
      Serial.println(localTemp);
  // [...]
 }
// [...]
}
```

La función para obtener la temperatura será consultada en un intervalo predefinido de 30 minutos llamado `WTInterval`.

### OpenWeatherMap API🌅

Para consultar la API es necesario primero crear una cuenta en [OpenWeatherMap](https://openweathermap.org/api) y obtener la llave de acceso. Las cuentas gratuitas son suficientes para este proyecto.

Se utilizan las librerías `HTTPClient.h` para realizar la consulta y `ArduinoJson.h` para acondicionar los datos obtenidos.

La URL a consultar está compuesta por los datos solicitados anteriormente al usuario, Longitud, Latitud y UTC. Se incluye también nuestra ApiKey y el idioma y unidades de medida preferidas.

```c++
void getWeatherData(){
  // [...]
  // URL web
  String url = "https://api.openweathermap.org/data/2.5/weather?lat=" + String(lat) + "&lon=" + String(lon) + "&appid=" + apiKey + "&units=metric&lang=es";

  // Consulta a URL
  http.begin(url);
  int httpCode = http.GET();

  if(httpCode > 0) {
    String payload = http.getString();
    // Parseo de JSON
      StaticJsonDocument<1024> doc;
      DeserializationError error = deserializeJson(doc, payload);
  // [...]
  // Busca temperatura actual en el JSON
  Temp = doc["main"]["temp"];
  // Busca sensación térmica en el JSON
  feelTemp = doc["main"]["feels_like"];
  // [...]
  }
// [...]
}
```
Luego de realizar la consulta se extraen los campos requeridos mediante la función `deserializeJson()`, en nuestro caso, la temperatura en `Temp` y la sensación térmica en `feels_like` para luego mostrarlos en el display.

Esta función también será consultada según el intervalo `WTInterval`.

### Gemini IA API🤖✨

Para consultar esta API será necesario registrarnos en [Google AI Studio](https://aistudio.google.com/app/apikey) y generar una llave de acceso.

En nuestro `prompt` indicaremos la temperatura actual que consultamos en OpenWeatherMap y le pediremos a la IA que nos recomiende qué ropa usar para salir.

Tras varias pruebas, se obtuvieron los mejores resultados con el siguiente `prompt`:

```c++
String Prompt = "\"Afuera hace " + StrTemp + " °C. Cómo debo vestirme para salir si soy algo friolento? Responde en 30 caracteres como máximo usando jerga argentina, sin usar caracteres acentuados ni comas\"";
```

Luego, en nuestra función `Gemini()` armamos la consulta a la API indicando el `prompt` y la cantidad máxima de tokens a procesar, definida como constante en nuestro código. Tras recibir la respuesta en formato JSON, procesamos su contenido al igual que hicimos con la consulta a la API de clima. Será necesario también, filtrar la respuesta para evitar caracteres especiales, saltos de línea y espacios en blanco.

```c++
void Gemini(){
// [...]
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
// [...]
}
```

Por último, dividimos la respuesta en dos renglones para poder mostrarla en ambas filas de la pantalla LCD.

```c++
void loop(){
// [...]
  // Generar comentario Gemini AI
    String Clothes = Gemini();
    Serial.print("Comentario Gemini: ");      
    Serial.println(Clothes);

    // Dividir frease en 2 filas
    Clothes1 = Clothes.substring(0, 15);
    Clothes2 = Clothes.substring(15);

    lcd.setCursor(0, 0);
    lcd.print(Clothes1);
    lcd.setCursor(0, 1);
    lcd.print(Clothes2);
// [...]
}
```
Esta función también será consultada según el intervalo `WTInterval`.

### Pantalla LCD y Scroll Touch 🖥️👈

El programa actualizará la información según los intervalos `NtpInterval` (1 minuto, Fecha y Hora ) y `WTInterval` (30 minutos, sensor de temperatura, API de clima y API Gemini). Se implementa para ello la función `millis()` dentro del `Loop()` para cada uno de los intervalos definidos (se muestra solo uno como ejemplo).

```c++
void loop(){
// [...]
  // Consultar NTP Server
  if (millis() - NtpPrevMillis > NtpInterval || NtpPrevMillis == 0){
  
    // Consultar Fecha y Hora
    timestamp = TimeData();
      Serial.print("Timestamp: ");      
      Serial.println(timestamp);

    NtpPrevMillis = millis();
  }
// [...]
}
```

Para mostrar toda la información, se divide en tres categorías: Temperaturas, Fecha/Hora y comentario IA. Se utiliza una pantalla LCD 16x2, lo que nos limita a un espacio de escritura de 32 caracteres en total.

El desplazamiento entre las tres categorías de información es comandado por el usuario mediante un pulsador táctil, aprovechando dicha capacidad integrada del ESP32. Para ello se implementa nuestra función `Scroll()`, donde leemos el valor digital que devuelve `touchPin` y verificamos si es menor a nuestro valor umbral definido. En caso positivo, incrementamos un contador con el número de la pantalla a mostrar.

```c++
// Función de cambio de pantalla con botón táctil
int Scroll(){
  int N_opciones = 3;
  static int opcion;
  int touchValue = touchRead(touchPin);

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
```

Dentro del ciclo `loop()` se utiliza un condicional switch-case que mostrará la información requerida según el número de pantalla especificado por el usuario. Tras mostrar la información, la pantalla permanecerá encendida 10 segundos y luego apagará su iluminación, utilizando para ello también la función `millis()` con un intervalo de tiempo especificado llamado `LCDInterval`.

## 🚀 MEJORAS

Planteo las siguientes mejoras como útiles y necesarias para el futuro del proyecto:

- Hardware: Utilizar una pantalla mas moderna que permita mostrar la información más clara y detallada.

- Hardware: Diseñar y construir una carcasa.

- Firmware: Dividir el código principal en archivos individuales para cada función personalizada.

## 📜 CRÉDITOS

**[ESP32 NTP Client-Server: Get Date and Time | Random Nerd Tutorials](https://randomnerdtutorials.com/esp32-date-time-ntp-client-server-arduino/)**
Instructivo para obtener Fecha y Hora desde un servidor NTP en ESP32


**[Running Gemini AI on ESP32 | Techiesms](https://www.youtube.com/watch?v=2nL46VIrwQM)**
Instructivo para implementar un chat con Gemini IA en ESP32

**[Example for custom parameters · Issue #1590 @probonopd @Halvhjearne | github/tzapu/WiFiManager](https://github.com/tzapu/WiFiManager/issues/1590)**
Implementación de parámetros personalizados mediante librería WiFi Manager
