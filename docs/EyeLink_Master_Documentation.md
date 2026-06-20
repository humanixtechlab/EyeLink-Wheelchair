# EyeLink — Eye & Head Controlled Smart Wheelchair

**A ₹2,500 (~$30) hands-free wheelchair control system for people with severe motor disabilities (ALS, spinal cord injury, cerebral palsy) — built from a pair of modified safety glasses and an ESP-NOW wireless link.**

> Project by **Humanix Tech Lab**
> YouTube: https://www.youtube.com/@HumanixTechLab
> GitHub: https://github.com/humanixtechlab
> Instagram: https://www.instagram.com/humanixtechlab/
> Facebook: https://www.facebook.com/humanixtechlab/
>
> If this project helps you, please subscribe / follow / star the repo — it genuinely helps support more builds like this one.

---

## One-paragraph pitch (use this for Reddit / LinkedIn intros)

Commercial eye-tracking wheelchair controllers cost ₹4,00,000–₹16,00,000 ($5,000–$20,000). **EyeLink** does the same core job — hands-free wheelchair control — for around ₹2,500 (~$30), using a WeMos D1 Mini, an MPU-6050, two IR blink sensors, and ESP-NOW wireless. You tilt your head in a direction and confirm with a specific eye-blink pattern; the wheelchair base receives the command instantly over a direct peer-to-peer wireless link — no Wi-Fi, no internet, no app. It's a working proof-of-concept that life-changing assistive tech doesn't have to be expensive.

---

