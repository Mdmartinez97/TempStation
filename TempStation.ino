#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiManager.h>
#include <EEPROM.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>

#include "Secrets.h"
#include "TimeData.h"
#include "Gemini.h"
#include "WeatherRequest.h"

int Temp, feelTemp;

//*************** LCD & Touch Scroll *********************
const int I2Caddress = 0x27;

LiquidCrystal_I2C lcd(I2Caddress, 16, 2);

#define touchPin 4
#define touchLED 2

// Duración de pantalla encendida
unsigned long LCDPrevMillis = 0;
unsigned long LCDInterval = 15000; // 15 segundos

int pantalla_anterior;

#include "Scroll.h"

String Clothes1, Clothes2;

//***************** Sensor DS18B20 ***********************
#define SensorPin 5

// Declaración de objetos
OneWire oneWire(SensorPin);
DallasTemperature sensors(&oneWire);

int localTemp;

//******************* WiFi Manager ***********************
#define RstWF 15 // Botón Reset WiFi

WiFiManager wifiManager;

// Location data
float lat, lon;
int utc;

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

  // Buscar datos guardados
  EEPROM.get(address_lat, lat);
  EEPROM.get(address_lon, lon);
  EEPROM.get(address_utc, utc);

  pinMode(RstWF, INPUT_PULLUP);
  pinMode(SensorPin, INPUT_PULLUP);
  pinMode(touchLED, OUTPUT);
  pinMode(touchPin, INPUT);

  // Configurar fecha y hora
  configTime(utc*60*60, 0, "pool.ntp.org"); // UTC expresado en segundos
  TimeData();
}

void loop() {//######################################################## LOOP #######################################################

  //-----Consultar NTP Server
    if (millis() - NtpPrevMillis > NtpInterval || NtpPrevMillis == 0){
  
    timestamp = TimeData();
    Serial.print("Timestamp: ");      
    Serial.println(timestamp);

    NtpPrevMillis = millis();
  }

  //-----Consultar Sensor y API's 
  if (millis() - WTPrevMillis > WTInterval || WTPrevMillis == 0){

    // Medir Temperatura interior
    sensors.requestTemperatures();
    localTemp = sensors.getTempCByIndex(0); // Consultar sensor #0
      Serial.print("Local Temp: ");      
      Serial.println(localTemp);

    // Consultar Temperatura exterior
    getWeatherData(Temp, feelTemp, lat, lon);
      Serial.print("Temp: ");      
      Serial.println(Temp);
      Serial.print("Feel Temp: ");      
      Serial.println(feelTemp);

    // Generar comentario Gemini AI
    String Clothes = Gemini(feelTemp);
      Serial.print("Comentario Gemini: ");      
      Serial.println(Clothes);

      // Dividir frase en 2 filas para LCD
      Clothes1 = Clothes.substring(0, 15);
      Clothes2 = Clothes.substring(15);

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
        
        lcd.setCursor(0, 0);
        lcd.print(Clothes1);
        lcd.setCursor(0, 1);
        lcd.print(Clothes2);

        LCDPrevMillis = millis();
        break;

        default:
          lcd.backlight();
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