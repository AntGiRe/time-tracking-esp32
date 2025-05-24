// Sistema de control de acceso con ESP32, lector de huellas, RFID y Supabase
// Proyecto final de asignatura. Incluye retroalimentación visual mediante LEDs y OLED

#include <WiFi.h>
#include <ESPSupabase.h>
#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ------------------------
// Configuración de pines
// ------------------------
HardwareSerial mySerial(2);  // RX = GPIO16, TX = GPIO17
// Uso de HardwareSerial con índice 2 (Serial2) permite comunicación UART adicional en ESP32
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

#define SS_PIN 5
#define RST_PIN 22
MFRC522 mfrc522(SS_PIN, RST_PIN);

#define LED_VERDE 14
#define LED_ROJO 27
#define BUZZER 26
#define OLED_SDA 4
#define OLED_SCL 15

// Configuración pantalla OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ------------------------
// Conexión Wi-Fi y Supabase
// ------------------------
const char* ssid = "vodafone7801";
const char* password = "5BDJYZMCC74C85";
const char* supabaseUrl = "https://kduoudmniqjtartllozn.supabase.co";
const char* supabaseKey = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImtkdW91ZG1uaXFqdGFydGxsb3puIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NDM5MzUwNzcsImV4cCI6MjA1OTUxMTA3N30.T1mpTXZaGF8AIzvQ96jeQh95Vo4cfLCiWh0QSH0Ea84"; // Oculto por seguridad

Supabase supabase;
// Configuración estática de usuario y ubicación
const int STATIC_EMPLOYEE_ID = 3;
const int STATIC_LOCATION_ID = 1;

int currentId = 1;

// ------------------------
// Setup principal
// ------------------------
void setup() {
  Serial.begin(115200);
  while (!Serial);

  setupOLED();
  setupIO();
  setupWiFi();
  setupSupabase();
  setupSensors();
}

// ------------------------
// Loop principal
// ------------------------
void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'r') {
      Serial.println("🔐 Registro de nueva huella...");
      if (enrollFingerprint(currentId) == FINGERPRINT_OK) {
        Serial.println("✅ Huella registrada con ID: " + String(currentId));
        currentId++;
      } else {
        Serial.println("❌ Fallo al registrar huella.");
      }
    }
  }

  uint8_t resultado = checkFingerprint();
  if (resultado == FINGERPRINT_OK) {
    accesoConcedidoFingerprint(finger.fingerID);
  } else if (resultado == FINGERPRINT_NOTFOUND) {
    accesoDenegado("Huella no válida");
  }

  checkRFID();
  delay(500);
}

// ------------------------
// Inicializaciones separadas
// ------------------------
void setupOLED() {
  Wire.begin(OLED_SDA, OLED_SCL); // Inicia I2C en pines personalizados (solo ESP32)
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Error al inicializar pantalla OLED");
    while (true);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Sistema de Acceso");
  display.println("Inicializando...");
  display.display();
}

void setupIO() {
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_ROJO, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_ROJO, LOW);
  digitalWrite(BUZZER, LOW);
}

void setupWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Conectando a Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi conectado");
}

void setupSupabase() {
  supabase.begin(supabaseUrl, supabaseKey);
  Serial.println("Supabase inicializado");
}

void setupSensors() {
  mySerial.begin(57600, SERIAL_8N1, 16, 17); // Comunicación UART por pines RX=16, TX=17
  finger.begin(57600);
  Serial.println("Inicializando sensor de huellas...");
  if (finger.verifyPassword()) {
    Serial.println("Sensor de huellas OK.");
  } else {
    Serial.println("Error al iniciar sensor de huellas.");
    while (1);
  }
  finger.getTemplateCount();
  currentId = finger.templateCount + 1;

  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("Sensor RFID listo.");
}

// ------------------------
// Funciones de huella digital
// ------------------------
uint8_t enrollFingerprint(int id) {
  int p = -1;
  Serial.println("Coloca un dedo...");
  while ((p = finger.getImage()) != FINGERPRINT_OK);
  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) return p;
  Serial.println("Primera imagen tomada. Retira el dedo...");
  delay(2000);
  while ((p = finger.getImage()) != FINGERPRINT_NOFINGER);
  Serial.println("Vuelve a colocar el mismo dedo...");
  while ((p = finger.getImage()) != FINGERPRINT_OK);
  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) return p;
  Serial.println("Comparando huellas...");
  p = finger.createModel();
  if (p != FINGERPRINT_OK) return p;
  Serial.println("Guardando huella con ID: " + String(id));
  return finger.storeModel(id);
}

