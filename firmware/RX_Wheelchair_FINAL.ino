/*
 * ============================================================
 * Eye-Controlled Wheelchair — RX FINAL (FIXED)
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
 * WHAT CHANGED FROM THE ORIGINAL "FINAL" VERSION
 * ------------------------------------------------------------
 * 1. TX now sends a periodic 'A' (Active-heartbeat) packet every
 *    time it is ACTIVE but not currently issuing a movement
 *    command (head centered / waiting for a gesture). Previously
 *    TX only sent packets in HALT ('W' every 500ms) or on a
 *    gesture event — so if you stayed ACTIVE without gesturing
 *    for more than 3 seconds, RX assumed TX had disconnected
 *    (COMMS_TIMEOUT_MS) and force-stopped the motors + turned the
 *    LED off, even though TX was still powered on and active.
 *    This was the most likely cause of "sometimes works, sometimes
 *    just stops for no reason."
 *
 * 2. The LED on RX now reflects TX's logical state (txActive),
 *    not just "did we get any packet." That fixes the exact
 *    problem you described: HALT and ACTIVE-idle used to look
 *    identical (both blinking the same way).
 *
 * LED STATES (D3) — exactly the 3 states you asked for:
 *   1. TX OFF / NOT CONNECTED                → LED OFF        (unchanged)
 *   2. TX ON, but PAUSED / HALT (button "off")→ FAST BLINK     (changed)
 *   3. TX ON and ACTIVE (idle OR moving)      → SOLID GLOW     (unchanged)
 *
 * ULTRASONIC:
 *   - Object < 20cm → ONLY forward ('F'/cruise) blocked
 *   - L / R / B commands still execute normally
 *
 * Pin Map:
 *   D0 (GPIO16) → ENA   D5 (GPIO14) → IN1   D6 (GPIO12) → IN2
 *   D7 (GPIO13) → IN3   D8 (GPIO15) → IN4   D4 (GPIO2)  → ENB
 *   D1 (GPIO5)  → TRIG  D2 (GPIO4)  → ECHO  D3 (GPIO0)  → LED
 *
 *   NOTE: D3/GPIO0 is a boot-mode strapping pin on ESP8266. It is
 *   fine to use as an LED output, but make sure nothing external
 *   is pulling it LOW at power-on or the board may fail to boot
 *   into normal run mode. If you ever get random boot/upload
 *   issues, move the LED to a different free GPIO (e.g. D3 spare
 *   pin elsewhere) and it'll be one less thing to debug.
 *
 *   NOTE: HC-SR04 ECHO output is a 5V signal. Feeding it straight
 *   into a 3.3V-only ESP8266 GPIO is a common reason ultrasonic
 *   readings become flaky/inconsistent over time. Add a simple
 *   resistor voltage divider (e.g. 1k + 2k) on the ECHO line, or
 *   use a 3.3V-tolerant ultrasonic module.
 * ============================================================
 */

#include <ESP8266WiFi.h>
#include <espnow.h>

// ─── Pins ────────────────────────────────────────────────────
#define ENA        16
#define IN1        14
#define IN2        12
#define IN3        13
#define IN4        15
#define ENB         2
#define TRIG_PIN    5
#define ECHO_PIN    4
#define LED_PIN     0

// ─── Speed Settings (PWM 0–1023) ────────────────────────────
#define SPEED_NORMAL     512    // 50% — forward/backward
#define SPEED_CRUISE    1023    // 100% — cruise mode
#define SPEED_TURN       250    // ~25% — turning

// ─── Turn Duration ───────────────────────────────────────────
#define TURN_DURATION_MS   300  // auto-stop after 300ms during a turn

// ─── Safety / Timing ─────────────────────────────────────────
#define OBSTACLE_CM              20
#define COMMS_TIMEOUT_MS       3000   // 3 sec with NO packet at all → disconnected
#define ULTRASONIC_INTERVAL_MS  100

// ─── LED Timing ──────────────────────────────────────────────
#define LED_BLINK_FAST_MS  150   // fast blink while TX is PAUSED/HALT

