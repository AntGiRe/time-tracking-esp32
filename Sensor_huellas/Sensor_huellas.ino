#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>
#include <SPI.h>
#include <MFRC522.h>

// ---------------------
// Pines y configuración
// ---------------------

// Sensor de huellas (UART por Serial2)
HardwareSerial mySerial(2);  // RX = GPIO16, TX = GPIO17
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// Sensor RFID RC522 (SPI)
#define SS_PIN 5
#define RST_PIN 22
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Pines para salidas
#define LED_VERDE 14
#define LED_ROJO 27
#define BUZZER 26

// UIDs autorizados (usa tus propios valores aquí)
const byte authorizedUIDs[][4] = {
  {0x5B, 0x9D, 0xAA, 0x00},
  {0x09, 0xCE, 0x49, 0x05}
};
const int numUIDs = sizeof(authorizedUIDs) / sizeof(authorizedUIDs[0]);

int currentId = 1;

// ---------------------
// Setup
// ---------------------
void setup() {
  Serial.begin(115200);
  while (!Serial);

  // Inicializar salidas
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_ROJO, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_ROJO, LOW);
  digitalWrite(BUZZER, LOW);

  // Inicializar sensor de huellas
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
  Serial.println("Escribe 'r' y pulsa ENTER para registrar una nueva huella.");

  // Inicializar sensor RFID
  SPI.begin();  // SPI = GPIO 18, 19, 23
  mfrc522.PCD_Init();
  Serial.println("✅ Sensor RFID listo.");
}

// ---------------------
// Loop principal
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
    accesoConcedido("Huella válida");
  } else if (resultado == FINGERPRINT_NOTFOUND) {
    Serial.println("❌ Huella no reconocida.");
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
  p = finger.storeModel(id);
  return p;
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
void checkRFID() {
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) return;

  Serial.print("🎟️  RFID detectado. UID: ");
  byte *uid = mfrc522.uid.uidByte;
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(uid[i] < 0x10 ? " 0" : " ");
    Serial.print(uid[i], HEX);
  }
  Serial.println();

  bool autorizado = false;
  for (int i = 0; i < numUIDs; i++) {
    bool coincide = true;
    for (int j = 0; j < 4; j++) {
      if (authorizedUIDs[i][j] != uid[j]) {
        coincide = false;
        break;
      }
    }
    if (coincide) {
      autorizado = true;
      break;
    }
  }

  if (autorizado) {
    accesoConcedido("RFID autorizado");
  } else {
    accesoDenegado("RFID no autorizado");
  }

  mfrc522.PICC_HaltA();
}

// ============================
// FUNCIONES DE CONTROL
// ============================
void accesoConcedido(String fuente) {
  Serial.println("✅ Acceso concedido por " + fuente);
  digitalWrite(LED_VERDE, HIGH);
  digitalWrite(BUZZER, HIGH);
  delay(200);
  digitalWrite(BUZZER, LOW);
  delay(1000);
  digitalWrite(LED_VERDE, LOW);
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