uint8_t checkFingerprint() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) return p;
  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return p;
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Huella válida. ID: " + String(finger.fingerID));
    return FINGERPRINT_OK;
  }
  return FINGERPRINT_NOTFOUND;
}

// ------------------------
// Funciones RFID y validación
// ------------------------
int getEmployeeIdFromRFID(String uidHex) {
  String result = supabase
                    .from("users")
                    .select("*")
                    .eq("rfid_card", uidHex)
                    .limit(1)
                    .doSelect();

  if (result.indexOf("id") != -1) {
    int idIndex = result.indexOf("\"id\":") + 5;
    int commaIndex = result.indexOf(",", idIndex);
    String idStr = result.substring(idIndex, commaIndex);
    return idStr.toInt();
  }
  return -1;
}

void checkRFID() {
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) return;

  byte *uid = mfrc522.uid.uidByte;
  String uidStr = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (uid[i] < 0x10) uidStr += "0";
    uidStr += String(uid[i], HEX);
  }
  uidStr.toUpperCase();

  Serial.println("UID leído: " + uidStr);

  int employeeId = getEmployeeIdFromRFID(uidStr);
  if (employeeId > 0) {
    accesoConcedidoRFID(employeeId, uidStr);
  } else {
    accesoDenegado("RFID no autorizado");
  }
  mfrc522.PICC_HaltA();
}

// ------------------------
// Acciones de control y registro
// ------------------------
void accesoConcedidoRFID(int employeeId, String uidStr) {
  Serial.println("Acceso concedido por RFID");
  digitalWrite(LED_VERDE, HIGH);
  digitalWrite(BUZZER, HIGH);
  mostrarOLED("RFID OK - Acceso");
  delay(200);
  digitalWrite(BUZZER, LOW);
  delay(1000);
  digitalWrite(LED_VERDE, LOW);

  String json = "{";
  json += "\"employee_id\": " + String(employeeId) + ",";
  json += "\"location_id\": " + String(STATIC_LOCATION_ID) + ",";
  json += "\"access_method\": \"rfid\",";
  json += "\"rfid_card_code\": \"" + uidStr + "\",";
  json += "\"fingerprint_id\": null";
  json += "}";

  int response = supabase.insert("access_logs", json, false);
  if (response == 201) Serial.println("Registro insertado en Supabase");
  else Serial.println("Error al insertar en Supabase. Código: " + String(response));
}

void accesoConcedidoFingerprint(int fingerprintId) {
  Serial.println("Acceso concedido por huella");
  digitalWrite(LED_VERDE, HIGH);
  digitalWrite(BUZZER, HIGH);
  mostrarOLED("Huella OK - Acceso");
  delay(200);
  digitalWrite(BUZZER, LOW);
  delay(1000);
  digitalWrite(LED_VERDE, LOW);

  String json = "{";
  json += "\"employee_id\": " + String(STATIC_EMPLOYEE_ID) + ",";
  json += "\"location_id\": " + String(STATIC_LOCATION_ID) + ",";
  json += "\"access_method\": \"fingerprint\",";
  json += "\"fingerprint_id\": " + String(fingerprintId) + ",";
  json += "\"rfid_card_code\": \"EMPTY\"";
  json += "}";

  int response = supabase.insert("access_logs", json, false);
  if (response == 201) Serial.println("Registro insertado en Supabase");
  else Serial.println("Error al insertar en Supabase. Código: " + String(response));
}

void accesoDenegado(String fuente) {
  Serial.println("Acceso denegado por " + fuente);
  mostrarOLED("Acceso Denegado\n" + fuente);
  for (int i = 0; i < 2; i++) {
    digitalWrite(LED_ROJO, HIGH);
    digitalWrite(BUZZER, HIGH);
    delay(150);
    digitalWrite(LED_ROJO, LOW);
    digitalWrite(BUZZER, LOW);
    delay(150);
  }
}

// ------------------------
// Pantalla OLED
// ------------------------
void mostrarOLED(String mensaje) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println(mensaje);
  display.display();
}
