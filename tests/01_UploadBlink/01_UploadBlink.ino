// 01_UploadBlink - Stage 1 verification for the 2-Part ProMini EEprom Logger
// ---------------------------------------------------------------------------
// PURPOSE: confirms the ProMini has a WORKING BOOTLOADER, that your UART
// adapter / drivers / IDE board settings are correct, and (after Stage 4)
// that the RGB indicator LED is wired correctly on A0/A1/A2.
//
// Run this BEFORE cutting anything off the ProMini board (per the tutorial:
// "Do not progress with the build until you have confirmed a working bootloader")
// and again after the RGB LED is installed.
//
// BOARD SETTINGS:  Arduino Pro or Pro Mini / ATmega328P (3.3V, 8MHz)
// SERIAL MONITOR:  500000 baud  (500000 = 8MHz/16 -> zero clock error)
//
// EXPECTED BEHAVIOUR:
//   - Serial monitor prints a banner then "Heartbeat: n" once per second
//   - LEDs rotate: RED (onboard D13) -> GREEN (A1) -> BLUE (A2), 1 second each
//   - Before the RGB LED is installed only the red D13 blink is visible: PASS
//   - After RGB install all three colours must light: PASS
//
// The green/blue channels are lit through the 328p's internal pullup
// resistors (~36k) with A0 driven LOW as the LED's ground leg - the same
// trick the main logger code uses to keep indicator current below 50uA.
// No libraries required.

#define RED_PIN 13   // onboard LED, stays in place on this build
#define GND_PIN A0   // RGB common cathode (red leg of RGB led is CUT)
#define GREEN_PIN A1
#define BLUE_PIN A2

uint32_t heartbeat = 0;

void setup() {
  Serial.begin(500000);
  Serial.println();
  Serial.println(F("====================================================="));
  Serial.println(F(" 01_UploadBlink  - bootloader / UART / LED check"));
  Serial.print  (F(" Compiled: ")); Serial.print(F(__DATE__));
  Serial.print  (F(" ")); Serial.println(F(__TIME__));
  Serial.println(F(" PASS if: heartbeat prints every second AND"));
  Serial.println(F("          LEDs rotate RED -> GREEN -> BLUE"));
  Serial.println(F(" (no RGB LED installed yet? red-only is a PASS)"));
  Serial.println(F("====================================================="));

  pinMode(RED_PIN, OUTPUT);  digitalWrite(RED_PIN, LOW);
  pinMode(GND_PIN, OUTPUT);  digitalWrite(GND_PIN, LOW); // common ground leg
  pinMode(GREEN_PIN, INPUT); // INPUT = off, INPUT_PULLUP = dimly lit
  pinMode(BLUE_PIN, INPUT);
}

void allOff() {
  digitalWrite(RED_PIN, LOW);
  pinMode(GREEN_PIN, INPUT);
  pinMode(BLUE_PIN, INPUT);
}

void loop() {
  heartbeat++;
  Serial.print(F("Heartbeat: ")); Serial.print(heartbeat);

  switch (heartbeat % 3) {
    case 0:
      Serial.println(F("  [RED  D13]"));
      allOff(); digitalWrite(RED_PIN, HIGH);
      break;
    case 1:
      Serial.println(F("  [GREEN A1]"));
      allOff(); pinMode(GREEN_PIN, INPUT_PULLUP);
      break;
    case 2:
      Serial.println(F("  [BLUE  A2]"));
      allOff(); pinMode(BLUE_PIN, INPUT_PULLUP);
      break;
  }
  delay(1000);
}
