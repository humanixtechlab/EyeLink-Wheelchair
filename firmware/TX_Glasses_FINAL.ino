/*
 * ============================================================
 * Eye-Controlled Wheelchair — TX FINAL (FIXED)
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
 * 1. NEW: while ACTIVE, TX now sends an 'A' (Active-heartbeat)
 *    packet every ACTIVE_HEARTBEAT_MS whenever it hasn't just sent
 *    a gesture command. Before this fix, once you pressed the
 *    button to go ACTIVE, TX sent NOTHING at all until a gesture
 *    matched. If you stayed centered/idle (or kept moving forward
 *    without re-gesturing) for more than 3 seconds, RX's 3-second
 *    "no packet = disconnected" timeout would fire even though TX
 *    was still on — motors would force-stop and the LED would
 *    drop to OFF. This is the most likely reason it "sometimes
 *    works, sometimes just stops."
 *
 * 2. CHANGED: when the button toggles to PAUSE/HALT, TX now sends
 *    'W' immediately (instead of 'S' followed by 'W' up to 500ms
 *    later). This removes a brief ambiguous moment where RX
 *    couldn't tell if you'd just paused or just centered your head
 *    while still active.
 *
 * Together with the matching RX update, the single onboard LED on
 * the receiver now gives you 3 distinguishable states:
 *   1. LED OFF         -> TX off / out of range / not paired
 *   2. LED FAST BLINK  -> TX on, but PAUSED (button toggled to HALT)
 *   3. LED SOLID GLOW  -> TX on and ACTIVE (idle or actually moving)
 *
 * BUTTON (D7) — SINGLE PRESS ONLY:
 *   System starts in HALT by default.
 *   Press once → ACTIVE (movement enabled)
 *   Press again → HALT (everything stops)
 *   Long press feature REMOVED.
 *
 * COMMANDS SENT TO RX:
 *   F = Forward     B = Backward    L = Left    R = Right
 *   S = Stop        C = Cruise ON   X = Cruise OFF
 *   W = PAUSED/Halt heartbeat   (sent every 500ms while halted)
 *   A = ACTIVE-idle heartbeat   (sent every 300ms while active & idle)
 *
 * GESTURE TRIGGERS (AFTER SWAP):
 *   Tilt head FORWARD  + double blink BOTH eyes  → BACKWARD (B)
 *   Tilt head BACKWARD + double blink BOTH eyes  → FORWARD  (F)
 *   Tilt head LEFT     + double blink LEFT eye   → LEFT     (L)
 *   Tilt head RIGHT    + double blink RIGHT eye  → RIGHT    (R)
 *   Head returns to CENTER                       → STOP     (S)
 *   Tilt FORWARD  + triple blink BOTH eyes       → CRUISE ON (C)
 *   Triple blink BOTH eyes (any position)        → CRUISE OFF (X)
 *
 * YOUR AXIS MAPPING (calibrated):
 *   FORWARD  → AY > 4000   BACKWARD → AY < -2000
 *   LEFT     → AZ > 4500   RIGHT    → AZ < -2500
 *
 * Pin Map:
 *   D1 (GPIO5)  → SCL    D2 (GPIO4)  → SDA
 *   D5 (GPIO14) → RIGHT EYE sensor
 *   D6 (GPIO12) → LEFT EYE sensor
 *   D7 (GPIO13) → BUTTON (active LOW, internal pull-up)
 *
 *   NOTE: rxMAC[] below MUST exactly match the MAC address that
 *   the RX board prints on its own Serial Monitor at boot ("RX
 *   MAC: ..."). If you ever re-flash or swap the RX board, its MAC
 *   can change — a mismatched MAC means packets are silently
 *   dropped (TX looks fine, RX LED just stays OFF, looking exactly
 *   like "TX is off"). Worth double-checking this first whenever
 *   the link seems dead.
 * ============================================================
 */

#include <Wire.h>
#include <MPU6050.h>
#include <ESP8266WiFi.h>
#include <espnow.h>

