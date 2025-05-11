#include <WiFi.h>
#include <ESPSupabase.h>
#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>
#include <SPI.h>
#include <MFRC522.h>

// ---------------------
// Pines y configuración
// ---------------------
HardwareSerial mySerial(2);  // RX = GPIO16, TX = GPIO17
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
#define SS_PIN 5
#define RST_PIN 22
MFRC522 mfrc522(SS_PIN, RST_PIN);
#define LED_VERDE 14
#define LED_ROJO 27
#define BUZZER 26

// ---------------------
// Wi-Fi y Supabase
// ---------------------
const char* ssid = "vodafone7801";
const char* password = "5BDJYZMCC74C85";
const char* supabaseUrl = "https://kduoudmniqjtartllozn.supabase.co";
const char* supabaseKey = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImtkdW91ZG1uaXFqdGFydGxsb3puIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NDM5MzUwNzcsImV4cCI6MjA1OTUxMTA3N30.T1mpTXZaGF8AIzvQ96jeQh95Vo4cfLCiWh0QSH0Ea84"; // Oculto por seguridad

Supabase supabase;

// ---------------------
// Config estática
// ---------------------
const int STATIC_EMPLOYEE_ID = 3;
const int STATIC_LOCATION_ID = 1;

int currentId = 1;

// ---------------------
// Setup
// ---------------------
void setup() {
  Serial.begin(115200);
  while (!Serial);

  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_ROJO, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_ROJO, LOW);
  digitalWrite(BUZZER, LOW);

  // Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Conectando a Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Wi-Fi conectado");


  // Supabase
  supabase.begin(supabaseUrl, supabaseKey);
  Serial.println("✅ Supabase inicializado");

  // Sensor huellas
  mySerial.begin(57600, SERIAL_8N1, 16, 17);
  finger.begin(57600);
  Serial.println("Inicializando sensor de huellas...");
  if (finger.verifyPassword()) {
    Serial.println("✅ Sensor de huellas OK.");
  } else {
    Serial.println("❌ Error al iniciar sensor de huellas.");
    while (1);
  }
  finger.getTemplateCount();
  currentId = finger.templateCount + 1;

  // Sensor RFID
  SPI.begin();  // SPI = GPIO 18, 19, 23
  mfrc522.PCD_Init();
  Serial.println("✅ Sensor RFID listo.");
}

// ---------------------
// Loop
// ---------------------
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

  // Verificar huella
  uint8_t resultado = checkFingerprint();
  if (resultado == FINGERPRINT_OK) {
  accesoConcedidoFingerprint(finger.fingerID);
  } else if (resultado == FINGERPRINT_NOTFOUND) {
    accesoDenegado("Huella no válida");
  }

  // Verificar tarjeta RFID
  checkRFID();

  delay(500);
}

// ============================
// FUNCIONES DE HUELLA
// ============================
uint8_t enrollFingerprint(int id) {
  int p = -1;
  Serial.println("📲 Coloca un dedo...");
  while ((p = finger.getImage()) != FINGERPRINT_OK);
  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) return p;
  Serial.println("✅ Primera imagen tomada. Retira el dedo...");
  delay(2000);
  while ((p = finger.getImage()) != FINGERPRINT_NOFINGER);
  Serial.println("📲 Vuelve a colocar el mismo dedo...");
  while ((p = finger.getImage()) != FINGERPRINT_OK);
  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) return p;
  Serial.println("⏳ Comparando huellas...");
  p = finger.createModel();
  if (p != FINGERPRINT_OK) return p;
  Serial.println("💾 Guardando huella con ID: " + String(id));
  return finger.storeModel(id);
}

uint8_t checkFingerprint() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) return p;
  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return p;
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("🎉 Huella válida. ID: " + String(finger.fingerID));
    return FINGERPRINT_OK;
  }
  return FINGERPRINT_NOTFOUND;
}

// ============================
// FUNCIONES DE RFID
// ============================

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
    int employeeId = idStr.toInt();

    Serial.println("🧑‍💼 Employee ID encontrado: " + String(employeeId));
    return employeeId;
  } else {
    Serial.println("⚠️ UID no registrado en Supabase");
    return -1;
  }
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

  Serial.println("🎟️  UID leído: " + uidStr);

  int employeeId = getEmployeeIdFromRFID(uidStr);
  if (employeeId > 0) {
    accesoConcedidoRFID(employeeId, uidStr);
  } else {
    accesoDenegado("RFID no autorizado");
  }

  mfrc522.PICC_HaltA();
}

// ============================
// FUNCIONES DE CONTROL
// ============================
void accesoConcedidoRFID(int employeeId, String uidStr) {
  Serial.println("✅ Acceso concedido por RFID");
  digitalWrite(LED_VERDE, HIGH);
  digitalWrite(BUZZER, HIGH);
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
  if (response == 201) {
    Serial.println("📝 Registro insertado correctamente en Supabase");
  } else {
    Serial.print("⚠️ Error al insertar en Supabase. Código: ");
    Serial.println(response);
  }
}

void accesoConcedidoFingerprint(int fingerprintId) {
  Serial.println("✅ Acceso concedido por huella");
  digitalWrite(LED_VERDE, HIGH);
  digitalWrite(BUZZER, HIGH);
  delay(200);
  digitalWrite(BUZZER, LOW);
  delay(1000);
  digitalWrite(LED_VERDE, LOW);

  String json = "{";
  json += "\"employee_id\": " + String(STATIC_EMPLOYEE_ID) + ",";  // Por ahora sigue siendo estático
  json += "\"location_id\": " + String(STATIC_LOCATION_ID) + ",";
  json += "\"access_method\": \"fingerprint\",";
  json += "\"fingerprint_id\": " + String(fingerprintId) + ",";
  json += "\"rfid_card_code\": \"EMPTY\"";
  json += "}";

  int response = supabase.insert("access_logs", json, false);
  if (response == 201) {
    Serial.println("📝 Registro insertado correctamente en Supabase");
  } else {
    Serial.print("⚠️ Error al insertar en Supabase. Código: ");
    Serial.println(response);
  }
}

void accesoDenegado(String fuente) {
  Serial.println("❌ Acceso denegado por " + fuente);
  for (int i = 0; i < 2; i++) {
    digitalWrite(LED_ROJO, HIGH);
    digitalWrite(BUZZER, HIGH);
    delay(150);
    digitalWrite(LED_ROJO, LOW);
    digitalWrite(BUZZER, LOW);
    delay(150);
  }
}