#include <time.h>

String timestamp;

// Intervalo de consulta Fecha y hora
unsigned long NtpPrevMillis = 0;
unsigned long NtpInterval = 60000; // 1 minuto

//Obtener Fecha y Hora desde NTP Server
String TimeData() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return "";  // Devuelve una cadena vacía si no se obtiene la hora
    }

    char time[20];
    strftime(time, sizeof(time), "%d/%m/%y   %H:%M", &timeinfo); // Formato. Agregar %A para obeter el día de la semana
    return String(time);  // Convierte el arreglo char en un objeto String
}