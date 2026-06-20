/*
 * ============================================================
 * MPU6050 Head-Tilt Calibration Helper — Eye-Controlled Wheelchair
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
 * The main TX sketch decides FORWARD / BACKWARD / LEFT / RIGHT by
 * comparing live accelerometer values against 4 fixed numbers:
 *
 *   #define FORWARD_AY_MIN    4000
 *   #define BACKWARD_AY_MAX  -2000
 *   #define LEFT_AZ_MIN       4500
 *   #define RIGHT_AZ_MAX     -2500
 *
 * Those numbers depend entirely on exactly how the MPU6050 sits on
 * YOUR glasses/headset — its mounting angle, which axis faces which
 * way, even how snug the frame is. They are NOT universal. Using
 * someone else's numbers as-is is a common reason gestures don't
 * trigger at all, or trigger when the head isn't even tilted.
 *
 * ------------------------------------------------------------
 * HOW TO USE
 * ------------------------------------------------------------
 *   1. Flash THIS sketch onto your TX (glasses) board, wearing it
 *      the same way you normally would.
 *   2. Open Serial Monitor at 115200 baud.
 *   3. Follow the on-screen prompts: hold CENTER, then FORWARD,
 *      BACKWARD, LEFT, and RIGHT — 3 seconds each.
 *   4. At the end it prints a ready-to-paste #define block.
 *   5. Copy that block into TX_Glasses_FINAL.ino, replacing the
 *      existing "CALIBRATED THRESHOLDS" section, then re-flash the
 *      real TX sketch.
 *
 * To re-run calibration, just reset (or re-flash) the board.
 *
 * Pin Map (same as main TX sketch):
 *   D1 (GPIO5) → SCL    D2 (GPIO4) → SDA
 * ============================================================
 */

#include <Wire.h>
#include <MPU6050.h>

#define PIN_SCL 5   // D1
#define PIN_SDA 4   // D2

#define HOLD_MS     3000   // how long to hold + record each position
#define SAMPLE_MS     20   // sampling interval while holding

MPU6050 mpu;
int16_t ax, ay, az, gx, gy, gz;

long centerAY = 0, centerAZ = 0;
long peakForwardAY  = 0;
long peakBackwardAY = 0;
long peakLeftAZ     = 0;
long peakRightAZ    = 0;

void readMPU() {
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
}

void countdown(int seconds) {
  for (int i = seconds; i > 0; i--) {
    Serial.printf("   ...%d\n", i);
    delay(1000);
  }
}