// ─── System States ───────────────────────────────────────────
typedef enum {
  STATE_DISCONNECTED,   // no packet received within timeout
  STATE_WAIT,           // motors stopped / waiting for next command
  STATE_MOVING          // movement command active
} SysState;

SysState sysState = STATE_DISCONNECTED;

// ─── Globals ─────────────────────────────────────────────────
volatile char lastCmd    = 'W';
volatile bool newCmd     = false;
unsigned long lastPktMs  = 0;
bool          everReceived = false;

// txActive = TX's logical mode as seen from the last packet:
//   true  -> TX is ACTIVE (idle heartbeat 'A' or any movement cmd)
//   false -> TX is PAUSED/HALT (heartbeat 'W')
// This drives the LED independently from motor state, so HALT and
// ACTIVE-idle no longer look the same.
bool txActive = false;

bool cruiseMode    = false;
bool obstacleFront = false;   // true = obstacle detected at front

unsigned long lastUltraMs = 0;
unsigned long lastLedMs   = 0;
bool          ledState    = false;

// Turn timer
bool          inTurn      = false;
unsigned long turnStartMs = 0;

// ─── Motor helpers ───────────────────────────────────────────
void motorsStop() {
  analogWrite(ENA, 0); analogWrite(ENB, 0);
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  inTurn = false;
  cruiseMode = false;
}

void motorsForward(int spd) {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  analogWrite(ENA, spd);   analogWrite(ENB, spd);
}

void motorsBackward(int spd) {
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
  analogWrite(ENA, spd);   analogWrite(ENB, spd);
}

void motorsLeft(int spd) {
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);  // A backward
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);   // B forward
  analogWrite(ENA, spd);   analogWrite(ENB, spd);
  inTurn = true; turnStartMs = millis();
}

void motorsRight(int spd) {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);   // A forward
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);  // B backward
  analogWrite(ENA, spd);   analogWrite(ENB, spd);
  inTurn = true; turnStartMs = millis();
}

// ─── Ultrasonic ──────────────────────────────────────────────
long readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long dur = pulseIn(ECHO_PIN, HIGH, 30000);
  return (dur == 0) ? 999 : dur / 58L;
}

// ─── ESP-NOW receive ─────────────────────────────────────────
void onReceive(uint8_t *mac, uint8_t *data, uint8_t len) {
  if (len < 1) return;
  lastCmd       = (char)data[0];
  newCmd        = true;
  lastPktMs     = millis();
  everReceived  = true;
}

// ─── LED update ──────────────────────────────────────────────
// 3 distinguishable states, exactly as requested:
//   DISCONNECTED      -> OFF
//   CONNECTED + PAUSED-> FAST BLINK
//   CONNECTED + ACTIVE-> SOLID (whether idle or actually moving)
void updateLED() {
  unsigned long now = millis();

  if (sysState == STATE_DISCONNECTED) {
    digitalWrite(LED_PIN, LOW);
    ledState = false;
    return;
  }

  if (!txActive) {
    // TX connected but PAUSED/HALT -> fast blink
    if (now - lastLedMs >= LED_BLINK_FAST_MS) {
      lastLedMs = now;
      ledState  = !ledState;
      digitalWrite(LED_PIN, ledState);
    }
  } else {
    // TX connected and ACTIVE (idle or moving) -> solid glow
    digitalWrite(LED_PIN, HIGH);
    ledState = true;
  }
}

// ─── Setup ───────────────────────────────────────────────────
void setup() {
  Serial.begin(115200); delay(200);
  Serial.println("\n=== Eye-Wheelchair RX FINAL (fixed) ===");

  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT); pinMode(ENB, OUTPUT);
  motorsStop();

  pinMode(TRIG_PIN, OUTPUT); pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  WiFi.mode(WIFI_STA); WiFi.disconnect();
  // Lock the WiFi channel explicitly. If RX/TX ever pick different
  // channels (e.g. after either board previously joined a router on
  // a different channel) ESP-NOW packets silently fail to arrive —
  // a classic cause of "works sometimes, randomly stops working."
  wifi_set_channel(1);

  Serial.print("RX MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != 0) { Serial.println("ESP-NOW INIT FAILED"); return; }
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(onReceive);

  lastPktMs = millis();
  Serial.println("Waiting for TX...");
}

