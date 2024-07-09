#include "sas.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include <WebServer.h>
#include <ElegantOTA.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>
#include <HTTPUpdate.h>  // Incluir la biblioteca HTTPUpdate para ESP32
#include "Arduino.h"

// Pines
const int LED_SAS = 12;
const int LED_WIFI = 13;
const int SAS_TX = 32;
const int SAS_RX = 35;
const int PULSADOR = 0;

// Constantes
#define K_TIMEOUT_SAS_RX_INTERBYTE 100

const char *VERSION = "1.0";               // Versión actual del firmware
const char *FIRMWARE = "sasMqtt.ino.bin";  // Nombre del archivo de firmware en el servidor
bool updateInProgress = false;
String latestVersion;  // Variable para almacenar la versión más reciente del firmware

// Definición de Timers
hw_timer_t *timer1 = NULL;
portMUX_TYPE timer1Mux = portMUX_INITIALIZER_UNLOCKED;
hw_timer_t *timer3 = NULL;
portMUX_TYPE timer3Mux = portMUX_INITIALIZER_UNLOCKED;
hw_timer_t *timer4 = NULL;
portMUX_TYPE timer4Mux = portMUX_INITIALIZER_UNLOCKED;

TaskHandle_t taskHandle = NULL;

String wifiSsid = "GALAXY";
String wifiPassword = "Dylan*152426*D";
String mqttServerAddress = "10.0.0.19";
String wifiMacAddress = "";
int mqttPort = 1883;
String mqttUser = "";
String mqttPassword = "";

// Otros parámetros y variables
unsigned short security1 = 1234;
unsigned short security2 = 5678;
String codigoBanca = "ABC";
String codigoMaquina = "NNNN";
byte sasAddress = 0x01;
unsigned short sasPollTime = 500;

bool TIMER1_TICK = false;
bool TIMER3_TICK = false;
bool TIMER4_TICK = false;
bool SAS_RX_FALLING = false;
bool AUX_LED = false;
bool SAS_POLL_ENABLED = false;
bool SAS_NEW_COUNTERS_DATA = false;
unsigned short sasAuxPollTime;
int sasPollStep;
bool SAS_COMMAND_PENDING = false;
bool SAS_PIN_ENABLED = false;
bool SAS_RECEIVING_FROM_SLOT_MACHINE = false;
bool SAS_NEW_RX_DATA = false;
bool SAS_NEW_RX_VALID_DATA = false;

unsigned short sasTxBuffer[256];
byte sasCommandBuffer[256];
int sasCommandNumBytes = 0;
byte sasTxNumbytes;
byte sasTxStatus;
byte sasTxNumBits;
unsigned short sasTxData;
int sasTxIndex;
unsigned short sasTxAuxBits;
unsigned long long sasCounters[45];
unsigned long long sasCountersBackup[45];
byte txData[16];

unsigned short sasRxBuffer[256];
byte sasRxNumbytes;
byte sasRxStatus;
byte sasRxNumBits;
byte sasRxAuxTime;
unsigned short sasRxData;
int sasRxIndex;
unsigned short sasRxAuxBits;
unsigned short timeLastSasByte;
byte SAS_COMANDO_ENTRANTE = 255;

unsigned short sasAnswerTimeOut = 0;
byte sasLastAnswerTime = 0;

byte sasAFTRegistrationKey[20] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x20 };
byte sasAFTAssetNumber[4] = { 0x01, 0x00, 0x00, 0x00 };
byte sasAFTPOSID[4] = { 0x01, 0x00, 0x00, 0x00 };
byte sasAFTCashableQuantity[5];
byte sasAFTRestrictedQuantity[10];
byte sasAFTNonRestrictedQuantity[5];
byte sasAFTTransactionID[18];

byte mqttAnswerBuffer[256];

// Definición de las variables de byte
byte sasAFT_Key[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x20 };
byte sasAFT_Asset[] = { 0x01, 0x00, 0x00, 0x00 };
byte sasAFT_POS[] = { 0x01, 0x00, 0x00, 0x00 };

WiFiClient espClient;
PubSubClient mqttClient(espClient);
Preferences preferences;
WebServer server(80);

// Declaración de Funciones
void ledOnOffSAS(bool state);
void ledOnOffWifi(bool state);
void handlerTimer4RxSAS();
void handlerTimer3TxSAS();
void handlerSAS_RxPin();
void handlerTimer1_1ms();
void printHex(byte *buffer, int length);
bool CheckMQTTWifiConnection();
void MqttSubscribeCallback(char *topic, byte *payload, unsigned int length);
void IRAM_ATTR handlerTimer1_1ms();
void IRAM_ATTR handlerSAS_RxPin();
void IRAM_ATTR handlerTimer4RxSAS();
void IRAM_ATTR handlerTimer3TxSAS();
void PublicaContador(int i);
void PublicaEvento();
void PublicaRespuesta();
void ProcesaISRs(void);
void ProcesaRespuestaSAS(void);
void pushSASEvent(byte event);
unsigned short SasCRC(byte *buffer, int length, unsigned short crc);
void procesaContadorSas(int contador, int valor, int index);
void ErrorProgramacion(void);
void Programacion(void);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setupServer() {
  // Configurar ElegantOTA con autenticación
  ElegantOTA.begin(&server, "admin", "admin");
  server.begin();
  Serial.println("Servidor HTTP iniciado");
  firmwareUpdate();
}

void firmwareUpdate() {
  HTTPClient http;
  WiFiClientSecure client;
  client.setInsecure();  // Usar solo para pruebas, deshabilitar en producción

  // Construye la URL del archivo que contiene la versión del firmware en el servidor
  String versionUrl = "https://raw.githubusercontent.com/juanclavel/esp32_updatetest/main/VERSION.txt";  // URL del archivo de versión
  if (!http.begin(client, versionUrl)) {
    Serial.println("Fallo al iniciar el cliente HTTP para obtener la versión.");
    return;
  }

  // Realiza la solicitud HTTP GET para obtener la versión del firmware
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    latestVersion = http.getString();
    latestVersion.trim();  // Elimina los espacios en blanco alrededor de la versión
    Serial.print("Versión más reciente del firmware en el servidor: ");
    Serial.println(latestVersion);
  } else {
    Serial.printf("Error al obtener la versión del firmware (código=%d)\n", httpCode);
    http.end();
    return;
  }
  http.end();

  // Compara la versión más reciente con la versión actual
  if (compareVersions(VERSION, latestVersion) < 0) {
    // Si la versión en el servidor es más reciente que la actual, procede con la actualización
    String firmwareUrl = String("https://github.com/juanclavel/esp32_updatetest/raw/main/") + FIRMWARE;  // URL del archivo de firmware
    if (!http.begin(client, firmwareUrl)) {
      Serial.println("Fallo al iniciar el cliente HTTP para la actualización.");
      return;
    }

    httpCode = http.sendRequest("HEAD");
    if (httpCode < 300 || httpCode > 400) {
      Serial.printf("Error al verificar el firmware (código=%d)\n", httpCode);
      http.end();
      return;
    }

    updateInProgress = true;


    httpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    t_httpUpdate_return ret = httpUpdate.update(client, firmwareUrl);

    switch (ret) {
      case HTTP_UPDATE_FAILED:
        Serial.printf("Actualización HTTP fallida (Error=%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
        break;

      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("¡No hay actualizaciones!");
        break;

      case HTTP_UPDATE_OK:
        Serial.println("¡Actualización OK!");
        break;
    }

    updateInProgress = false;
  } else {
    Serial.println("El firmware ya está actualizado .");
  }
}