// Samples for HOLD_MS and returns whichever extreme (max or min) was seen.
long captureExtreme(bool useAY, bool extremeIsMax) {
  unsigned long start = millis();
  long extreme = 0;
  bool first = true;
  while (millis() - start < HOLD_MS) {
    readMPU();
    long val = useAY ? ay : az;
    if (first) { extreme = val; first = false; }
    else if (extremeIsMax && val > extreme) extreme = val;
    else if (!extremeIsMax && val < extreme) extreme = val;
    Serial.printf("   AY=%6d  AZ=%6d\n", ay, az);
    delay(SAMPLE_MS);
  }
  return extreme;
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n================================================");
  Serial.println(" MPU6050 Head-Tilt Calibration Helper");
  Serial.println(" Humanix Tech Lab — youtube.com/@HumanixTechLab");
  Serial.println("================================================\n");

  Wire.begin(PIN_SDA, PIN_SCL);
  mpu.initialize();
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 NOT FOUND — check SDA/SCL wiring and power, then reset.");
    while (true) delay(1000);
  }
  Serial.println("MPU6050 connected OK.\n");
  delay(500);

  // ── CENTER baseline ──────────────────────────────────────
  Serial.println("STEP 1/5: Keep your head CENTERED (straight, neutral).");
  Serial.println("Starting in:");
  countdown(3);
  Serial.println("Recording CENTER for 3 seconds...");
  long sumAY = 0, sumAZ = 0, n = 0;
  unsigned long start = millis();
  while (millis() - start < HOLD_MS) {
    readMPU();
    sumAY += ay; sumAZ += az; n++;
    Serial.printf("   AY=%6d  AZ=%6d\n", ay, az);
    delay(SAMPLE_MS);
  }
  centerAY = sumAY / n;
  centerAZ = sumAZ / n;
  Serial.printf("CENTER baseline -> AY=%ld  AZ=%ld\n\n", centerAY, centerAZ);

  // ── FORWARD ──────────────────────────────────────────────
  Serial.println("STEP 2/5: Tilt your head FORWARD and HOLD.");
  Serial.println("Starting in:");
  countdown(3);
  Serial.println("Recording FORWARD for 3 seconds...");
  peakForwardAY = captureExtreme(true, true);
  Serial.printf("FORWARD peak AY = %ld\n\n", peakForwardAY);

  // ── BACKWARD ─────────────────────────────────────────────
  Serial.println("STEP 3/5: Tilt your head BACKWARD and HOLD.");
  Serial.println("Starting in:");
  countdown(3);
  Serial.println("Recording BACKWARD for 3 seconds...");
  peakBackwardAY = captureExtreme(true, false);
  Serial.printf("BACKWARD peak AY = %ld\n\n", peakBackwardAY);

  // ── LEFT ─────────────────────────────────────────────────
  Serial.println("STEP 4/5: Tilt your head LEFT and HOLD.");
  Serial.println("Starting in:");
  countdown(3);
  Serial.println("Recording LEFT for 3 seconds...");
  peakLeftAZ = captureExtreme(false, true);
  Serial.printf("LEFT peak AZ = %ld\n\n", peakLeftAZ);

  // ── RIGHT ────────────────────────────────────────────────
  Serial.println("STEP 5/5: Tilt your head RIGHT and HOLD.");
  Serial.println("Starting in:");
  countdown(3);
  Serial.println("Recording RIGHT for 3 seconds...");
  peakRightAZ = captureExtreme(false, false);
  Serial.printf("RIGHT peak AZ = %ld\n\n", peakRightAZ);

  // ── Recommended thresholds: 65% of the way from CENTER to the
  //    recorded peak, so a clear (but not maximal) tilt reliably
  //    crosses the line every time. ──────────────────────────
  long fwdThresh   = centerAY + (long)((peakForwardAY  - centerAY) * 0.65);
  long bwdThresh   = centerAY + (long)((peakBackwardAY - centerAY) * 0.65);
  long leftThresh  = centerAZ + (long)((peakLeftAZ     - centerAZ) * 0.65);
  long rightThresh = centerAZ + (long)((peakRightAZ    - centerAZ) * 0.65);

  Serial.println("================================================");
  Serial.println(" CALIBRATION COMPLETE — paste this block into");
  Serial.println(" TX_Glasses_FINAL.ino, replacing the existing");
  Serial.println(" CALIBRATED THRESHOLDS section:");
  Serial.println("================================================\n");

  Serial.println("// ╔══════════════════════════════════════════════════╗");
  Serial.println("// ║   CALIBRATED THRESHOLDS — edit if needed        ║");
  Serial.println("// ╚══════════════════════════════════════════════════╝");
  Serial.printf("#define FORWARD_AY_MIN    %ld   // AY above this = FORWARD tilt\n", fwdThresh);
  Serial.printf("#define BACKWARD_AY_MAX   %ld   // AY below this = BACKWARD tilt\n", bwdThresh);
  Serial.printf("#define LEFT_AZ_MIN       %ld   // AZ above this = LEFT tilt\n", leftThresh);
  Serial.printf("#define RIGHT_AZ_MAX      %ld   // AZ below this = RIGHT tilt\n", rightThresh);

  Serial.println("\nRaw values recorded (for reference / manual tuning):");
  Serial.printf("  CENTER   AY=%ld  AZ=%ld\n", centerAY, centerAZ);
  Serial.printf("  FORWARD  peak AY=%ld\n", peakForwardAY);
  Serial.printf("  BACKWARD peak AY=%ld\n", peakBackwardAY);
  Serial.printf("  LEFT     peak AZ=%ld\n", peakLeftAZ);
  Serial.printf("  RIGHT    peak AZ=%ld\n", peakRightAZ);

  Serial.println("\nIf a gesture feels too sensitive or not sensitive enough");
  Serial.println("once testing the real TX sketch, nudge these numbers by");
  Serial.println("hand — closer to CENTER makes it easier to trigger,");
  Serial.println("closer to the peak requires a bigger, more deliberate tilt.");
  Serial.println("\nLive AY/AZ values will keep printing below for fine-tuning.");
  Serial.println("Reset (or re-flash) this board to run calibration again.\n");
}

void loop() {
  // Calibration itself is a one-time routine in setup(). Live values
  // are still printed here so you can fine-tune by eye if needed.
  readMPU();
  Serial.printf("LIVE  AY=%6d  AZ=%6d\n", ay, az);
  delay(300);
}
