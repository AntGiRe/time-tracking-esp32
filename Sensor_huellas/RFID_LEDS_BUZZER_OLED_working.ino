#include <SPI.h>
#include <Wire.h>
#include <MFRC522.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Pines lector RFID RC522
#define SS_PIN      5
#define RST_PIN     22

// Pines LED y buzzer
#define LED_VERDE   2
#define LED_ROJO    4
#define BUZZER      15

// OLED (I2C)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_SDA      25
#define OLED_SCL      26

// Instancias
MFRC522 mfrc522(SS_PIN, RST_PIN);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// UID autorizado (modificar si es necesario)
byte UID_AUTORIZADO[] = {0x13, 0x02, 0x14, 0xF8};

// Estado actual (entrada o salida)
bool estadoFichaje = false;

// Compara dos arrays byte a byte
bool compararUID(byte *uid, byte *autorizado, byte size) {
  for (byte i = 0; i < size; i++) {
    if (uid[i] != autorizado[i]) return false;
  }
  return true;
}

// Mostrar mensaje en pantalla OLED
void mostrarMensajeOLED(String mensaje, String uid = "") {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Sistema de fichado RFID");
  display.setCursor(0, 20);
  display.println(mensaje);
  if (uid != "") {
    display.setCursor(0, 40);
    display.println("UID: " + uid);
  }
  display.display();
}

void setup() {
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();

  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_ROJO, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Error al iniciar pantalla OLED");
    while (true);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10);
  display.println("Sistema de fichado RFID");
  display.setCursor(0, 30);
  display.println("Inicializando...");
  display.display();

  Serial.println("Sistema de fichado RFID listo.");
  Serial.println("Pasa una tarjeta por el lector para ver su UID.");
}

void loop() {
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  String uidString = "";
  Serial.print("UID de la tarjeta (HEX): ");
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) Serial.print(" 0");
    else Serial.print(" ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);

    uidString += String(mfrc522.uid.uidByte[i], HEX);
    if (i < mfrc522.uid.size - 1) uidString += ":";
  }
  Serial.println();

  Serial.print("UID de la tarjeta (DEC): ");
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i]);
    Serial.print(" ");
  }
  Serial.println();

  bool tarjeta_autorizada = (mfrc522.uid.size == sizeof(UID_AUTORIZADO)) &&
                            compararUID(mfrc522.uid.uidByte, UID_AUTORIZADO, mfrc522.uid.size);

  if (tarjeta_autorizada) {
    if (!estadoFichaje) {
      Serial.println(">> FICHADA DE ENTRADA");
      mostrarMensajeOLED("Fichada de ENTRADA", uidString);
      digitalWrite(LED_VERDE, HIGH);
    } else {
      Serial.println(">> FICHADA DE SALIDA");
      mostrarMensajeOLED("Fichada de SALIDA", uidString);
      digitalWrite(LED_ROJO, HIGH);
    }

    tone(BUZZER, 1000);
    delay(500);
    noTone(BUZZER);
    digitalWrite(LED_VERDE, LOW);
    digitalWrite(LED_ROJO, LOW);
    estadoFichaje = !estadoFichaje;

  } else {
    Serial.println(">> TARJETA NO AUTORIZADA");
    mostrarMensajeOLED("NO AUTORIZADA", uidString);
    digitalWrite(LED_ROJO, HIGH);
    tone(BUZZER, 2000);
    delay(500);
    noTone(BUZZER);
    digitalWrite(LED_ROJO, LOW);
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  while (mfrc522.PICC_IsNewCardPresent() || mfrc522.PICC_ReadCardSerial()) {
    delay(50);
  }
}