// ─── Pins ────────────────────────────────────────────────────
#define PIN_SCL        5    // D1
#define PIN_SDA        4    // D2
#define PIN_RIGHT_EYE 14    // D5
#define PIN_LEFT_EYE  12    // D6
#define PIN_BUTTON    13    // D7

// ─── RX MAC Address — update if your RX MAC is different ─────
uint8_t rxMAC[] = {0xC4, 0x5B, 0xBE, 0xC8, 0xF6, 0x27};

// ╔══════════════════════════════════════════════════╗
// ║   CALIBRATED THRESHOLDS — edit if needed        ║
// ╚══════════════════════════════════════════════════╝
#define FORWARD_AY_MIN    4000   // AY above this = FORWARD tilt
#define BACKWARD_AY_MAX  -2000   // AY below this = BACKWARD tilt
#define LEFT_AZ_MIN       4500   // AZ above this = LEFT tilt
#define RIGHT_AZ_MAX     -2500   // AZ below this = RIGHT tilt

// ─── Timing ──────────────────────────────────────────────────
#define HEAD_HOLD_MS        350   // head must stay in position this long
#define BLINK_DEBOUNCE_MS    60   // min blink duration ms
#define BLINK_COOLDOWN_MS   150   // min gap between two blinks
#define BOTH_GAP_MS          40   // L+R within 5~40ms = both eyes
#define DOUBLE_WIN_MS       900   // window for double blink
#define TRIPLE_WIN_MS      1500   // window for triple blink
#define WAIT_SEND_MS        500   // send 'W' every 500ms when halted
#define ACTIVE_HEARTBEAT_MS 300   // send 'A' every 300ms when active & idle
#define BTN_DEBOUNCE_MS      50   // button debounce

// ─────────────────────────────────────────────────────────────

enum HeadDir { DIR_CENTER, DIR_FORWARD, DIR_BACKWARD, DIR_LEFT, DIR_RIGHT };
const char* DN[] = {"CENTER","FORWARD","BACKWARD","LEFT","RIGHT"};

MPU6050 mpu;

// System state
bool sysActive = false;   // false = HALT, true = ACTIVE

// Head tracking
HeadDir curDir    = DIR_CENTER;
HeadDir stableDir = DIR_CENTER;
unsigned long dirSince = 0;

// Eye structs
struct Eye {
  int pin;
  bool blink, inBlink;
  unsigned long onStart, lastBlink;
};
Eye L = {PIN_LEFT_EYE,  false, false, 0, 0};
Eye R = {PIN_RIGHT_EYE, false, false, 0, 0};

// Blink counters
struct Ctr { int n; unsigned long t; bool on; };
Ctr LC={0,0,false}, RC={0,0,false}, BC={0,0,false};

// Both-eyes timestamp tracking
unsigned long LT=0, RT=0;

// Button state — debounced, single press only
bool btnPrev       = HIGH;
bool btnPressed    = false;
unsigned long btnLastMs = 0;

// Periodic 'W' sending (HALT) and 'A' sending (ACTIVE-idle)
unsigned long lastWaitSendMs   = 0;
unsigned long lastActiveSendMs = 0;

// Debug
unsigned long dbgT = 0;

// ─── Send ────────────────────────────────────────────────────
void send(char c) {
  uint8_t p = (uint8_t)c;
  esp_now_send(rxMAC, &p, 1);
  Serial.printf(">>> %c\n", c);
}
void onSent(uint8_t*, uint8_t) {}

// ─── Eye update ──────────────────────────────────────────────
void updateEye(Eye &e) {
  e.blink = false;
  bool raw = digitalRead(e.pin);
  if (raw && !e.inBlink) {
    if (millis() - e.lastBlink > BLINK_COOLDOWN_MS) {
      e.inBlink = true;
      e.onStart = millis();
    }
  } else if (!raw && e.inBlink) {
    if (millis() - e.onStart >= BLINK_DEBOUNCE_MS) {
      e.blink      = true;
      e.lastBlink  = millis();
    }
    e.inBlink = false;
  }
}