// ─── Loop ────────────────────────────────────────────────────
void loop() {
  unsigned long now = millis();

  // ── Auto-stop turn after TURN_DURATION_MS ──────────────
  if (inTurn && now - turnStartMs >= TURN_DURATION_MS) {
    motorsStop();
    Serial.println("TURN complete → STOP");
    sysState = STATE_WAIT;
  }

  // ── Connection timeout check ────────────────────────────
  if (everReceived && (now - lastPktMs > COMMS_TIMEOUT_MS)) {
    // Lost connection
    motorsStop();
    sysState  = STATE_DISCONNECTED;
    txActive  = false;
    Serial.println("TX DISCONNECTED");
    updateLED();
    delay(20);
    return;
  }

  // ── Not yet received anything ───────────────────────────
  if (!everReceived) {
    sysState = STATE_DISCONNECTED;
    txActive = false;
    updateLED();
    delay(20);
    return;
  }

  // ── Ultrasonic — check forward obstacle ────────────────
  if (now - lastUltraMs >= ULTRASONIC_INTERVAL_MS) {
    lastUltraMs  = now;
    long distCm  = readDistanceCm();
    obstacleFront = (distCm < OBSTACLE_CM);
    if (obstacleFront) {
      Serial.printf("OBSTACLE at %ldcm — forward blocked\n", distCm);
    }
  }

  // ── Process incoming command ────────────────────────────
  if (newCmd) {
    newCmd = false;
    char cmd = lastCmd;
    Serial.printf("CMD: %c\n", cmd);

    // Any command OTHER than the halt-heartbeat 'W' means TX is
    // currently ACTIVE (this includes the new 'A' idle-heartbeat).
    txActive = (cmd != 'W');

    switch (cmd) {

      case 'W':
        // TX PAUSED/HALT — stop motors, wait state
        motorsStop();
        sysState = STATE_WAIT;
        break;

      case 'A':
        // TX ACTIVE but idle (no gesture this cycle). Nothing to do
        // for the motors — this packet exists purely so RX (a) does
        // not time out the connection and (b) knows TX is active.
        break;

      case 'S':
        // Stop / return to center (sent while TX is ACTIVE)
        motorsStop();
        sysState = STATE_WAIT;
        break;

      case 'F':
        // Forward — blocked if obstacle detected
        if (obstacleFront) {
          Serial.println("FORWARD BLOCKED — obstacle < 20cm");
          motorsStop();
          sysState = STATE_WAIT;
        } else {
          cruiseMode = false;
          motorsForward(SPEED_NORMAL);
          sysState = STATE_MOVING;
        }
        break;

      case 'B':
        // Backward — always allowed
        cruiseMode = false;
        motorsBackward(SPEED_NORMAL);
        sysState = STATE_MOVING;
        break;

      case 'L':
        // Left — always allowed
        cruiseMode = false;
        motorsLeft(SPEED_TURN);
        sysState = STATE_MOVING;
        break;

      case 'R':
        // Right — always allowed
        cruiseMode = false;
        motorsRight(SPEED_TURN);
        sysState = STATE_MOVING;
        break;

      case 'C':
        // Cruise ON — blocked if obstacle
        if (obstacleFront) {
          Serial.println("CRUISE BLOCKED — obstacle < 20cm");
          motorsStop();
          sysState = STATE_WAIT;
        } else {
          cruiseMode = true;
          motorsForward(SPEED_CRUISE);
          sysState = STATE_MOVING;
          Serial.println("CRUISE ON");
        }
        break;

      case 'X':
        // Cruise OFF
        cruiseMode = false;
        motorsStop();
        sysState = STATE_WAIT;
        Serial.println("CRUISE OFF");
        break;

      default:
        break;
    }
  }

  // ── If cruise active but obstacle appears — stop forward ─
  if (cruiseMode && obstacleFront) {
    Serial.println("CRUISE stopped — obstacle detected");
    motorsStop();
    cruiseMode = false;
    sysState = STATE_WAIT;
  }

  updateLED();
  delay(10);
}