// Función para comparar versiones en formato "X.Y.Z"
int compareVersions(const String &version1, const String &version2) {
  int major1, minor1, patch1;
  int major2, minor2, patch2;
  sscanf(version1.c_str(), "%d.%d.%d", &major1, &minor1, &patch1);
  sscanf(version2.c_str(), "%d.%d.%d", &major2, &minor2, &patch2);

  if (major1 != major2) {
    return major1 - major2;
  } else if (minor1 != minor2) {
    return minor1 - minor2;
  } else {
    return patch1 - patch2;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setupTimers() {
  // Timer 1 prescaler de 80, tiempo de timer en 1/80MHz => 1us.
  timer1 = timerBegin(0, 80, true);
  timerAttachInterrupt(timer1, &handlerTimer1_1ms, true);
  timerAlarmWrite(timer1, 1000, true);
  timerAlarmEnable(timer1);

  // Timer 3 prescaler de 80, tiempo de timer en 1/80MHz => 1us.
  timer3 = timerBegin(1, 80, true);
  timerAttachInterrupt(timer3, &handlerTimer3TxSAS, true);
  timerAlarmWrite(timer3, 52, true);

  // Configuración del Timer 4
  timer4 = timerBegin(2, 80, true);                         // Timer 4, prescaler de 80 (80 MHz / 80 = 1 MHz)
  timerAttachInterrupt(timer4, &handlerTimer4RxSAS, true);  // Asociar la función de interrupción
  timerAlarmWrite(timer4, 26, true);                        // Configurar la alarma para 26 microsegundos
  timerAlarmEnable(timer4);                                 // Habilitar la alarma del Timer 4

  // // Iniciar el Timer 4
  // timerStart(timer4);
}

void setupPins() {
  pinMode(LED_SAS, OUTPUT);
  pinMode(LED_WIFI, OUTPUT);
  pinMode(SAS_TX, OUTPUT);
  pinMode(SAS_RX, INPUT);

  // Configuración de la interrupción del pin SAS_RX por flanco de bajada
  attachInterrupt(SAS_RX, handlerSAS_RxPin, FALLING);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Inicializando sistema....");
  // Serial.println("mqttServerAddress...");
  // Serial.println(mqttServerAddress);

  setupPins();
  setupTimers();

  ledOnOffSAS(false);
  ledOnOffWifi(false);

  // Inicialización de variables
  sasRxIndex = 0;
  sasRxNumbytes = 0;
  SAS_RECEIVING_FROM_SLOT_MACHINE = false;

  SAS_RX_FALLING = false;
  TIMER4_TICK = false;

  // Inicialización de tareas en núcleos específicos
  xTaskCreatePinnedToCore(
    ProcesaLoop,    // Función que ejecutará la tarea en el primer núcleo
    "ProcesaLoop",  // Nombre de la tarea
    10000,          // Tamaño de la pila
    NULL,           // Parámetros de entrada a la tarea (en este caso, no se usan)
    1,              // Prioridad de la tarea (1 es la más baja en FreeRTOS)
    NULL,           // Manejador de la tarea (no se usa aquí)
    0               // Núcleo en el que se ejecutará la tarea (0 para el primer núcleo)
  );
}

// Función ProcesaLoop que será ejecutada en el segundo núcleo
void ProcesaLoop(void *pvParameters) {
  int i, j;
  bool AUX;

  // Obtiene la dirección MAC
  wifiMacAddress = WiFi.macAddress();

  // Secuencia de luces intermitentes, si se presiona la tecla, va a programación
  AUX = false;
  for (i = 0; i < 20; i++) {
    ledOnOffWifi(true);
    ledOnOffSAS(false);
    delay(150);
    if (digitalRead(PULSADOR) == false) AUX = true;
    ledOnOffWifi(false);
    ledOnOffSAS(true);
    delay(150);
  }
  ledOnOffWifi(false);
  ledOnOffSAS(false);
  if (AUX == true) Programacion();

  // Lectura de las credenciales almacenadas en Flash
  preferences.begin("Configuracion", false);
  wifiSsid = preferences.getString("wifiSsid", "");
  wifiPassword = preferences.getString("wifiPassword", "");
  mqttServerAddress = preferences.getString("mqttServer", "");
  mqttUser = preferences.getString("mqttUser", "");
  mqttPassword = preferences.getString("mqttPassword", "");
  mqttPort = preferences.getInt("mqttPort", 0);
  sasAddress = (byte)preferences.getUShort("sasAddress", 0);
  sasPollTime = preferences.getUShort("sasPollTime", 0);
  security1 = preferences.getUShort("security1", 0);
  security2 = preferences.getUShort("security2", 0);
  preferences.end();

  // Verificación de la seguridad 1
  unsigned short crc;
  byte checkBuffer[32];
  byte auxBytes[32];

  for (i = 0; i < 32; i++) checkBuffer[i] = 0x5a;
  wifiMacAddress.getBytes(checkBuffer, wifiMacAddress.length() + 1);
  checkBuffer[17] = 'S';
  checkBuffer[18] = 'o';
  checkBuffer[19] = 'f';
  checkBuffer[20] = 'i';
  checkBuffer[21] = 'a';
  wifiSsid.getBytes(auxBytes, wifiSsid.length() + 1);

  j = wifiSsid.length() - 1;
  if (j > 10) j = 10;
  for (i = 0; i <= j; i++) checkBuffer[i + 22] = auxBytes[i];

  crc = SasCRC(&checkBuffer[0], 32, 0x0000);
  if (crc != security1) ErrorProgramacion();

  // Verificación de la seguridad 2
  for (i = 0; i < 32; i++) checkBuffer[i] = 0x5a;
  wifiMacAddress.getBytes(checkBuffer, wifiMacAddress.length() + 1);
  checkBuffer[17] = 'J';
  checkBuffer[18] = 'u';
  checkBuffer[19] = 'a';
  checkBuffer[20] = 'n';
  checkBuffer[21] = 'K';
  wifiPassword.getBytes(auxBytes, wifiPassword.length() + 1);

  j = wifiPassword.length() - 1;
  if (j > 10) j = 10;
  for (i = 0; i <= j; i++) checkBuffer[i + 22] = auxBytes[i];

  crc = SasCRC(&checkBuffer[0], 32, 0x0000);
  if (crc != security2) ErrorProgramacion();

  // Inicialización de contadores
  for (i = 0; i < 45; i++) {
    sasCounters[i] = 0;
    sasCountersBackup[i] = 0;
  }

  ledOnOffSAS(false);
  ledOnOffWifi(false);

  SAS_POLL_ENABLED = true;
  sasAuxPollTime = 5;
  sasPollStep = 0;

  // Bucle de ejecución principal
  while (true) {
    if (sasPollStep == 0) {
      while (CheckMQTTWifiConnection() == false) {
        ledOnOffWifi(false);
      }
    }
    ledOnOffWifi(true);
    mqttClient.loop();
    ProcesaISRs();

    if (SAS_NEW_COUNTERS_DATA == true) {
      SAS_NEW_COUNTERS_DATA = false;
      for (i = 0; i < 45; i++) {
        if (sasCounters[i] != sasCountersBackup[i]) {
          PublicaContador(i);
          sasCountersBackup[i] = sasCounters[i];
          delay(25);
        }
      }
    }

    vTaskDelay(1);  // Liberar el núcleo para otras tareas
  }
}


void loop() {
  // int i, j;
  // bool AUX;

  // // Obtiene la dirección MAC
  // wifiMacAddress = WiFi.macAddress();

  // // Secuencia de luces intermitentes, si se presiona la tecla, va a programación
  // AUX = false;
  // for (i = 0; i < 20; i++) {
  //   ledOnOffWifi(true);
  //   ledOnOffSAS(false);
  //   delay(150);
  //   if (digitalRead(PULSADOR) == false) AUX = true;
  //   ledOnOffWifi(false);
  //   ledOnOffSAS(true);
  //   delay(150);
  // }
  // ledOnOffWifi(false);
  // ledOnOffSAS(false);
  // if (AUX == true) Programacion();

  // // Lectura de las credenciales almacenadas en Flash
  // preferences.begin("Configuracion", false);
  // wifiSsid = preferences.getString("wifiSsid", "");
  // wifiPassword = preferences.getString("wifiPassword", "");
  // mqttServerAddress = preferences.getString("mqttServer", "");
  // mqttUser = preferences.getString("mqttUser", "");
  // mqttPassword = preferences.getString("mqttPassword", "");
  // mqttPort = preferences.getInt("mqttPort", 0);
  // sasAddress = (byte)preferences.getUShort("sasAddress", 0);
  // sasPollTime = preferences.getUShort("sasPollTime", 0);
  // security1 = preferences.getUShort("security1", 0);
  // security2 = preferences.getUShort("security2", 0);
  // preferences.end();

  // // Verificación de la seguridad 1
  // unsigned short crc;
  // byte checkBuffer[32];
  // byte auxBytes[32];

  // for (i = 0; i < 32; i++) checkBuffer[i] = 0x5a;
  // wifiMacAddress.getBytes(checkBuffer, wifiMacAddress.length() + 1);
  // checkBuffer[17] = 'S';
  // checkBuffer[18] = 'o';
  // checkBuffer[19] = 'f';
  // checkBuffer[20] = 'i';
  // checkBuffer[21] = 'a';
  // wifiSsid.getBytes(auxBytes, wifiSsid.length() + 1);

  // j = wifiSsid.length() - 1;
  // if (j > 10) j = 10;
  // for (i = 0; i <= j; i++) checkBuffer[i + 22] = auxBytes[i];

  // crc = SasCRC(&checkBuffer[0], 32, 0x0000);
  // if (crc != security1) ErrorProgramacion();

  // // Verificación de la seguridad 2
  // for (i = 0; i < 32; i++) checkBuffer[i] = 0x5a;
  // wifiMacAddress.getBytes(checkBuffer, wifiMacAddress.length() + 1);
  // checkBuffer[17] = 'J';
  // checkBuffer[18] = 'u';
  // checkBuffer[19] = 'a';
  // checkBuffer[20] = 'n';
  // checkBuffer[21] = 'K';
  // wifiPassword.getBytes(auxBytes, wifiPassword.length() + 1);

  // j = wifiPassword.length() - 1;
  // if (j > 10) j = 10;
  // for (i = 0; i <= j; i++) checkBuffer[i + 22] = auxBytes[i];

  // crc = SasCRC(&checkBuffer[0], 32, 0x0000);
  // if (crc != security2) ErrorProgramacion();

  // // Inicialización de contadores
  // for (i = 0; i < 45; i++) {
  //   sasCounters[i] = 0;
  //   sasCountersBackup[i] = 0;
  // }

  // ledOnOffSAS(false);
  // ledOnOffWifi(false);

  // SAS_POLL_ENABLED = true;
  // sasAuxPollTime = 5;
  // sasPollStep = 0;

  // while (true) {
  //   if (sasPollStep == 0) {
  //     while (CheckMQTTWifiConnection() == false) {
  //       ledOnOffWifi(false);
  //     }
  //   }
  //   ledOnOffWifi(true);
  //   mqttClient.loop();
  //   ProcesaISRs();

  //   if (SAS_NEW_COUNTERS_DATA == true) {
  //     SAS_NEW_COUNTERS_DATA = false;
  //     for (i = 0; i < 45; i++) {
  //       if (sasCounters[i] != sasCountersBackup[i]) {
  //         PublicaContador(i);
  //         sasCountersBackup[i] = sasCounters[i];
  //         delay(25);
  //       }
  //     }
  //   }
  // }
}

void ErrorProgramacion() {
  delay(1000);
  ledOnOffSAS(false);
  ledOnOffWifi(false);
  delay(1000);
  Serial.println();

  while (true) {
    Serial.println("Error de programación....");
    ledOnOffSAS(true);
    ledOnOffWifi(false);
    delay(500);
    ledOnOffSAS(true);
    ledOnOffWifi(true);
    delay(500);
    ledOnOffSAS(false);
    ledOnOffWifi(true);
    delay(500);
    ledOnOffSAS(false);
    ledOnOffWifi(false);
    delay(500);
  }
}

void Programacion() {
  String command;
  String inputStringValue;
  Serial.println("INICIO RUTINA DE PROGRAMACION");

  // noInterrupts();

  delay(2000);

  ledOnOffSAS(true);
  ledOnOffWifi(true);

  while (true) {
    while (Serial.available()) {
      command = Serial.readStringUntil(' ');
      if (command == "com1") {
        Serial.print("COM1:");
        Serial.println(wifiMacAddress);
      } else if (command.substring(0, 4) == "com2") {
        wifiSsid = command.substring(4);
        Serial.print("COM2:");
        Serial.println(wifiSsid);
      } else if (command.substring(0, 4) == "com3") {
        wifiPassword = command.substring(4);
        Serial.print("COM3:");
        Serial.println(wifiPassword);
      } else if (command.substring(0, 4) == "com4") {
        mqttServerAddress = command.substring(4);
        Serial.print("COM4:");
        Serial.println(mqttServerAddress);
      } else if (command.substring(0, 4) == "com5")  //Puerto Mqtt
      {
        inputStringValue = command.substring(4);
        mqttPort = (ushort)(inputStringValue.toInt());
        Serial.print("COM5:");
        Serial.println(mqttPort);
      } else if (command.substring(0, 4) == "com6") {
        mqttUser = command.substring(4);
        Serial.print("COM6:");
        Serial.println(mqttUser);

      } else if (command.substring(0, 4) == "com7") {
        mqttPassword = command.substring(4);
        Serial.print("COM7:");
        Serial.println(mqttPassword);
      } else if (command.substring(0, 4) == "com8")  // Sas Address
      {
        inputStringValue = command.substring(4);
        sasAddress = (byte)(inputStringValue.toInt());
        Serial.print("COM8:");
        Serial.println(sasAddress);
      } else if (command.substring(0, 4) == "com9")  // Poll Time
      {
        inputStringValue = command.substring(4);
        sasPollTime = (ushort)(inputStringValue.toInt());
        Serial.print("COM9:");
        Serial.println(sasPollTime);
      } else if (command.substring(0, 4) == "coma")  // Security1
      {
        inputStringValue = command.substring(4);
        security1 = (ushort)(inputStringValue.toInt());
        Serial.print("COMA:");
        Serial.println(security1);
      } else if (command.substring(0, 4) == "comb")  // Security 2
      {
        inputStringValue = command.substring(4);
        security2 = (ushort)(inputStringValue.toInt());
        Serial.print("COMB:");
        Serial.println(security2);

        preferences.begin("Configuracion", false);

        preferences.putString("wifiSsid", wifiSsid);
        preferences.putString("wifiPassword", wifiPassword);
        preferences.putString("mqttServer", mqttServerAddress);
        preferences.putString("mqttUser", mqttUser);
        preferences.putString("mqttPassword", mqttPassword);
        preferences.putInt("mqttPort", mqttPort);
        preferences.putUShort("sasAddress", sasAddress);
        preferences.putUShort("sasPollTime", sasPollTime);
        preferences.putUShort("security1", security1);
        preferences.putUShort("security2", security2);

        preferences.end();

        delay(1000);
        ledOnOffSAS(false);
        ledOnOffWifi(false);
        delay(5000);
        ESP.restart();
      }
    }
  }
}

void ProcesaISRs() {

  if (TIMER1_TICK == true)  // Base de tiempo de 1ms
  {

    TIMER1_TICK = false;

    // Secuencia de Poll a la Máquina
    if (SAS_POLL_ENABLED == true) {
      if (sasAuxPollTime > 0) sasAuxPollTime--;
      else {
        sasAuxPollTime = sasPollTime;
        SAS_Poll();
      }
    }

    // Verificación del tiempo interbyte de recepción
    if (SAS_NEW_RX_DATA == true)  // Si existe un nuevo dato recibido desde la máquina...
    {
      //sasAnswerTimeOut=0;
      //sasLastAnswerTime=0;
      if (SAS_RECEIVING_FROM_SLOT_MACHINE == true) {
        timeLastSasByte = K_TIMEOUT_SAS_RX_INTERBYTE;  // Si está recibiendo bytes deja el contador de último byte en su valor máximo en ms
      } else {
        if (timeLastSasByte > 0) timeLastSasByte--;  // Decrementa el contador hasta llegar a 0.
        else {
          SAS_NEW_RX_DATA = false;
          ProcesaRespuestaSAS();
          sasRxNumbytes = 0;
          sasRxIndex = 0;
        }
      }
    }

    if (SAS_NEW_RX_VALID_DATA == false)
    //else    // Se cuenta el tiempo en el que no se ha recibido algún datos de la máquina
    {
      sasAnswerTimeOut++;
      if (sasAnswerTimeOut == 3000) {
        sasAnswerTimeOut = 0;
        if (sasLastAnswerTime < 255) sasLastAnswerTime++;
        String topic = "SCM/" + wifiMacAddress + "/conexion";
        mqttAnswerBuffer[0] = sasLastAnswerTime;
        mqttClient.publish(topic.c_str(), mqttAnswerBuffer, 1);
        Serial.print(topic);
        printHex(&mqttAnswerBuffer[0], 8);
        Serial.println();
      }
    }
  }
}

void ProcesaRespuestaSAS() {
  unsigned short us, crc;
  int i, j, k;
  bool AUX;
  byte auxBuffer[256];
  bool ASSET_ID_OK = false;

  SAS_NEW_RX_VALID_DATA = false;

  // Chequea que el mensaje tengo de bit de paridad en 0
  AUX = false;
  for (i = 0; i < sasRxNumbytes; i++) {
    us = sasRxBuffer[i];
    us &= 0b100000000;
    if (us != 0) AUX = true;  // Si la paridad de algún bit recibido es 1, ignora el mensaje.
    auxBuffer[i] = (byte)sasRxBuffer[i];
  }
  if (AUX == true) return;


  // Se determina si es un evento
  if (sasRxNumbytes == 1) {
    if (auxBuffer[0] >= 0x11) {
      SAS_NEW_RX_VALID_DATA = true;
      sasAnswerTimeOut = 0;
      sasLastAnswerTime = 0;
      PublicaEvento(auxBuffer[0], 0, &auxBuffer[0]);
    } else if (auxBuffer[0] == 0x00) {
      SAS_NEW_RX_VALID_DATA = true;
      sasAnswerTimeOut = 0;
      sasLastAnswerTime = 0;
    }
  } else {
    if (auxBuffer[0] == sasAddress)  // Valida que es una repuesta de la máquina
    {
      SAS_NEW_RX_VALID_DATA = true;
      sasLastAnswerTime = 0;
      sasAnswerTimeOut = 0;

      // Verificación del CRC
      crc = SasCRC(&auxBuffer[0], (sasRxNumbytes - 2), 0x0000);
      us = auxBuffer[sasRxNumbytes - 1];
      us *= 256;
      us += auxBuffer[sasRxNumbytes - 2];
      if (us != crc) return;

      AUX = false;

      switch (auxBuffer[1]) {
        case 0x1a:  // Créditos actuales
          if (sasRxNumbytes == 8) {
            sasCounters[SAS_CREDITS] = SasCapturaValorContador(&auxBuffer[2], 4);
            SAS_NEW_COUNTERS_DATA = true;
            //PublicaContador(SAS_CREDITS);
          }
          break;

        case 0x0f:  // Contadores varios
          if (sasRxNumbytes == 28) {
            sasCounters[SAS_TOTAL_CANCELLED] = SasCapturaValorContador(&auxBuffer[2], 4);
            sasCounters[SAS_TOTAL_COIN_IN] = SasCapturaValorContador(&auxBuffer[6], 4);
            sasCounters[SAS_TOTAL_COIN_OUT] = SasCapturaValorContador(&auxBuffer[10], 4);
            sasCounters[SAS_TOTAL_DROP] = SasCapturaValorContador(&auxBuffer[14], 4);
            sasCounters[SAS_TOTAL_JACKPOT] = SasCapturaValorContador(&auxBuffer[18], 4);
            sasCounters[SAS_GAMES_PLAYED] = SasCapturaValorContador(&auxBuffer[22], 4);
            SAS_NEW_COUNTERS_DATA = true;
          }
          break;
        case 0x1c:  // Contadores varios
          if (sasRxNumbytes == 36) {
            sasCounters[SAS_TOTAL_COIN_IN] = SasCapturaValorContador(&auxBuffer[2], 4);
            sasCounters[SAS_TOTAL_COIN_OUT] = SasCapturaValorContador(&auxBuffer[6], 4);
            sasCounters[SAS_TOTAL_DROP] = SasCapturaValorContador(&auxBuffer[10], 4);
            sasCounters[SAS_TOTAL_JACKPOT] = SasCapturaValorContador(&auxBuffer[14], 4);
            sasCounters[SAS_GAMES_PLAYED] = SasCapturaValorContador(&auxBuffer[18], 4);
            sasCounters[SAS_GAMES_WON] = SasCapturaValorContador(&auxBuffer[22], 4);
            sasCounters[SAS_DOOR_OPEN] = SasCapturaValorContador(&auxBuffer[26], 4);
            sasCounters[SAS_POWER_UP] = SasCapturaValorContador(&auxBuffer[30], 4);
            SAS_NEW_COUNTERS_DATA = true;
          }
          break;
        case 0x5b:                    // Recibe código del PIN
          mqttAnswerBuffer[0] = '6';  // Respuesta a comando "6"
          mqttAnswerBuffer[1] = auxBuffer[2];
          for (i = 0; i < auxBuffer[2]; i++) mqttAnswerBuffer[i + 2] = auxBuffer[i + 3];
          PublicaRespuesta(auxBuffer[2] + 2);
          break;

        case 0x73:  // Recibe información acerca del estado del registro AFT

          if (auxBuffer[2] == 0x1d)  // Respuesta con 29 bytes
          {
            j = 0;
            k = 4;
            for (i = 0; i < 4; i++) {
              if (auxBuffer[k] == sasAFTAssetNumber[i]) j++;
              k++;
            }
            if (j == 4) ASSET_ID_OK = true;

            for (i = 0; i < 20; i++) {
              if (auxBuffer[k] == sasAFTRegistrationKey[i]) j++;
              k++;
            }
            for (i = 0; i < 4; i++) {
              if (auxBuffer[k] == sasAFTPOSID[i]) j++;
              k++;
            }
            if (ASSET_ID_OK == true)  // Los campos del registro coinciden
            {
              if (auxBuffer[3] == 0)  // Máquina lista para registrar
              {
                sasCommandBuffer[0] = 0x73;  // Se realiza la solicitud de registro automaticamente
                sasCommandBuffer[1] = 0x1d;
                sasCommandBuffer[2] = 0x01;
                j = 3;
                for (i = 0; i < 4; i++) {
                  sasCommandBuffer[j] = sasAFTAssetNumber[i];
                  j++;
                }
                for (i = 0; i < 20; i++) {
                  sasCommandBuffer[j] = sasAFTRegistrationKey[i];
                  j++;
                }
                for (i = 0; i < 4; i++) {
                  sasCommandBuffer[j] = sasAFTPOSID[i];
                  j++;
                }
                SAS_COMMAND_PENDING = true;
                sasCommandNumBytes = j;
                Serial.println("Efectúa registro AFT");
              }
            }
            // Envía respuesta al servidor
            mqttAnswerBuffer[0] = 'R';  // Respuesta a comando "S"
            mqttAnswerBuffer[1] = sasRxNumbytes;
            for (i = 0; i < sasRxNumbytes; i++) mqttAnswerBuffer[i + 2] = auxBuffer[i];
            PublicaRespuesta(sasRxNumbytes + 2);
          }
          break;

        default:                      // Envía la respuesta de la máquina, posiblemente sea por comando literal enviado
          mqttAnswerBuffer[0] = 'S';  // Respuesta a comando "S"
          mqttAnswerBuffer[1] = sasRxNumbytes;
          for (i = 0; i < sasRxNumbytes; i++) mqttAnswerBuffer[i + 2] = auxBuffer[i];
          PublicaRespuesta(sasRxNumbytes + 2);
          AUX = true;
          break;
      }
      if (AUX == false) {
        if (SAS_COMANDO_ENTRANTE == auxBuffer[1])  // Si el comando entrante es igual a unop de los comando de Poll se transmite la respuesta de la máquina
        {
          mqttAnswerBuffer[0] = 'S';  // Respuesta a comando "S"
          mqttAnswerBuffer[1] = sasRxNumbytes;
          for (i = 0; i < sasRxNumbytes; i++) mqttAnswerBuffer[i + 2] = auxBuffer[i];
          PublicaRespuesta(sasRxNumbytes + 2);
          SAS_COMANDO_ENTRANTE = 255;
        }
      }
    }
  }

  //    printHex(&auxBuffer[0],sasRxNumbytes);
}


void SAS_Poll() {
  byte outBuffer[256];
  int i;
  unsigned short crc;

  switch (sasPollStep) {
    case 0:  // Este tiempo es usado para chequear que la conexión MQTT está correcta.
      sasPollStep = 10;
      break;
    case 10:  // Sincronismo
      outBuffer[0] = 0x80;
      transmiteSAS(&outBuffer[0], 1);
      sasPollStep = 20;
      break;
    case 20:  // Pide eventos
      outBuffer[0] = 0x80;
      outBuffer[0] += sasAddress;
      transmiteSAS(&outBuffer[0], 1);
      if (SAS_COMMAND_PENDING == true) sasPollStep = 200;
      else sasPollStep = 30;
      break;
    case 30:  // Pide créditos actuales. Comando 0x1a
      outBuffer[0] = sasAddress;
      outBuffer[1] = 0x1a;
      transmiteSAS(&outBuffer[0], 2);
      if (SAS_COMMAND_PENDING == true) sasPollStep = 200;
      else sasPollStep = 40;
      break;
    case 40:  // Contadores varios. Comando 0x0f
      outBuffer[0] = sasAddress;
      outBuffer[1] = 0x0f;
      transmiteSAS(&outBuffer[0], 2);
      if (SAS_COMMAND_PENDING == true) sasPollStep = 200;
      else sasPollStep = 50;
      break;
    case 50:  // Contadores varios. Comando 0x1c
      outBuffer[0] = sasAddress;
      outBuffer[1] = 0x1c;
      transmiteSAS(&outBuffer[0], 2);
      if (SAS_COMMAND_PENDING == true) sasPollStep = 200;
      else sasPollStep = 60;
      break;
    case 60:  // Habilita PIN. Comando 0x5a
      outBuffer[0] = sasAddress;
      outBuffer[1] = 0x5a;
      if (SAS_PIN_ENABLED == true) {
        outBuffer[2] = 0x01;
        outBuffer[3] = 0xd2;
        outBuffer[4] = 0x65;
      } else {
        outBuffer[2] = 0x00;
        outBuffer[3] = 0x5b;
        outBuffer[4] = 0x74;
      }
      transmiteSAS(&outBuffer[0], 5);
      if (SAS_COMMAND_PENDING == true) sasPollStep = 200;
      else sasPollStep = 0;
      break;

    case 200:
      SAS_COMMAND_PENDING = false;
      outBuffer[0] = sasAddress;
      for (i = 1; i <= sasCommandNumBytes; i++) outBuffer[i] = sasCommandBuffer[i - 1];
      if ((sasCommandNumBytes > 1) || (outBuffer[1] == 0x5b) || (outBuffer[1] == 0x01) || (outBuffer[1] == 0x02) || (outBuffer[1] == 0x06) || (outBuffer[1] == 0x07))  // %b es un comando particular de la Potogold
      {
        crc = SasCRC(&outBuffer[0], i, 0x0000);
        outBuffer[i] = crc & 0x00ff;
        i++;
        outBuffer[i] = crc >> 8;
        i++;
      }
      transmiteSAS(&outBuffer[0], i);
      sasPollStep = 0;
      break;
    default:
      sasPollStep = 0;
      break;
  }
}

void PublicaContador(int NumeroContador) {
  int i;
  String topic;

  unsigned long long n = sasCounters[NumeroContador];
  if (NumeroContador >= 45) return;

  //    mqttAnswerBuffer[0]=(byte)NumeroContador;
  for (i = 0; i < 9; i++) {
    mqttAnswerBuffer[i] = (byte)n;
    n = n >> 8;
  }
  String s = String(NumeroContador);
  topic = "SCM/" + wifiMacAddress + "/contadores/" + s;
  mqttClient.publish(topic.c_str(), mqttAnswerBuffer, 8);
  Serial.print(topic);
  printHex(&mqttAnswerBuffer[0], 8);
  Serial.println();
}

void PublicaEvento(byte CodigoEvento, unsigned int NumBytes, byte Data[]) {
  byte MAC[18];
  byte salida[19 + NumBytes];
  int i;
  String topic;

  mqttAnswerBuffer[0] = CodigoEvento;
  mqttAnswerBuffer[1] = (byte)NumBytes;
  for (i = 0; i < NumBytes; i++) mqttAnswerBuffer[i + 2] = Data[i];

  topic = "SCM/" + wifiMacAddress + "/eventos";
  mqttClient.publish(topic.c_str(), mqttAnswerBuffer, NumBytes + 2);
}

void PublicaRespuesta(unsigned int NumBytes) {
  String topic;
  // El buffer mqttAnswerBuffer debe venir listo desde quien llame esta función
  // a partir de la dirección 17
  topic = "SCM/" + wifiMacAddress + "/respuestas";
  mqttClient.publish(topic.c_str(), mqttAnswerBuffer, NumBytes);
}


void MqttSubscribeCallback(char *topic, byte *payload, unsigned int len) {
  int i, j;
  Serial.print("Comando entrante: ");
  switch (payload[0]) {
    case 0x30:  // Habilita Máquina
                //            for (i=0;i<len;i++) sasCommandBuffer[i]=payload[i+1]
      SAS_COMMAND_PENDING = true;
      sasCommandBuffer[0] = 0x02;
      sasCommandNumBytes = 1;
      Serial.println("Habilita máquina");
      break;
    case 0x31:  // Deshabilita Máquina
      SAS_COMMAND_PENDING = true;
      sasCommandBuffer[0] = 0x01;
      sasCommandNumBytes = 1;
      Serial.println("Deshabilita máquina");
      break;
    case 0x32:  // Habilita Billetero
      SAS_COMMAND_PENDING = true;
      sasCommandBuffer[0] = 0x06;
      sasCommandNumBytes = 1;
      Serial.println("Habilita billetero");
      break;
    case 0x33:  // Deshabilita Billetero
      SAS_COMMAND_PENDING = true;
      sasCommandBuffer[0] = 0x07;
      sasCommandNumBytes = 1;
      Serial.println("Deshabilita billetero");
      break;
    case 0x34:  // Habilita PIN
      Serial.println("Habilita PIN");
      SAS_PIN_ENABLED = true;
      break;
    case 0x35:  // Deshabilita PIN
      Serial.println("Deshabilita PIN");
      SAS_PIN_ENABLED = false;
      break;
    case 0x36:  // Pide información del PIN
      SAS_COMMAND_PENDING = true;
      sasCommandBuffer[0] = 0x5b;
      sasCommandNumBytes = 1;
      Serial.println("Pide información del PIN");
      break;
    case 'P':  // Comando Solicitud Pago Tiquete
      SAS_COMMAND_PENDING = true;
      sasCommandBuffer[0] = 0x4d;
      sasCommandBuffer[1] = 0x00;  // Cantidad de bytes
      sasCommandNumBytes = 2;
      SAS_COMMAND_PENDING = true;
      Serial.println("Comando que pide los datos del tiquete");
      break;

    case 'S':  // Comando SAS enviado literalmente desde el servidor
      SAS_COMMAND_PENDING = true;
      SAS_COMANDO_ENTRANTE = payload[1];
      for (i = 1; i < len; i++) sasCommandBuffer[i - 1] = payload[i];
      sasCommandNumBytes = len - 1;
      Serial.println("Comando SAS literal.");
      break;
    case 'A':  // Configuracion AFT
      j = 1;
      for (i = 0; i < 4; i++) {
        sasAFTAssetNumber[i] = payload[j];
        j++;
      }
      for (i = 0; i < 20; i++) {
        sasAFTRegistrationKey[i] = payload[j];
        j++;
      }
      for (i = 0; i < 4; i++) {
        sasAFTPOSID[i] = payload[j];
        j++;
      }
      Serial.println("Configuracion AFT");
      printHex(&payload[0], len);
      break;

    case 'I':  // Consulta estado transferencia
      sasCommandBuffer[0] = 0x72;
      sasCommandBuffer[1] = 0x02;        // Cantidad de bytes
      sasCommandBuffer[2] = 0xFF;        // Interroga la transacción
      sasCommandBuffer[3] = payload[1];  // Debe enviarse cuál indice de transacción
      sasCommandNumBytes = 4;
      SAS_COMMAND_PENDING = true;
      Serial.println("Consulta estado de la transferencia AFT");
      break;

    case 'C':  // Cancela transferencia
      sasCommandBuffer[0] = 0x72;
      sasCommandBuffer[1] = 0x02;        // Cantidad de bytes
      sasCommandBuffer[2] = 0x80;        // Interroga la transacción
      sasCommandBuffer[3] = payload[1];  // Debe enviarse cuál indice de transacción
      sasCommandNumBytes = 4;
      SAS_COMMAND_PENDING = true;
      Serial.println("Cancela la transferencia AFT");
      break;


    case 'T':  // Efectúa transferencia AFT
      sasCommandBuffer[0] = 0x72;
      sasCommandBuffer[1] = 0x45;        // Cantidad de bytes
      sasCommandBuffer[2] = 0x00;        // Transfer code
      sasCommandBuffer[3] = payload[1];  // Transaction Index
      sasCommandBuffer[4] = payload[2];  // Tipo de transferencia, 0x00 acredita, 0x80 retira

      // Valor de la transferencia
      TransformaByteArrayToBCDByteArray(&payload[3], &sasCommandBuffer[5], 5);
      TransformaByteArrayToBCDByteArray(&payload[8], &sasCommandBuffer[10], 5);
      TransformaByteArrayToBCDByteArray(&payload[13], &sasCommandBuffer[15], 5);

      sasCommandBuffer[20] = 0x00;  // Transfer flags
      j = 21;
      for (i = 0; i < 4; i++)  // Asset Number
      {
        sasCommandBuffer[j] = sasAFTAssetNumber[i];
        j++;
      }
      for (i = 0; i < 20; i++)  // Registration Key
      {
        sasCommandBuffer[j] = sasAFTRegistrationKey[i];
        j++;
      }
      sasCommandBuffer[j] = 0x12;
      j++;                      // Cantidad de bytes del Transfer ID
      for (i = 0; i < 18; i++)  // Transfer ID
      {
        sasCommandBuffer[j] = payload[i + 18];
        j++;
      }
      for (i = 0; i < 4; i++)  // Fecha de validez
      {
        sasCommandBuffer[j] = payload[i + 36];
        j++;
      }
      for (i = 0; i < 2; i++)  // Pool ID
      {
        sasCommandBuffer[j] = payload[i + 40];
        j++;
      }
      sasCommandBuffer[j] = 0x00;
      j++;  // Cantidad de bytes del Receipt Data
      sasCommandNumBytes = j;
      SAS_COMMAND_PENDING = true;
      Serial.println("Transferencia AFT");
      break;

    case 'R':  // Realiza el registro AFT
               //            sasCommandBuffer[]=0x;
      sasCommandBuffer[0] = 0x73;
      sasCommandBuffer[1] = 0x1d;
      sasCommandBuffer[2] = 0x00;
      j = 3;
      for (i = 0; i < 4; i++) {
        sasCommandBuffer[j] = sasAFTAssetNumber[i];
        j++;
      }
      for (i = 0; i < 20; i++) {
        sasCommandBuffer[j] = sasAFTRegistrationKey[i];
        j++;
      }
      for (i = 0; i < 4; i++) {
        sasCommandBuffer[j] = sasAFTPOSID[i];
        j++;
      }
      SAS_COMMAND_PENDING = true;
      sasCommandNumBytes = j;
      Serial.println("Inicia registro AFT");
      break;
    case 0x37:  // Envía código de validación de la máquina
      SAS_COMMAND_PENDING = true;
      sasCommandBuffer[0] = 0x4c;
      sasCommandBuffer[1] = 0x00;
      sasCommandBuffer[2] = 0x00;
      sasCommandBuffer[3] = 0x01;
      sasCommandBuffer[4] = 0x00;
      sasCommandBuffer[5] = 0x00;
      sasCommandBuffer[6] = 0x00;
      sasCommandNumBytes = 7;
      Serial.println("Envía codigo de validacion");
      break;

    case 0x38:  // Envía código de validación de la máquina
      SAS_COMMAND_PENDING = true;
      sasCommandBuffer[0] = 0x59;
      sasCommandBuffer[1] = 0x01;
      sasCommandBuffer[2] = 0x03;
      sasCommandBuffer[3] = 0x12;
      sasCommandBuffer[4] = 0x00;
      sasCommandBuffer[5] = 0x77;
      sasCommandNumBytes = 6;
      Serial.println("Envía codigo de validacion Poto");
      break;


    case 0x200:  // Envío del comando literal desde el servidor
      Serial.println("Comando literal..");
      break;

    default:
      Serial.println("Desconocido");
      break;
  }
}

bool CheckMQTTWifiConnection() {
  while (true) {
    if (WiFi.status() == WL_CONNECTED) {
      if (mqttClient.connected() == true) return true;
      else {

        Serial.println("Conectando al broker MQTT...");
        mqttClient.setServer(mqttServerAddress.c_str(), mqttPort);
        while (!mqttClient.connected()) {
          if (mqttClient.connect(wifiMacAddress.c_str(), mqttUser.c_str(), mqttPassword.c_str())) {
            Serial.println("Conectado al servidor MQTT");
            mqttClient.setCallback(MqttSubscribeCallback);
            String topic;
            topic = "SCM/" + wifiMacAddress + "/comandos";

            mqttClient.subscribe(topic.c_str());
            SAS_POLL_ENABLED = true;
            //                        SAS_POLL_ENABLED=false;
            sasAuxPollTime = 5;
            sasPollStep = 10;

          } else {
            Serial.println("Error al conectar al servidor MQTT: " + mqttClient.state());
            SAS_POLL_ENABLED = false;
            delay(2000);
          }
        }
      }
    } else {
      SAS_POLL_ENABLED = false;

      Serial.println("Conectando a la red " + wifiSsid + "....");

      WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str());



      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      Serial.println("Conexión exitosa!");
      Serial.println(WiFi.localIP());
      setupServer();

      wifiMacAddress = WiFi.macAddress();
      Serial.println("Usando MAC Address " + wifiMacAddress);
      // setupServer();
    }
  }
}



void printHex(byte *buffer, int bufferSize) {
  int i;
  for (int i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

void transmiteSAS(byte *buffer, int bufferSize) {
  int i, j;

  for (i = 0; i < bufferSize; i++) {
    sasTxBuffer[i] = *buffer;
    buffer++;
  }
  sasTxBuffer[0] += 0b100000000;
  sasTxNumbytes = bufferSize;
  sasTxIndex = 0;
  sasTxStatus = 5;
  timerWrite(timer3, 0);
  timerAlarmEnable(timer3);
}



void ledOnOffSAS(bool ESTADO) {
  if (ESTADO == true) digitalWrite(LED_SAS, false);
  else digitalWrite(LED_SAS, true);
}

void ledOnOffWifi(bool ESTADO) {
  if (ESTADO == true) digitalWrite(LED_WIFI, false);
  else digitalWrite(LED_WIFI, true);
}

void IRAM_ATTR handlerTimer1_1ms() {
  TIMER1_TICK = true;
}

void IRAM_ATTR handlerSAS_RxPin() {
  detachInterrupt(SAS_RX);
  SAS_RX_FALLING = true;

  SAS_RX_FALLING = false;
  sasRxNumBits = 9;
  sasRxData = 0;
  sasRxAuxTime = 3;

  if (AUX_LED == false) {
    AUX_LED = true;
    ledOnOffSAS(true);
    //digitalWrite(SAS_TX,true);
  } else {
    AUX_LED = false;
    ledOnOffSAS(false);
    //digitalWrite(SAS_TX,false);
  }

  SAS_RECEIVING_FROM_SLOT_MACHINE = true;
  timerWrite(timer4, 0);
  timerAlarmEnable(timer4);
  // Iniciar el Timer 4
  timerStart(timer4);
}

void IRAM_ATTR handlerTimer4RxSAS() {
  TIMER4_TICK = true;


  //    if (TIMER4_TICK==true)      // Interrupción que controla la recepción de la comunicación SAS. Se procesa cada 26us equivalente a la mitad del tiempo de bit a 19200
  //    {
  TIMER4_TICK = false;


  sasRxAuxTime--;
  if (sasRxAuxTime == 0) {
    sasRxAuxTime = 2;

    if (AUX_LED == false) {
      AUX_LED = true;
      ledOnOffSAS(true);
      //digitalWrite(SAS_TX,true);
    } else {
      AUX_LED = false;
      ledOnOffSAS(false);
      //digitalWrite(SAS_TX,false);
    }


    if (digitalRead(SAS_RX)) sasRxAuxBits = 0b1000000000;
    else sasRxAuxBits = 0;
    sasRxData |= sasRxAuxBits;
    sasRxData = sasRxData >> 1;
    sasRxNumBits--;
    if (sasRxNumBits == 0) {
      sasRxBuffer[sasRxIndex] = sasRxData;
      sasRxIndex++;
      sasRxNumbytes++;
      // timerAlarmDisable(timer4);
      // Parar el Timer 4
      timerStop(timer4);

      SAS_RX_FALLING = false;
      attachInterrupt(SAS_RX, handlerSAS_RxPin, FALLING);
      SAS_RECEIVING_FROM_SLOT_MACHINE = false;
      SAS_NEW_RX_DATA = true;
    }
  }
  //    }
}

void IRAM_ATTR handlerTimer3TxSAS(void) {
  TIMER3_TICK = true;

  TIMER3_TICK = false;

  switch (sasTxStatus) {
    case 0:  // Idle
      break;
    case 5:  // Transmitiendo StartBit
      digitalWrite(SAS_TX, false);
      sasTxAuxBits = 0b000000000001;
      sasTxStatus = 10;
      sasTxNumBits = 10;
      sasTxData = sasTxBuffer[sasTxIndex];
      sasTxData += 0b1000000000;
      break;
    case 10:  // Transmitiendo uno a uno los bits
      if ((sasTxAuxBits & sasTxData) == 0) digitalWrite(SAS_TX, false);
      else digitalWrite(SAS_TX, true);
      sasTxAuxBits = sasTxAuxBits << 1;
      sasTxNumBits--;
      if (sasTxNumBits == 0) {
        sasTxStatus = 20;
      }
      break;
    case 20:  // Verifica si se transmitieron todos los bytes
      sasTxNumbytes--;
      if (sasTxNumbytes == 0) {
        sasTxStatus = 0;
        timerAlarmDisable(timer3);
      } else {
        sasTxIndex++;
        sasTxStatus = 5;
      }
      break;
  }
}


unsigned short SasCRC(byte *s, int len, unsigned short sas_CRCval) {
  unsigned short c, q;
  int pos = len;
  for (; len; len--) {
    c = *s++;
    q = (sas_CRCval ^ c) & 0x0f;
    sas_CRCval = (sas_CRCval >> 4) ^ (q * 010201);
    q = (sas_CRCval ^ (c >> 4)) & 017;
    sas_CRCval = (sas_CRCval >> 4) ^ (q * 010201);
  }
  return sas_CRCval;
}

void procesaContadorSas(int IndiceContador, int Start, int Len) {
  //    sasCounters[IndiceContador]=SasCapturaValorContador(&rxdata[Start],Len);
  if (sasCounters[IndiceContador] != sasCountersBackup[IndiceContador]) {
    //        SAS_COUNTERS_CHANGE=true;
  }
}


unsigned long long SasCapturaValorContador(byte *s, int Len) {
  unsigned long long valor = 0;
  unsigned long long multiplicador = 1;
  //    double valor=0;
  //    double multiplicador=1;
  int i;
  byte a, b;

  s += (Len - 1);
  for (i = 0; i < Len; i++) {
    a = *s;
    b = a & 0x0f;
    a = a >> 4;
    valor += b * multiplicador;
    multiplicador *= 10;
    valor += a * multiplicador;
    multiplicador *= 10;
    s--;
  }
  return valor;
}

void TransformaByteArrayToBCDByteArray(byte *in, byte *out, int outLen) {
  long long value;
  int i;
  byte ret[8];

  // Convierte el valor entrado a entero
  value |= (long long)(*in);
  in++;
  value |= (long long)(*in << 8);
  in++;
  value |= (long long)(*in << 16);
  in++;
  value |= (long long)(*in << 24);
  in++;
  value |= (long long)(*in << 32);

  Serial.println(value, DEC);
  // Formatea el buffer de salida
  for (i = 0; i < 8; i++) ret[i] = 0x00;
  for (i = 0; i < 8; i++) {
    ret[i] = (byte)(value % 10);
    value /= 10;
    ret[i] |= (byte)((value % 10) << 4);
    value /= 10;
  }

  for (i = (outLen - 1); i >= 0; i--) {
    *out = ret[i];
    out++;
  }
}