// ─── Both-eyes crosstalk filter ──────────────────────────────
bool checkBoth() {
  if (L.blink) LT = millis();
  if (R.blink) RT = millis();
  if ((L.blink || R.blink) && LT > 0 && RT > 0) {
    unsigned long gap = LT > RT ? LT - RT : RT - LT;
    if (gap >= 5 && gap <= BOTH_GAP_MS) {
      LT = 0; RT = 0;
      Serial.println("BOTH EYES");
      return true;
    }
  }
  return false;
}

// ─── Blink counter ───────────────────────────────────────────
bool count(Ctr &c, bool now, int target, unsigned long win) {
  if (now) {
    if (!c.on) { c.on = true; c.t = millis(); c.n = 1; }
    else {
      if (millis() - c.t <= win) c.n++;
      else { c.t = millis(); c.n = 1; }
    }
  }
  if (c.on && millis() - c.t > win) { c.on = false; c.n = 0; }
  if (c.n >= target) { c.n = 0; c.on = false; return true; }
  return false;
}

// ─── Head direction ──────────────────────────────────────────
HeadDir readHead() {
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  if (millis() - dbgT > 2000) {
    dbgT = millis();
    Serial.printf("AX=%d AY=%d AZ=%d | stable=%s | %s\n",
      ax, ay, az, DN[stableDir], sysActive ? "ACTIVE" : "HALT");
  }

  if (ay > FORWARD_AY_MIN)  return DIR_FORWARD;
  if (ay < BACKWARD_AY_MAX) return DIR_BACKWARD;
  if (az > LEFT_AZ_MIN)     return DIR_LEFT;
  if (az < RIGHT_AZ_MAX)    return DIR_RIGHT;
  return DIR_CENTER;
}

// ─── Button — debounced single press ─────────────────────────
bool readButtonPress() {
  bool raw = digitalRead(PIN_BUTTON);   // LOW when pressed
  bool pressed = false;

  if (raw == LOW && btnPrev == HIGH) {
    // Button just pressed down
    btnLastMs = millis();
  }
  if (raw == HIGH && btnPrev == LOW) {
    // Button just released
    if (millis() - btnLastMs >= BTN_DEBOUNCE_MS) {
      pressed = true;   // valid single press
    }
  }
  btnPrev = raw;
  return pressed;
}

