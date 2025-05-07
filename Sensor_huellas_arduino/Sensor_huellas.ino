#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>

HardwareSerial mySerial(2);  // Serial2: GPIO16 (RX2), GPIO17 (TX2)
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// Variable para asignar IDs automáticamente
int currentId = 1;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  mySerial.begin(57600, SERIAL_8N1, 16, 17);
  finger.begin(57600);

  Serial.println("Inicializando sensor de huellas...");
  if (finger.verifyPassword()) {
    Serial.println("✅ Sensor conectado y autenticado.");
  } else {
    Serial.println("❌ Error al conectar con el sensor.");
    while (1);
  }

  finger.getTemplateCount();
  currentId = finger.templateCount + 1;

  Serial.print("Capacidad total del sensor: ");
Serial.println(finger.capacity);

  Serial.println("Escribe 'r' y pulsa ENTER para registrar una nueva huella.");
  Serial.println("O coloca un dedo para verificar.");
}

void loop() {
  // Registrar nueva huella si el usuario lo pide
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'r') {
      Serial.println("🔐 Registro de nueva huella...");
      if (enrollFingerprint(currentId) == FINGERPRINT_OK) {
        Serial.println("✅ Huella registrada con ID: " + String(currentId));
        currentId++;
      } else {
        Serial.println("❌ No se pudo registrar la huella.");
      }
    }
  }

  // Verificar huella colocada
  uint8_t resultado = getFingerprintID();
  if (resultado == FINGERPRINT_OK) {
    // Huella reconocida, ya se imprimió el ID
  } else if (resultado == FINGERPRINT_NOTFOUND) {
    Serial.println("❌ Huella no reconocida.");
  }

  delay(1000);
}

// ============================
// FUNCIONES DE REGISTRO Y LECTURA
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

uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) return p;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return p;

  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("🎉 Huella reconocida. ID: " + String(finger.fingerID));
    return FINGERPRINT_OK;
  }

  return FINGERPRINT_NOTFOUND;
}
