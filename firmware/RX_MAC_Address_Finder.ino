/*
 * ============================================================
 * RX MAC Address Finder — Eye-Controlled Wheelchair
 *
 * Project by Humanix Tech Lab
 *   YouTube   : https://www.youtube.com/@HumanixTechLab
 *   GitHub    : https://github.com/humanixtechlab
 *   Instagram : https://www.instagram.com/humanixtechlab/
 *   Facebook  : https://www.facebook.com/humanixtechlab/
 * If this project helped you, please subscribe / follow —
 * it really helps support more builds like this one!
 *
 * ------------------------------------------------------------
 * WHY YOU NEED THIS
 * ------------------------------------------------------------
 * The TX (glasses) sketch needs the EXACT MAC address of your RX
 * (wheelchair) board hardcoded into this one line:
 *
 *     uint8_t rxMAC[] = {0xC4, 0x5B, 0xBE, 0xC8, 0xF6, 0x27};
 *
 * Every ESP8266 board ships with a DIFFERENT MAC address, so that
 * sample value will never match your hardware. Skip this step and
 * everything still "looks" fine when you power it on — TX runs,
 * sensors read, nothing crashes — but every packet TX sends gets
 * silently dropped. RX's LED just stays OFF forever, which looks
 * exactly like "TX is powered off" even though it isn't. This is
 * the #1 reason a freshly cloned copy of this project "does
 * nothing" for someone.
 *
 * ------------------------------------------------------------
 * HOW TO USE
 * ------------------------------------------------------------
 *   1. Flash THIS sketch onto your RX (wheelchair) board.
 *   2. Open Serial Monitor at 115200 baud.
 *   3. Copy the printed "uint8_t rxMAC[] = {...};" line exactly.
 *   4. Paste it into TX_Glasses_FINAL.ino, replacing the existing
 *      rxMAC[] line near the top of the file.
 *   5. Re-flash the real RX_Wheelchair_FINAL.ino back onto this
 *      board afterward — this finder sketch does not drive motors.
 * ============================================================
 */

#include <ESP8266WiFi.h>

void printMac() {
  String mac = WiFi.macAddress();   // e.g. "C4:5B:BE:C8:F6:27"

  // Build "uint8_t rxMAC[] = {0xXX, 0xXX, ...};" straight from it
  String arrayLine = "uint8_t rxMAC[] = {";
  for (int i = 0; i < 6; i++) {
    String byteStr = mac.substring(i * 3, i * 3 + 2);
    arrayLine += "0x" + byteStr;
    if (i < 5) arrayLine += ", ";
  }
  arrayLine += "};";

  Serial.println("\n================================================");
  Serial.println(" RX MAC ADDRESS FINDER  —  Humanix Tech Lab");
  Serial.println(" youtube.com/@HumanixTechLab");
  Serial.println("================================================\n");
  Serial.print("This board's MAC address : ");
  Serial.println(mac);
  Serial.println("\nCopy the line below EXACTLY into TX_Glasses_FINAL.ino,");
  Serial.println("replacing the existing rxMAC[] line:\n");
  Serial.println(arrayLine);
  Serial.println("\n================================================\n");
}

void setup() {
  Serial.begin(115200);
  delay(300);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  printMac();
}

void loop() {
  // Re-print every 5 seconds in case you opened Serial Monitor late.
  delay(5000);
  printMac();
}
