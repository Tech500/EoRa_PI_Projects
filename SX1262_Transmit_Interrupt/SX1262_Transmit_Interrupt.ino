/*

   "SX1262_Transmit_Interrupt.ino"

   RadioLib SX1262 Transmit Example

   This example transmits packets using SX1262 LoRa radio module.
   Each packet contains up to 256 bytes of data, in the form of:
    - Arduino String
    - null-terminated char array (C-string)
    - arbitrary binary data (byte array)

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/RadioLib/
*/
//Enable RadioLib debug output before anything else
#define RADIOLIB_DEBUG

#include <RadioLib.h>

// Define your board model (must match what's in utilities.h)
#define EoRa_PI_V1
#include "boards.h"


#define USING_SX1262_868M

#if defined(USING_SX1268_433M)
uint8_t txPower = 22;
float radioFreq = 433.0;
SX1268 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

#elif defined(USING_SX1262_868M)
uint8_t txPower = 22;
float radioFreq = 915.0;
SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

#endif

// or using RadioShield
// https://github.com/jgromes/RadioShield
// SX1262 radio = RadioShield.ModuleA;

// save transmission state between loops
int transmissionState = RADIOLIB_ERR_NONE;
// flag to indicate that a packet was sent
volatile bool transmittedFlag = false;


// disable interrupt when it's not needed
volatile bool interruptEnabled = true;

// this function is called when a complete packet
// is transmitted by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
void setFlag(void) {
  // check if the interrupt is enabled
  if (!interruptEnabled) {
    return;
  }

  // we sent a packet, set the flag
  transmittedFlag = true;
}


void setup() {
  Serial.begin(115200);
  initBoard();
  Serial.println("Booting Transmit Sketch...");  // Confirm setup starts
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);  // LED sanity flash

  // initialize SX1262 with default settings
  Serial.print(F("[SX1262] Initializing ... "));

  Serial.print("CS Pin: ");
  Serial.println(RADIO_CS_PIN);
  Serial.print("DIO1 Pin: ");
  Serial.println(RADIO_DIO1_PIN);
  Serial.print("RST Pin: ");
  Serial.println(RADIO_RST_PIN);
  Serial.print("BUSY Pin: ");
  Serial.println(RADIO_BUSY_PIN);

  int state = radio.begin(
    radioFreq,  // 915.0 MHz as set earlier
    125.0,      // Bandwidth (default LoRa)
    9,          // Spreading factor
    7,          // Coding rate
    RADIOLIB_SX126X_SYNC_WORD_PRIVATE,
    txPower,  // 22 dBm
    16,        // Preamble length
    0.0,      // No TCXO
    true      // 🔐 LDO mode ON
  );



  // set the function that will be called
  // when packet transmission is finished
  radio.setPacketSentAction(setFlag);

  // start transmitting the first packet
  Serial.print(F("[SX1262] Sending first packet ... "));

  // you can transmit C-string or Arduino string up to
  // 256 characters long
  transmissionState = radio.startTransmit("Hello World!");

  // you can also transmit byte array up to 256 bytes long
  /*
    byte byteArr[] = {0x01, 0x23, 0x45, 0x67,
                      0x89, 0xAB, 0xCD, 0xEF};
    state = radio.startTransmit(byteArr, 8);
  */

  pinMode(48, INPUT);
}

// counter to keep track of transmitted packets
int count = 0;

void loop() {
  // check if the previous transmission finished
  if (transmittedFlag) {
    // reset flag
    transmittedFlag = false;

    if (transmissionState == RADIOLIB_ERR_NONE) {
      // packet was successfully sent
      Serial.println(F("transmission finished!"));

      digitalWrite(BOARD_LED, HIGH);  // Signal transmission started
      delay(100);                     // Brief flash
      digitalWrite(BOARD_LED, LOW);   // Back to idle
      Serial.println("LED flash complete");

      // NOTE: when using interrupt-driven transmit method,
      //       it is not possible to automatically measure
      //       transmission data rate using getDataRate()

    } else {
      Serial.print(F("failed, code "));
      Serial.println(transmissionState);
    }

    // clean up after transmission is finished
    // this will ensure transmitter is disabled,
    // RF switch is powered down etc.
    radio.finishTransmit();

    // wait a second before transmitting again
    delay(1000);

    // send another one
    Serial.print(F("[SX1262] Sending another packet ... "));

    // you can transmit C-string or Arduino string up to
    // 256 characters long
    String str = "Hello World! #" + String(count++);
    transmissionState = radio.startTransmit(str);

    // you can also transmit byte array up to 256 bytes long
    /*
      byte byteArr[] = {0x01, 0x23, 0x45, 0x67,
                        0x89, 0xAB, 0xCD, 0xEF};
      transmissionState = radio.startTransmit(byteArr, 8);
    */
  }
}