## Table of Contents
1. [Introduction](#introduction)
2. [How It Works — System Overview](#how-it-works)
3. [Bill of Materials](#bill-of-materials)
4. [Wiring & Pin Maps](#wiring--pin-maps)
5. [Power System & Safety-Critical Battery Notes](#power-system)
6. [Building the TX Glasses](#building-tx)
7. [Building the RX Wheelchair Base](#building-rx)
8. [Firmware: Setup, Flashing, and Helper Tools](#firmware)
9. [Calibration](#calibration)
10. [Safety Features Explained](#safety-features)
11. [User Guide — Day to Day Operation](#user-guide)
12. [Known Issues, Fixes & Troubleshooting](#known-issues)
13. [Results, Limitations & Future Improvements](#results)
14. [Credits & Links](#credits)
15. [Appendix: Adapting This Doc Per Platform](#platform-appendix)

---

<a name="introduction"></a>
## 1. Introduction

EyeLink is a low-cost, wearable, hands-free control system for motorized wheelchairs. The user wears a pair of modified safety glasses fitted with two infrared eye-blink sensors, a head-tilt IMU (MPU-6050), and a small ESP8266 microcontroller. By combining a deliberate head tilt with a specific blink pattern, the user wirelessly drives a two-wheeled motorized wheelchair base — completely hands-free, using no Wi-Fi router or internet connection.

This build documents the **second hardware revision**: the receiver now has a finished seat (foam cushion + seat cover) instead of a bare test chassis, and both the transmitter and receiver firmware have been hardened against a comms-timeout bug found during testing (see [Known Issues](#known-issues)).

| | |
|---|---|
| **Total cost (final, finished build)** | ₹2,500 (~$30 USD) |
| **Total cost (semi-finished / bare prototype)** | ₹1,500 (~$18 USD) |
| **Commercial equivalent** | ₹4,00,000–₹16,00,000 (~$5,000–$20,000) |
| **Wireless protocol** | ESP-NOW (peer-to-peer, no router/internet needed) |
| **Control inputs** | Head tilt (4 directions) + eye blinks (single/double/triple) + 1 pushbutton |
| **Build skill level** | Intermediate — soldering, basic Arduino IDE use |

---

<a name="how-it-works"></a>
## 2. How It Works — System Overview

EyeLink has two halves that talk to each other wirelessly:

**TRANSMITTER (TX) — The Glasses**
- 2× IR eye-blink sensors (one per eye)
- MPU-6050 IMU for head-tilt detection
- WeMos D1 Mini (ESP8266)
- 480 mAh LiPo battery + TP4056 USB-C charger + 3.7V→5V boost module
- 1 pushbutton (pause / resume)

**RECEIVER (RX) — The Wheelchair Base**
- WeMos D1 Mini (ESP8266)
- L298N H-bridge motor driver
- 2× DC gear motors (100 RPM, 6mm shaft)
- HC-SR04 ultrasonic obstacle sensor
- 2× 18650 Li-ion cells (2S)
- Status LED (blue) + power LED (red)

**Command flow:**
1. User tilts their head in the desired direction.
2. They hold that position for ~350ms (filters out involuntary head movement).
3. They perform the matching blink pattern.
4. TX sends a single-character wireless command via ESP-NOW.
5. RX receives it and drives the motors (or stops, or toggles cruise mode) instantly.

While the system is active and idle (no gesture happening), TX also sends a lightweight heartbeat every 300ms so RX always knows the link is alive — this is what allows the status LED to show a clean ACTIVE/PAUSED/DISCONNECTED state instead of guessing.

---

<a name="bill-of-materials"></a>
## 3. Bill of Materials

### TRANSMITTER (TX) — Glasses

| Component | Qty | Notes |
|---|---|---|
| WeMos D1 Mini (ESP8266) | 1 | Main controller |
| MPU-6050 IMU module | 1 | Head-tilt detection (I2C) |
| IR eye-blink sensor module | 2 | One per eye |
| LiPo battery, 480mAh 3.7V | 1 | Keep it small/light — it sits on your face |
| TP4056 USB-C charging module | 1 | Charges the LiPo |
| 3.7V → 5V boost converter module | 1 | Steps LiPo voltage up to the 5V the D1 Mini wants |
| Slide/power switch | 1 | Main on/off |
| Momentary push button | 1 | Pause / resume (see [Safety Features](#safety-features)) |
| Safety glasses frame (transparent) | 1 | Mounting platform |
| Connecting wires (assorted) | — | |
| Hot glue gun + glue sticks | — | |

### RECEIVER (RX) — Wheelchair Base

| Component | Qty | Notes |
|---|---|---|
| WeMos D1 Mini (ESP8266) | 1 | Main controller |
| L298N H-Bridge motor driver | 1 | Drives both motors |
| DC gear motor, 100 RPM, 6mm shaft | 2 | Rear drive wheels |
| Rubber wheel, 10×2 cm | 2 | |
| Castor wheel | 1 | Front, free-swivel support |
| 18650 Li-ion battery, 2500mAh | 2 | **Wired 2S (series) — see battery safety note below** |
| DC barrel jack charging connector | 1 | |
| HC-SR04 ultrasonic sensor | 1 | Front-facing obstacle detection |
| 5mm LED, red | 1 | **Power-on indicator** — wired directly across the switched power rail, not driven by firmware |
| 5mm LED, blue | 1 | **Status indicator** — driven by the ESP8266 firmware (off / fast blink / solid, see [Safety Features](#safety-features)) |
| 100Ω resistor | 2 | One per LED |
| Rocker on/off switch | 1 | Main power |
| PVC foam board | 1 | Chassis |
| Foam/sponge cushion + seat cover fabric | 1 set | Seat upholstery for a finished, real-wheelchair look |
| Connecting wires (assorted) | — | |
| Hot glue gun + glue sticks | — | |

### Tools Required
Arduino IDE (free, arduino.cc) · Soldering iron + solder · Wire stripper · Small flathead screwdriver (IR sensor trimmer) · Multimeter · USB micro cable (for flashing the D1 Mini)

---

<a name="wiring--pin-maps"></a>
## 4. Wiring & Pin Maps

### TX Pin Connections

| Signal | D1 Mini Pin | GPIO |
|---|---|---|
| IR Left Eye sensor | D6 | GPIO12 |
| IR Right Eye sensor | D5 | GPIO14 |
| MPU-6050 SDA | D2 | GPIO4 |
| MPU-6050 SCL | D1 | GPIO5 |
| Push button (pause/resume) | D7 | GPIO13 (INPUT_PULLUP) |
| Power | LiPo → TP4056 → switch → boost (3.7→5V) → 5V pin | |

### RX Pin Connections

| Signal | D1 Mini Pin | GPIO |
|---|---|---|
| L298N ENA | D0 | GPIO16 |
| L298N IN1 | D5 | GPIO14 |
| L298N IN2 | D6 | GPIO12 |
| L298N IN3 | D7 | GPIO13 |
| L298N IN4 | D8 | GPIO15 |
| L298N ENB | D4 | GPIO2 |
| HC-SR04 TRIG | D1 | GPIO5 |
| HC-SR04 ECHO | D2 | GPIO4 |
| Blue status LED | D3 | GPIO0, via 100Ω resistor |
| Red power LED | — | Wired directly to switched battery rail via its own 100Ω resistor — **not connected to any GPIO** |
| Power | 2× 18650 (2S) → rocker switch → L298N 12V/VCC in | |
| Logic power | L298N 5V OUT → D1 Mini 5V pin | |
| Ground | All grounds tied common | |

> ⚠️ **GPIO0 note:** D3/GPIO0 is a boot-mode strapping pin on the ESP8266. It works fine as an LED output, but make sure nothing external holds it LOW at power-up, or the board can fail to boot normally.
>
> ⚠️ **HC-SR04 note:** the ECHO pin outputs a 5V signal, but ESP8266 GPIOs are 3.3V-only. A simple resistor voltage divider (e.g. 1kΩ + 2kΩ) on the ECHO line protects the pin and gives more reliable readings.

---

<a name="power-system"></a>
## 5. Power System & Safety-Critical Battery Notes

**TX (glasses):** LiPo 480mAh 3.7V → TP4056 (handles USB-C charging) → slide switch → 3.7V-to-5V boost module → D1 Mini 5V pin. The push button is signal-only (D7, INPUT_PULLUP) and doesn't carry power.

**RX (wheelchair):** 2× 18650 2500mAh cells wired in **2S (series)**, giving roughly 7.4V nominal / 8.4V full charge → rocker switch → L298N VCC/12V input. The L298N's onboard 5V regulator then feeds the D1 Mini's 5V pin, and all grounds are tied together.

> 🔴 **Important — read this before you wire the 2S pack:**
> These two 18650 cells are wired in series **without a BMS (battery management/protection circuit)**. That's a common shortcut in hobby builds, but it means:
> - **Charging must use a charger rated for 2S Li-ion (8.4V max, current-limited)** — never plug a generic 12V or 9V wall adapter into the DC barrel jack. Overvoltage on a BMS-less pack can overcharge a cell, which is a real fire risk.
> - **The two cells can drift out of balance** over many charge cycles since nothing is actively balancing them. Check both cell voltages with a multimeter every so often (they should stay within ~0.1V of each other) and stop using the pack if they diverge significantly.
> - **Don't let the pack deep-discharge.** Li-ion cells degrade and can become unsafe if run down too far. A basic 2S BMS module costs very little and is worth adding as a future improvement (see [Results & Future Improvements](#results)) — it adds over-charge, over-discharge, and short-circuit protection for a few extra rupees.
>
> This isn't meant to scare you off the design — it's meant to be said clearly once, in writing, since this device sits underneath a wheelchair seat.

---

<a name="building-tx"></a>
## 6. Building the TX Glasses

> 📷 Photo for each step below: see [`docs/IMAGE_SHOT_LIST.md`](IMAGE_SHOT_LIST.md#tx-build-tx_build) — drop files into `docs/images/tx_build/` using the filenames listed there and they'll show up if you add `![](images/tx_build/tx_0X_name.jpg)` under each step.

**Step 1 — Prepare the frame.** Take a transparent safety glasses frame. Hot-glue a small proto-board strip on each temple arm as a mounting platform for the sensors.

**Step 2 — Mount the IR blink sensors.** Attach one sensor per side, angled inward toward each eye at roughly 45°, positioned 2–3cm from the eye. Adjust the trimmer potentiometer until the OUT pin only goes HIGH when the eye is fully closed. Test in a dim room first — IR sensors can behave differently under bright light (see [Known Issues](#known-issues)).

**Step 3 — Mount the MPU-6050.** Hot-glue it flat on the top center of the glasses bridge. Wire it via I2C: SDA → D2, SCL → D1, VCC → 3.3V, GND → GND.

**Step 4 — Mount the D1 Mini.** Attach it to one temple arm, route the wiring harness along the frame, and secure with small hot-glue dabs every ~2cm.

**Step 5 — Add the power system.** Mount the TP4056 module and slide switch on the outer temple edge. Wire LiPo → TP4056 (B+/B-) → load output → slide switch → boost module → D1 Mini 5V. Add the pushbutton on D7 with `INPUT_PULLUP`.

**Step 6 — First power test (before final gluing).** Power on, open Serial Monitor at 115200 baud, tilt the glasses and confirm AY/AZ values change, and blink each eye to confirm sensor triggers register. Fix anything odd now — it's much harder once everything is sealed under hot glue.

---

<a name="building-rx"></a>
## 7. Building the RX Wheelchair Base

> 📷 Photo for each step below: see [`docs/IMAGE_SHOT_LIST.md`](IMAGE_SHOT_LIST.md#rx-build-rx_build) — drop files into `docs/images/rx_build/` using the filenames listed there.

**Step 1 — Build the chassis.** Cut a rectangular box from 5mm PVC foam board (recommended ~18×14cm base, 8cm walls) — enough room for all electronics inside.

**Step 2 — Mount the motors.** Fix the two 100 RPM gear motors at the rear corners with hot glue + zip ties, and press the rubber wheels firmly onto the shafts.

**Step 3 — Mount the castor wheel.** Press-fit or glue it at the center-front for stable 3-point support while still allowing free turning.

**Step 4 — Mount the HC-SR04.** Cut two small holes in the front face for the ultrasonic transducers, and glue the module behind them.

**Step 5 — Build the seat.** Cut a foam/sponge cushion to fit the top of the chassis and wrap it with the seat cover fabric, stapled or glued underneath. This is purely cosmetic/comfort but makes a huge difference in how the project reads as "a real wheelchair" rather than a bare test rig — worth doing before your final photos/video.

**Step 6 — Mount the electronics inside.** Suggested layout: 18650 pack at center (largest/heaviest item, keep it low and central for balance), L298N next to it, D1 Mini at the back wall, HC-SR04 at the front wall, both LEDs poking through small holes in the front face, rocker switch through a side wall.

**Step 7 — Wire everything.** Following the pin map above: battery(+) → rocker switch → L298N VCC; L298N 5V OUT → D1 Mini 5V; both motors → L298N outputs; all grounds tied together; red LED wired directly to the switched power rail (own resistor); blue LED to D3 through its resistor.

---

<a name="firmware"></a>
## 8. Firmware: Setup, Flashing, and Helper Tools

### Arduino IDE setup
1. Download Arduino IDE from arduino.cc.
2. File → Preferences → add this to **Additional Board Manager URLs**:
   `http://arduino.esp8266.com/stable/package_esp8266com_index.json`
3. Tools → Board → Board Manager → search "esp8266" → install **ESP8266 by ESP8266 Community**.
4. Select board: **LOLIN(WeMos) D1 R2 and mini**.

### Required libraries (Library Manager)
- MPU6050 (by Electronic Cats or jrowberg's i2cdevlib)
- Wire (built-in)
- ESP8266WiFi (built-in with the ESP8266 package)

### Files in this repo
| File | Flash onto | Purpose |
|---|---|---|
| `RX_MAC_Address_Finder.ino` | RX board (temporarily) | Prints the RX's MAC address in a ready-to-paste format |
| `MPU6050_Calibration_Helper.ino` | TX board (temporarily) | Walks you through head-tilt calibration and prints ready-to-paste threshold values |
| `TX_Glasses_FINAL.ino` | TX board (final) | Main glasses firmware |
| `RX_Wheelchair_FINAL.ino` | RX board (final) | Main wheelchair firmware |

### Flashing order (this matters!)

**Step 1 — Get the RX MAC address.** Flash `RX_MAC_Address_Finder.ino` to the RX board, open Serial Monitor at 115200 baud, and copy the printed `uint8_t rxMAC[] = {...};` line.

**Step 2 — Update TX with that MAC address.** Open `TX_Glasses_FINAL.ino`, find the `rxMAC[]` line near the top, and paste your value in. *(Skipping this is the single most common reason a freshly cloned build "does nothing" — TX runs fine but every packet it sends gets silently dropped, and the RX LED just stays off, looking exactly like "RX is off.")*

**Step 3 — Calibrate head-tilt thresholds.** Flash `MPU6050_Calibration_Helper.ino` to the TX board and follow the on-screen prompts (hold CENTER, then FORWARD, BACKWARD, LEFT, RIGHT — 3 seconds each). It prints a ready-to-paste `#define` block — copy it into `TX_Glasses_FINAL.ino`, replacing the existing threshold block. See [Calibration](#calibration) for more detail.

**Step 4 — Flash the real TX firmware.** Upload `TX_Glasses_FINAL.ino` (now containing your RX's MAC + your calibrated thresholds). Open Serial Monitor and confirm AY/AZ values respond to head movement.

**Step 5 — Flash the real RX firmware.** Upload `RX_Wheelchair_FINAL.ino`. Open Serial Monitor — it should print "Waiting for TX...".

**Step 6 — Power both boards and confirm the link.** Within a couple of seconds the blue status LED should start fast-blinking (paused/connected). Press the TX pushbutton once — the LED should switch to solid (active).

---

<a name="calibration"></a>
## 9. Calibration

### Eye-blink sensors
1. Power on TX, wear the glasses normally.
2. Open Serial Monitor at 115200 baud.
3. Keep both eyes open — no blink events should print.
4. Slowly close your **left** eye only — Serial Monitor should print a left-blink event.
5. If the right sensor also triggers (crosstalk), turn the left sensor's trimmer slightly clockwise.
6. Repeat for the right eye.

### Head-tilt thresholds
The easiest path is the `MPU6050_Calibration_Helper.ino` sketch described above — it records your personal CENTER baseline plus your FORWARD/BACKWARD/LEFT/RIGHT peaks and computes safe thresholds automatically (set at ~65% of the distance from your center to your peak, so a normal deliberate tilt reliably triggers without requiring a maximum-effort tilt every time).

If you'd rather do it by hand, the underlying logic is: with Serial Monitor open, note your AY value tilting forward/backward and AZ value tilting left/right, then set:
```cpp
#define FORWARD_AY_MIN    <your forward AY value>
#define BACKWARD_AY_MAX   <your backward AY value>
#define LEFT_AZ_MIN        <your left AZ value>
#define RIGHT_AZ_MAX       <your right AZ value>
```
in `TX_Glasses_FINAL.ino`. Every MPU-6050 mounting angle is slightly different — there is no universal "correct" number here.

---

<a name="safety-features"></a>
## 10. Safety Features Explained

**Layer 1 — Two-factor command confirmation.** Every movement requires a head tilt held for 350ms *and* the correct blink pattern, at the same time. This prevents an accidental movement from a stray blink or casual head turn.

**Layer 2 — Ultrasonic obstacle detection.** The HC-SR04 checks for obstacles every 100ms. If anything is within 20cm, forward motion (including cruise) is blocked until the path clears. Left/right/backward movement are not blocked by this check.

**Layer 3 — Communications watchdog.** If RX receives *no packet at all* (movement command or heartbeat) for 3 seconds, it assumes the link is lost and stops the motors automatically. This now works correctly during normal active idling too, because TX sends a lightweight heartbeat every 300ms whenever it's active and not currently issuing a movement command — see [Known Issues](#known-issues) for why this matters.

**Layer 4 — Pushbutton pause/resume.** The TX pushbutton is a **single-press toggle**: one press pauses the system (motors stop, all gestures ignored), another press resumes it. There is no separate long-press/short-press distinction in the current firmware — every press just flips between PAUSED and ACTIVE.

**Turn safety.** Turns run at ~25% speed and auto-stop after 300ms, preventing a single gesture from spinning the chair in a full circle. Repeat the gesture for more turning.

**Status LED guide (current firmware):**

| LED state | Meaning |
|---|---|
| **OFF** | TX is off, out of range, or not paired (MAC mismatch) |
| **FAST BLINK** | TX connected, but PAUSED |
| **SOLID ON** | TX connected and ACTIVE (idle or actually moving) |

---

<a name="user-guide"></a>
## 11. User Guide — Day to Day Operation

**First-time setup each day:**
1. Charge TX via USB-C, charge RX via the DC barrel jack (using a charger rated for 2S Li-ion — see [Power System](#power-system)).
2. Switch on the glasses, then switch on the wheelchair base.
3. Wait a couple of seconds for the ESP-NOW link — the blue LED will fast-blink (paused, connected).
4. Press the glasses pushbutton once to go ACTIVE — LED turns solid.

**Driving commands:**

| Action | How |
|---|---|
| Forward | Tilt head forward, hold 350ms, double-blink both eyes |
| Backward | Tilt head backward, hold 350ms, double-blink both eyes |
| Turn left | Tilt head left, hold 350ms, double-blink left eye (repeat to turn further) |
| Turn right | Tilt head right, hold 350ms, double-blink right eye (repeat to turn further) |
| Stop | Return head to center — automatic |
| Cruise ON | Tilt head forward, triple-blink both eyes — drives forward continuously at full speed |
| Cruise OFF | Triple-blink both eyes again (any head position) |
| Pause everything | Single press of the glasses pushbutton |
| Resume | Single press of the glasses pushbutton again |

---

<a name="known-issues"></a>
## 12. Known Issues, Fixes & Troubleshooting

These are real problems found and fixed during development — worth keeping in the writeup, since anyone cloning this project will hit the same class of issues if they skip a step.

- **Intermittent "it just stops working" during active use.** Root cause: the original firmware only sent wireless packets in PAUSED mode (heartbeat) or when a gesture matched. If you stayed active without gesturing for more than the comms timeout, RX assumed TX had disconnected and force-stopped everything. **Fixed** by adding a 300ms active-idle heartbeat ('A' command) so RX always knows TX is alive, whether or not anything is currently moving.
- **Status LED looked identical whether PAUSED or ACTIVE-but-idle.** Same root cause as above — fixed alongside it, since the LED now reads directly off TX's reported active/paused state instead of guessing from motor activity.
- **RX LED stuck off even though both boards are powered.** Almost always a `rxMAC[]` mismatch — use `RX_MAC_Address_Finder.ino` to confirm.
- **Intermittent connectivity for no obvious reason.** Possible WiFi channel mismatch between TX and RX (ESP-NOW rides on a WiFi channel, and either board can drift to a different one if it ever previously joined a router). Lock the channel explicitly with `wifi_set_channel(1)` on both sides.
- **RX resets or disconnects specifically when the motors start moving.** Suspect voltage sag — the L298N's onboard 5V linear regulator can dip under motor load, brown-out resetting the ESP8266. If this happens, power the D1 Mini from a separate small 5V buck converter instead of relying on the L298N's onboard regulator.
- **Ultrasonic readings flaky over time.** The HC-SR04's ECHO output is 5V logic into a 3.3V-only GPIO — add a resistor divider on that line.
- **IR sensors falsely triggering outdoors.** They're sensitive to strong ambient IR (sunlight). Best performance is indoors / shaded; outdoor shielding is a planned improvement.

---

<a name="results"></a>
## 13. Results, Limitations & Future Improvements

> 📷 Add your best 2-3 final photos/clips here — completed glasses worn, completed wheelchair, and ideally a short clip of the chair moving. See [`docs/IMAGE_SHOT_LIST.md`](IMAGE_SHOT_LIST.md#final-results-final) for filenames.

**What works:**
- Wireless ESP-NOW link between glasses and wheelchair *(note your own tested range here once confirmed — e.g. "stable up to X meters")*
- Eye-blink detection with crosstalk filtering — reliably distinguishes left, right, and both-eyes-together
- Head-tilt detection across all 4 directions
- Obstacle detection halts forward motion within 20cm
- Pause/resume button works instantly
- Cruise mode for straight-line driving
- Fully rechargeable on both units
- Status LED now gives an unambiguous read of system state (see fix above)

**Current limitations (semi-finished prototype):**
- Glasses wiring is still exposed — a clean enclosure/3D-printed housing is the next step
- Wheelchair chassis is PVC foam board — fine for proof-of-concept, needs a stronger material for real-world daily use
- Not yet tested on an actual wheelchair frame — currently a standalone drive-base prototype
- IR sensors are sensitive to strong sunlight — needs shielding for reliable outdoor use
- RX battery pack has no BMS — see the battery safety note above

**Challenges overcome:**
- IR crosstalk between the two eye sensors — solved with a 5–40ms timing-gap filter in firmware
- Accidental triggers from involuntary blinks — solved with the head-position-hold + blink-confirmation combo
- Uncontrolled spinning during turns — solved with a 300ms auto-stop at reduced turn speed
- False "disconnected" state during normal active use — solved with the active-idle heartbeat (see Known Issues)

**Future improvements:**
- 3D-printed glasses housing for a clean wearable design
- 2S BMS module on the RX battery pack
- Separate 5V regulator for the RX's ESP8266, isolated from motor-driver power sag
- OLED display on the glasses for status feedback
- Universal mounting bracket to retrofit a real wheelchair frame
- Buzzer on the wheelchair for audible obstacle warnings
- Mobile app for calibration and settings
- Outdoor-rated IR sensor shielding

---

<a name="credits"></a>
## 14. Credits & Links

Built and documented by **Humanix Tech Lab**.

- YouTube: https://www.youtube.com/@HumanixTechLab
- GitHub: https://github.com/humanixtechlab
- Instagram: https://www.instagram.com/humanixtechlab/
- Facebook: https://www.facebook.com/humanixtechlab/

If this project helped you, or you build your own version, a subscribe/follow/star — or just a comment showing your build — genuinely helps support more projects like this.

---

<a name="platform-appendix"></a>
## 15. Appendix: Adapting This Master Doc Per Platform

You don't need a different write-up for every site — trim/reformat this one document differently for each:

| Platform | How to adapt it |
|---|---|
| **Instructables** | Use almost everything as-is. Break each numbered section into its own "Step" block with 1–3 photos per step (you already have good internal-wiring shots for the RX steps and a glasses shot for the TX steps). Attach the 4 `.ino` files as downloads in the firmware step, exactly like before. |
| **GitHub README.md** | Trim to: pitch paragraph → photo/demo gif → BOM table → pin map → "Quick Start" (the 6-step flashing order) → link to a `/docs` folder containing the rest of this doc in full. Put `LICENSE`, the 4 `.ino` files, and a wiring diagram image in the repo root. Keep code-facing content first; humans reading a README want to clone-and-run before they want backstory. |
| **Hackster.io** | Very similar structure to Instructables ("Story" format), but Hackster auto-builds a parts list widget from tags — fill that in separately from the BOM table. Keep the narrative slightly more concise than Instructables; lead with the cost-comparison hook. |
| **Hackaday.io** | Frame it as a project **log series** instead of one page: Log 1 – concept & cost pitch, Log 2 – TX build, Log 3 – RX build, Log 4 – firmware/calibration, Log 5 – safety + the heartbeat bug story (Hackaday's audience loves an honest "here's the bug I found and how I fixed it" log). Schematics and pin tables are very well received there — keep them. |
| **Reddit** (r/arduino, r/esp8266, r/electronics, accessibility-tech subs) | Don't post the whole doc — write a short, genuine 4–6 sentence summary (the pitch paragraph works well), attach 3–5 of your best photos, and link to the full GitHub/Instructables for details. Reddit reacts badly to anything that reads like a press release — keep the tone "here's what I built, happy to answer questions," not "please subscribe." Save the channel-credit block for your own site/repo, not the Reddit post body. |
| **LinkedIn** | Lead with the *impact* angle, not the parts list: who this could help, the cost-vs-commercial contrast, a short personal note on why you built it. 3–5 short paragraphs max, one strong photo or short video, link to the full writeup for anyone who wants depth. Save the technical detail for the linked doc. |

A reasonable workflow: keep this Markdown file as the canonical source in your GitHub repo, and copy/trim sections from it into each platform's native editor rather than rewriting from scratch each time.
