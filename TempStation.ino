//******************* Hora y Clima ***********************
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

// Latitud, Longitud y UTC
float lat;
float lon;
int utc;

const char* apiKey = "79ec60437018d54a22094466dce934bc";

// Intervalo de consulta
unsigned long WTPrevMillis = 0;
unsigned long WTInterval = 60000; // 1 minuto

String timestamp;
int localTemp,Temp, feelTemp;

//***************** LCD Display **************************
#include <LiquidCrystal_I2C.h>
// Dirección I2C
const int I2Caddress = 0x27;

// Crear un objeto LiquidCrystal_I2C
LiquidCrystal_I2C lcd(I2Caddress, 16, 2);

// Duración de pantalla encendida
unsigned long LCDPrevMillis = 0;
unsigned long LCDInterval = 10000; // 10 segundos

int pantalla_anterior;

//***************** Sensor DS18B20 ***********************
#include <OneWire.h>
#include <DallasTemperature.h>
#define SensorPin 4
// Declaración de objetos
OneWire oneWire(SensorPin);
DallasTemperature sensors(&oneWire);

//******************* Pin Touch **************************
const int ledPin = 2;
const int touchPin = 4;

//******************* WiFi Manager ***********************
#include <WiFi.h>
#include <WiFiManager.h>
#include <EEPROM.h>
#define RstWF 15 //Botón Reset WiFi

WiFiManager wifiManager;

const char* WM_ssid = "WiFi Manager TempStation";
const char* WM_pass = "12345678";


// Direcciones de memoria
const int address_lat = 0;
const int address_lon = 4;
const int address_utc = 8;

void setup() {//######################################################## SETUP #####################################################
  Serial.begin(115200);
  lcd.init();

  pinMode(RstWF, INPUT_PULLUP);
  EEPROM.begin(512); // Definir tamaño de la EEPROM

  CustomWiFiManager(); // Llamado a función conectar Wi-Fi

  pinMode(RstWF, INPUT_PULLUP); // Botón Reset WiFi
  pinMode(ledPin, OUTPUT);
  pinMode(touchPin, INPUT);
}