// ─── Setup ───────────────────────────────────────────────────
void setup() {
  Serial.begin(115200); delay(200);
  Serial.println("\n=== Eye-Wheelchair TX FINAL (fixed) ===");
  Serial.println("System starts in HALT. Press button to ACTIVATE.");

  pinMode(PIN_LEFT_EYE,  INPUT);
  pinMode(PIN_RIGHT_EYE, INPUT);
  pinMode(PIN_BUTTON,    INPUT_PULLUP);

  Wire.begin(PIN_SDA, PIN_SCL);
  mpu.initialize();
  Serial.println(mpu.testConnection() ? "MPU6050 OK" : "MPU6050 FAILED");

  WiFi.mode(WIFI_STA); WiFi.disconnect();
  // Lock the WiFi channel explicitly so it always matches RX.
  // A channel mismatch (e.g. after either board previously joined a
  // router on a different channel) makes ESP-NOW packets silently
  // fail to arrive — a classic cause of intermittent connectivity.
  wifi_set_channel(1);

  if (esp_now_init() != 0) { Serial.println("ESP-NOW INIT FAILED"); return; }
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_send_cb(onSent);
  esp_now_add_peer(rxMAC, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);

  // Start in HALT — send first 'W' immediately
  sysActive = false;
  send('W');
  lastWaitSendMs = millis();

  Serial.println("Ready. RX MAC: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", rxMAC[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
}

// ─── Loop ────────────────────────────────────────────────────
void loop() {
  unsigned long now = millis();

  // ── Button: single press toggles HALT / ACTIVE ──────────
  if (readButtonPress()) {
    sysActive = !sysActive;
    if (sysActive) {
      Serial.println("=== SYSTEM ACTIVE ===");
      // Reset head tracking on activation
      stableDir = DIR_CENTER;
      curDir    = DIR_CENTER;
      dirSince  = now;
      lastActiveSendMs = now;   // send first heartbeat right away if idle
    } else {
      Serial.println("=== SYSTEM HALTED ===");
      // Send 'W' immediately (also stops motors on RX). Sending 'W'
      // directly — instead of 'S' then waiting up to 500ms for the
      // next 'W' — avoids a brief ambiguous moment on the RX LED.
      send('W');
      lastWaitSendMs = now;
    }
  }

  // ── HALT state: send 'W' periodically so RX LED fast-blinks ──
  if (!sysActive) {
    if (now - lastWaitSendMs >= WAIT_SEND_MS) {
      lastWaitSendMs = now;
      send('W');
    }
    delay(10);
    return;
  }

  // ════════════════════════════════════════════════════════
  //  ACTIVE STATE — process gestures
  // ════════════════════════════════════════════════════════

  // ── Active heartbeat: keeps RX from timing out the link and
  //    keeps RX's LED on SOLID instead of falling back to OFF. ──
  if (now - lastActiveSendMs >= ACTIVE_HEARTBEAT_MS) {
    lastActiveSendMs = now;
    send('A');
  }

  // ── Head direction with hold timer ──────────────────────
  HeadDir raw = readHead();
  if (raw != curDir) {
    curDir   = raw;
    dirSince = now;
    if (raw == DIR_CENTER && stableDir != DIR_CENTER) {
      stableDir = DIR_CENTER;
      Serial.println("HEAD CENTER → STOP");
      send('S');
    }
  }
  if (now - dirSince >= HEAD_HOLD_MS && stableDir != curDir) {
    stableDir = curDir;
    Serial.printf("HEAD STABLE: %s\n", DN[stableDir]);
  }

  // ── Eye blink detection ─────────────────────────────────
  updateEye(L);
  updateEye(R);
  bool both  = checkBoth();
  bool lOnly = L.blink && !both;
  bool rOnly = R.blink && !both;

  bool bothDbl  = count(BC, both,  2, DOUBLE_WIN_MS);
  bool bothTpl  = count(BC, both,  3, TRIPLE_WIN_MS);
  bool leftDbl  = count(LC, lOnly, 2, DOUBLE_WIN_MS);
  bool rightDbl = count(RC, rOnly, 2, DOUBLE_WIN_MS);

  // ── Gesture commands ────────────────────────────────────

  // Cruise ON: tilt forward + triple both-eye blink
  if (stableDir == DIR_FORWARD && bothTpl) {
    Serial.println("CRUISE ON");
    send('C');
    return;
  }

  // Cruise OFF: triple both-eye blink any position
  if (bothTpl) {
    Serial.println("CRUISE OFF");
    send('X');
    return;
  }

  // FORWARD/BACKWARD SWAPPED (chassis inverted):
  //   Head tilt FORWARD  + both double blink → sends 'B' (Backward)
  //   Head tilt BACKWARD + both double blink → sends 'F' (Forward)
  if (stableDir == DIR_FORWARD  && bothDbl) {
    Serial.println("HEAD FORWARD → send BACKWARD (B)");
    send('B');
    return;
  }
  if (stableDir == DIR_BACKWARD && bothDbl) {
    Serial.println("HEAD BACKWARD → send FORWARD (F)");
    send('F');
    return;
  }

  // LEFT: head tilt left + double LEFT eye blink
  if (stableDir == DIR_LEFT  && leftDbl) {
    Serial.println("LEFT");
    send('L');
    return;
  }

  // RIGHT: head tilt right + double RIGHT eye blink
  if (stableDir == DIR_RIGHT && rightDbl) {
    Serial.println("RIGHT");
    send('R');
    return;
  }

  delay(10);
}
