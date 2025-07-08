//EoRa_PI_Receiver.ino

//Enable RadioLib debug output
#define RADIOLIB_DEBUG

#include <WiFiManager.h>
#include <Update.h>
#include <RadioLib.h>

#define EoRa_PI_V1
#include "boards.h"

// ⬇ Pin Definitions
#define RADIO_SCLK_PIN 5
#define RADIO_MISO_PIN 38
#define RADIO_MOSI_PIN 6
#define RADIO_CS_PIN 7
#define RADIO_DIO1_PIN 33
#define RADIO_BUSY_PIN 34
#define RADIO_RST_PIN 8
#define TX_LED_PIN 37

#define SCL_PIN 36
#define SDA_PIN 16
#define KY002S_TRIGGER 12
#define INA226_ALERT 15

// ⬇ LoRa radio instance
SX1262 radio = new Module(
  RADIO_CS_PIN,
  RADIO_DIO1_PIN,
  RADIO_RST_PIN,
  RADIO_BUSY_PIN,
  SPI,
  RADIOLIB_DEFAULT_SPI_SETTINGS);
volatile bool scanFlag = false;
volatile bool packetReceived = false;
volatile bool alertTriggered = false;
bool otaInProgress = false;

// --- Persistent Packet Counter ---
RTC_DATA_ATTR unsigned long pktCount = 0;

// --- Interrupt Handlers ---
void setFlag() {
  scanFlag = true;
}

void onReceive() {
  packetReceived = true;
}

void alert() {
  alertTriggered = true;
}

// --- Channel Activity Detection Start ---
void startChannelScan() {
  if (!otaInProgress) {
    radio.setDio1Action(setFlag);
    radio.startChannelScan();
    Serial.println("🔍 Starting CAD scan...");
  }
}

// --- Power Monitoring & OTA ---
void prepareForOTA() {
  otaInProgress = true;
  detachInterrupt(RADIO_DIO1_PIN);
  detachInterrupt(INA226_ALERT);
  radio.standby();
  Serial.println("🌐 Entering OTA Mode...");
}

// --- Setup ---
void setup() {
  Serial.begin(115200);
  pinMode(TX_LED_PIN, OUTPUT);
  digitalWrite(TX_LED_PIN, HIGH);
  delay(500);
  digitalWrite(TX_LED_PIN, LOW);

  SPI.begin(SCK, MISO, MOSI, SS);

  int state = radio.begin();
  radio.setFrequency(915.0);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.printf("❌ Radio init failed, code %d\n", state);
    while (true) delay(1000);
  } else {
    Serial.println("✅ Radio initialized.");
  }

  if (state != RADIOLIB_ERR_NONE) {
    Serial.printf("❌ Radio init failed, code %d\n", state);
    while (true) delay(1000);
  }

  radio.setDio1Action(setFlag);

  attachInterrupt(RADIO_DIO1_PIN, setFlag, RISING);
  attachInterrupt(INA226_ALERT, alert, FALLING);

  startChannelScan();
}

// --- Main Loop ---
void loop() {
  // --- Handle CAD Result ---
  if (scanFlag) {
    scanFlag = false;

    int result = radio.getChannelScanResult();
    if (result == RADIOLIB_LORA_DETECTED) {
      Serial.println("📡 LoRa activity detected.");
      radio.setDio1Action(onReceive);
      radio.startReceive();  // ISR-style
    } else {
      Serial.println("🔁 No activity. Rescanning...");
      startChannelScan();
    }
  }

  // --- Handle Incoming Packet ---
  if (packetReceived) {
    packetReceived = false;
    digitalWrite(TX_LED_PIN, HIGH);

    String msg;
    int state = radio.readData(msg);
    if (state == RADIOLIB_ERR_NONE) {
      pktCount++;
      Serial.printf("📥 [%lu] Packet: %s\n", pktCount, msg.c_str());
    } else {
      Serial.println("⚠️ Receive error.");
    }

    digitalWrite(TX_LED_PIN, LOW);
    radio.setDio1Action(setFlag);
    startChannelScan();
  }

  // --- Handle Power Alert / OTA ---
  if (alertTriggered) {
    alertTriggered = false;
    prepareForOTA();

    WiFiManager wm;
    bool res = wm.autoConnect("EoRaSetup");
    if (!res) {
      Serial.println("❌ OTA setup failed. Rebooting...");
      ESP.restart();
    } else {
      Serial.println("✅ WiFi connected. Ready for OTA.");
    }

    while (WiFi.status() == WL_CONNECTED) {
      delay(500);
      // Add OTA.handle() here if using ArduinoOTA
    }

    Serial.println("🔁 Rebooting after OTA...");
    ESP.restart();
  }
}