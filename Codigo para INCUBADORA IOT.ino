// CORPORACIÓN UNIVERSITARIA COMFACAUCA - UNICOMFACAUCA
// PROYECTO FINAL - FISICA II
// INCUBADORA IOT DE BAJO COSTO PARA HUEVOS DE GALLINA

// INTEGRANTES:
// JOHAN SEBASTIAN PEÑA POSADA.
// VICTOR ALFONSO GOMEZ URRESTE.
// ANGELA MARIA CORTES VASQUEZ.
// ANGELA MARIA TOLA TABORD.



//-----DECLARACION DE LIBRERIAS------------------------------
#include "DHT.h"            // Sensores DHT
#include "ESP32Time.h"      // Hora y fecha
#include <LiquidCrystal.h>  // Display lcd
#if defined(ESP32)          // Microcontrolador Esp32
#include <WiFi.h>           // Wifi
#include <FirebaseESP32.h>  // Base de datos (FIREBASE)
#elif defined(ESP8266)      //Por si se usa el microcontrolador  ESP8266
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#endif
#include <addons/TokenHelper.h>  // información del proceso de generación del token.
#include <addons/RTDBHelper.h>   //información de impresión de la carga útil de RTDB y otras funciones auxiliares

//-----DEFINICION DE PINES PARA EL DISPLAY LCD----
int rs = 18;
int e = 19;
int d4 = 5;
int d5 = 4;
int d6 = 21;
int d7 = 2;

//-----DEFINICION DE TIPO Y PINES PARA EL SENSOR--------
#define DHTPIN 13      // Pin digital conectado al sensor DHT
#define DHTTYPE DHT22  // tipo DHT 22

//-----DEFINICION DE PINES PARA LOS RELAYS--------
#define relayv 15  // Rele del cooler
#define relayf 22  // Rele del Foco de calor

//credenciales

//Definir las credenciales WiFi
#define WIFI_SSID "PP"
#define WIFI_PASSWORD "12345678"

//Definir la API Key
#define API_KEY "AIzaSyAcxY_-XuzOsRYotEdDsovG_nzydhzkqnc"

//Definir el RTDB URL
#define DATABASE_URL "proyecto-fisica-ii-default-rtdb.firebaseio.com/"  //<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app

// Defina el correo electrónico y la contraseña del usuario que ya se registró o agregó en su proyecto
#define USER_EMAIL "johanpena@unicomfacauca.edu.co"
#define USER_PASSWORD "11111111"

//Definir objeto de datos de Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
unsigned long sendDataPrevMillis = 0;
unsigned long count = 0;

LiquidCrystal lcd(rs, e, d4, d5, d6, d7);
DHT dht(DHTPIN, DHTTYPE);

//Datos de la red
const char* ssid = "PP";
const char* password = "12345678";

//Configuracion del servidor NTP
const char* ntpServer = "pool.ntp.org";  //Servidor NTP
const long gmtOffset_sec = -5 * 3600;    //Desplazamiento GMT
const int daylightOffset_sec = 0;        //Compensacion de luz diurna
ESP32Time rtc;

void setup() {

  Serial.begin(9600);       // Puerto serial a 9600bps
  dht.begin();              //iniciamos sensor
  lcd.begin(16, 2);         //iniciamos el Display lcd
  pinMode(relayv, OUTPUT);  // Relay Cooler como salida
  pinMode(relayf, OUTPUT);  // Relay Foco como salida

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);  // iniciar el wifi
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");  // si no se logra conectar apareceran estos puntos
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  //Asigne la clave api (obligatorio)
  config.api_key = API_KEY;

  //Asigne las credenciales de inicio de sesión del usuario
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  //Asigne la URL de RTDB (obligatorio)
  config.database_url = DATABASE_URL;

  //Asigne la función de devolución de llamada para la tarea de generación de tokens de ejecución prolongada/
  config.token_status_callback = tokenStatusCallback;  //see addons/TokenHelper.h

  Firebase.begin(&config, &auth);

  //Comente o pase un valor falso cuando la reconexión WiFi controlará su código o la biblioteca de terceros
  Firebase.reconnectWiFi(true);

  Firebase.setDoubleDigits(5);

  // Configuracion de la hora
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void loop() {
  delay(1000);
  float h = dht.readHumidity();     //Leemos la Humedad
  float t = dht.readTemperature();  //Leemos la temperatura en grados Celsius

  //--------Enviamos las lecturas al display LCD----------
  lcd.setCursor(0, 0);
  lcd.print("Humedad: ");
  lcd.print(h);
  lcd.print("%");
  lcd.setCursor(0, 1);
  lcd.print("temp: ");
  lcd.print(t);
  lcd.print("C");
  delay(1000);
  //--------CONDICIONES PARA CONEXION CON LA BASE DE DATOS---------
  if (Firebase.ready() && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    Serial.printf("Set Temperature... %s\n", Firebase.setFloat(fbdo, F("/test/temperature"), t) ? "ok" : fbdo.errorReason().c_str());

    Serial.printf("Get Temperature... %s\n", Firebase.getFloat(fbdo, F("/test/temperature")) ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());

    Serial.printf("Set Humidity... %s\n", Firebase.setDouble(fbdo, F("/test/humidity"), h) ? "ok" : fbdo.errorReason().c_str());

    Serial.printf("Get Humidity... %s\n", Firebase.getDouble(fbdo, F("/test/humidity")) ? String(fbdo.to<double>()).c_str() : fbdo.errorReason().c_str());

    //For the usage of FirebaseJson, see examples/FirebaseJson/BasicUsage/Create_Parse_Edit.ino
    FirebaseJson json;
    Serial.println(rtc.getTime("%A, %B %d %Y %H:%M:%S"));  //Funcion para imprimir la fehca y hora

    if (count == 0) {
      json.set("value/round/" + String(count), F("cool!"));
      json.set(F("vaue/ts/.sv"), F("timestamp"));
      Serial.printf("Set json... %s\n", Firebase.set(fbdo, F("/test/json"), json) ? "ok" : fbdo.errorReason().c_str());
    } else {
      json.add(String(count), "smart!");
      Serial.printf("Update node... %s\n", Firebase.updateNode(fbdo, F("/test/json/value/round"), json) ? "ok" : fbdo.errorReason().c_str());
    }

    Serial.println();
    delay(3000);

    //--------CONTROL DE LA INCUBADORA----------
    if (t <= # && >= #) {         // condicion para la temperatura - si es mayor_igual a # y menor_igual a #grados
      digitalWrite(relayv, LOW);  // Desactivado el ventilador por medio del rele1
      Serial.println("Cooler DESACTIVADO");
      delay(10000);
      digitalWrite(relayf, HIGH);  // Desactivado el ventilador por medio del rele2
      Serial.println("FOCO ACTIVADO");
      delay(10000);
    }
    if (t > #) {                  // condicion para la temperatura - si es mayor_igual a #
      digitalWrite(relayv, HIGH);  // Desactivado el ventilador por medio del rele1
      Serial.println("Cooler ACTIVADO");
      delay(10000);
      digitalWrite(relayf, LOW);  // Desactivado el ventilador por medio del rele1
      Serial.println("FOCO DESACTIVADO");
      delay(10000);
      
      // AGREGAR CONDICIONES DE HUMEDAD PARA EL CONTROL TOTAL
    }
    count++;
  }
}