void loop() {//######################################################## LOOP #######################################################
  
  // Buscar datos guardados
  EEPROM.get(address_lat, lat);
  EEPROM.get(address_lon, lon);
  EEPROM.get(address_utc, utc);

  // Consultar Clima y Hora
  if (millis() - WTPrevMillis > WTInterval || WTPrevMillis == 0){
  
  // Configurar y Consultar fecha y hora
  configTime(utc*60*60, 0, "pool.ntp.org"); // UTC * 60" * 60'
  timestamp = TimeData();


  // Medir Temperatura interior
  localTemp = getLocalTemp();

  // Consultar Temperatura exterior
  getWeatherData(Temp, feelTemp);

  WTPrevMillis = millis();
  }

  int pantalla = Scroll(); // Leer botón Touch

  if(pantalla != pantalla_anterior){ // Solo actualizar si se presiona el btn

    pantalla_anterior = pantalla;

      switch (pantalla) { // Pantallas de información 0, 1 y 2
        case 0:
        Serial.println("Entramos a la pantalla 0");
        lcd.clear();
        lcd.backlight();

        lcd.setCursor(2, 0);
        lcd.print("Inside ");
        lcd.print(localTemp);
        lcd.print(" C");

        lcd.setCursor(0, 1);
        lcd.print("Out ");
        lcd.print(Temp);
        //lcd.print(" C");

        lcd.setCursor(8, 1);
        lcd.print(" Feel ");
        lcd.print(feelTemp);
        //lcd.print(" C");

        LCDPrevMillis = millis();
        break;

      case 1:
        Serial.println("Entramos a la pantalla 1");
        lcd.clear();
        lcd.backlight();

        lcd.setCursor(0, 1);
        lcd.print(timestamp);

        LCDPrevMillis = millis();
        break;

      case 2:
        Serial.println("Entramos a la pantalla 2");
        lcd.clear();
        lcd.backlight();
        lcd.setCursor(2, 0);
        lcd.print("Pantalla 2");
        LCDPrevMillis = millis();
        break;

        default:
          // Aquí irían las instrucciones por default
          break;
        }

  }  else{
      // Apagar pantalla al cumplirse el tiempo
      if (millis() - LCDPrevMillis > LCDInterval || LCDPrevMillis == 0){
      lcd.clear();
      lcd.noBacklight();
      LCDPrevMillis = millis();
      }
    }

  //BOTÓN DE RESET WIFI
 if(digitalRead(RstWF) == LOW){ // Restablecer configuración, borrar las credenciales almacenadas y reiniciar ESP
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
}

//######################################################## FUNCIONES ###############################################################
// Función WiFi Manager Custom Parameters
bool shouldSaveConfig = false; // Bandera para guardar datos

void saveConfigCallback() {
  // Bandera true si se actualizan los datos de WM
  shouldSaveConfig = true;
}

void CustomWiFiManager(){
  // Mensaje de inicio en LCD
  lcd.clear();
  lcd.backlight();
  lcd.setCursor(2, 0);
  lcd.print("Wi-Fi Manager");
  lcd.setCursor(3, 1);
  lcd.print("iniciado... ");

  WiFi.mode(WIFI_STA);
  wifiManager.setDebugOutput(true); //Habilita debug serie, deshabilitar a gusto
  wifiManager.setSaveConfigCallback(saveConfigCallback); // Llama a la función si se actualizaron los datos
  wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0)); //Definir IP de acceso a Panel WiFi Manager
  //⬇⬇ COMENTAR PARA USAR DHCP ⬇⬇
  //wifiManager.setSTAStaticIPConfig(IPAddress(192,168,100,99), IPAddress(192,168,100,1), IPAddress(255,255,255,0)); //Configuración de red MANUAL (opcional DNS 4to argumento)

  wifiManager.setCustomHeadElement("<style>button{background-color: #006994;}</style>"); //CSS botones
  wifiManager.setClass("invert"); //Tema oscuro
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

  bool res;    
  res = wifiManager.autoConnect(WM_ssid, WM_pass);
   
  if(!res) {
    Serial.println("Error al conectar!");
    Serial.println("Por favor, reinicie manualmente");
    delay(30000);    // 30 segundos de gracia
    ESP.restart(); //Automatic restart
  }

  // Mostrar datos de Red en LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(WiFi.SSID());
  lcd.setCursor(1, 1);
  lcd.print(WiFi.localIP());
  delay(2000);
  //lcd.noBacklight();

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

  // Imprimir datos obtenidos
  Serial.print("Latitud ingresada: ");
  Serial.println(lat);
  Serial.print("Longuitud ingresada: ");
  Serial.println(lon);
  Serial.print("Uso horario ingresado: ");
  Serial.println(utc);

  // Tries to reconnect automatically when the connection is lost
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
}

// Función de consulta a Sensor local
int getLocalTemp() {
 /* sensors.requestTemperatures();
  int localTemp = sensors.getTempCByIndex(0);*/
  int localTemp = temperatureRead(); // REEMPLAZAR
  return localTemp;
}

// Función de consulta a Get Weather Map
void getWeatherData(int &Temp, int &feelTemp) {
  HTTPClient http; // Iniciación

  // URL web
  String url = "https://api.openweathermap.org/data/2.5/weather?lat=" + String(lat) + "&lon=" + String(lon) + "&appid=" + apiKey + "&units=metric&lang=es";
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

// Función de cambio de pantalla con botón táctil
int Scroll(){
  int N_opciones = 3;
  static int opcion;
  int touchValue = touchRead(touchPin);
  //Serial.println(touchValue);

  const int touchThreshold = 80; // Valor umbral de sensibilidad touchPin
  if (touchValue < touchThreshold) {
    digitalWrite(ledPin, HIGH);  // Enciende el LED
      if(opcion < N_opciones - 1){
        opcion++;
      } else {
        opcion=0;
        }
    
  } else {
    digitalWrite(ledPin, LOW);   // Apaga el LED
    }
  delay(200);

  return opcion;
}