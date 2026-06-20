# Image shot list — what to add, where, and what to name it

This is your checklist. Add images whenever you have them — nothing here blocks you from publishing the repo today with zero images and filling this in over the next few days/weeks. The README and master doc already reference these exact paths, so once a file exists at the right path with the right name, it just appears — no other edits needed.

**Folder structure to create inside `docs/images/`:**
```
docs/images/
├── 01_hero_banner.jpg
├── 02_demo.gif
├── 03_system_diagram.svg        <- already created for you, included in the repo
├── 04_parts_flatlay.jpg         (optional)
├── 05_wiring_diagram.png
├── tx_build/
│   ├── tx_01_frame.jpg
│   ├── tx_02_ir_sensors.jpg
│   ├── tx_03_mpu_mount.jpg
│   ├── tx_04_d1mini_mount.jpg
│   ├── tx_05_power_wiring.jpg
│   └── tx_06_final.jpg
├── rx_build/
│   ├── rx_01_chassis.jpg
│   ├── rx_02_motors.jpg
│   ├── rx_03_castor.jpg
│   ├── rx_04_ultrasonic.jpg
│   ├── rx_05_seat.jpg
│   ├── rx_06_electronics_layout.jpg
│   └── rx_07_final.jpg
└── final/
    ├── final_01_glasses_worn.jpg
    ├── final_02_wheelchair_complete.jpg
    └── final_03_demo_clip.gif
```

---

## Priority order (do these first if you're short on time)

1. **`01_hero_banner.jpg`** — single best photo, top of README. Highest impact for first impressions.
2. **`02_demo.gif`** — short looping clip of one command cycle. Highest impact for engagement — people trust working demos far more than photos.
3. **`05_wiring_diagram.png`** — even a clear phone photo of a hand-drawn diagram is fine for v1.
4. Everything else can follow gradually.

---

## Hero & top-level (`docs/images/`)

#### `01_hero_banner.jpg`
- **What:** Your single best, most polished photo. Glasses + wheelchair together if possible, good lighting, tidy background.
- **Used in:** README, top of page.
- **Size:** 1280px wide minimum, landscape orientation.

#### `02_demo.gif`
- **What:** 5–10 second loop of one full cycle: head tilt → blink → wheelchair moves. Cut from your edited YouTube video.
- **How to make it:** Export a short clip from Filmora as MP4, then convert at [ezgif.com/video-to-gif](https://ezgif.com/video-to-gif) (free, no install) — keep under ~8MB so it loads fast on GitHub.
- **Used in:** README, right under the hero banner.

#### `03_system_diagram.svg`
- **Status: already created and included** — a clean block diagram of TX → ESP-NOW → RX with all major components labeled. Open it in any browser to check it, or replace it with your own later if you prefer a hand-drawn version.

#### `04_parts_flatlay.jpg` (optional)
- **What:** All TX components laid flat on a table before assembly. A second photo for RX components. Top-down, good lighting.
- **Why it helps:** Builders can visually match their own parts against yours before ordering.

#### `05_wiring_diagram.png`
- **What:** A wiring/schematic diagram — one for TX, one for RX (or combined). Use Fritzing, EasyEDA, or KiCad if you know them; otherwise a clean hand-drawn diagram on paper, photographed well, is genuinely fine for a first release.
- **Used in:** README and master doc, Wiring section.

---

## TX build steps (`docs/images/tx_build/`)

One photo per step in [`EyeLink_Master_Documentation.md`](EyeLink_Master_Documentation.md#building-tx):

| Filename | What to shoot |
|---|---|
| `tx_01_frame.jpg` | The bare safety-glasses frame with proto-board strips glued on, before sensors |
| `tx_02_ir_sensors.jpg` | Both IR blink sensors mounted, angled toward the eyes |
| `tx_03_mpu_mount.jpg` | MPU-6050 glued to the bridge of the glasses, wires visible |
| `tx_04_d1mini_mount.jpg` | D1 Mini mounted on the temple arm with wiring routed |
| `tx_05_power_wiring.jpg` | TP4056, switch, and boost converter wired up (before final glue) |
| `tx_06_final.jpg` | Finished glasses, worn or on a stand, all wiring sealed |

Add each one in the doc right after its matching step text, e.g.:
```markdown
**Step 2 — Mount the IR blink sensors.** ...
![IR sensors mounted](images/tx_build/tx_02_ir_sensors.jpg)
```

---

## RX build steps (`docs/images/rx_build/`)

One photo per step in [`EyeLink_Master_Documentation.md`](EyeLink_Master_Documentation.md#building-rx):

| Filename | What to shoot |
|---|---|
| `rx_01_chassis.jpg` | The cut PVC foam board chassis, empty |
| `rx_02_motors.jpg` | Both gear motors mounted at the rear with wheels on |
| `rx_03_castor.jpg` | Castor wheel mounted at front-center |
| `rx_04_ultrasonic.jpg` | HC-SR04 mounted behind the front face holes |
| `rx_05_seat.jpg` | Foam cushion + seat cover being fitted |
| `rx_06_electronics_layout.jpg` | Inside view — battery, L298N, D1 Mini, wiring all visible before closing up |
| `rx_07_final.jpg` | Fully painted, seated, finished wheelchair base |

Same pattern — add under each step in the master doc.

---

## Firmware / Arduino IDE screenshots (`docs/images/firmware_screenshots/`)

These are screenshots of YOUR Arduino IDE while flashing — not photos. On Windows, use **Windows Key + Shift + S** to snip just the relevant window, or the Snipping Tool. These go inline in the "Flashing order" steps in [`EyeLink_Master_Documentation.md`](EyeLink_Master_Documentation.md#firmware) so a first-time builder can compare their screen to yours.

⚠️ Before saving these, double-check your MAC address and any personal info isn't something you mind being public — it's just a device identifier, not sensitive, but worth a glance.

| Filename | What to screenshot |
|---|---|
| `fw_01_mac_output.png` | Serial Monitor window after flashing `RX_MAC_Address_Finder.ino` — showing the printed `uint8_t rxMAC[] = {...};` line |
| `fw_02_rxmac_in_code.png` | The Arduino IDE code editor, scrolled to the `rxMAC[]` line near the top of `TX_Glasses_FINAL.ino`, with your pasted-in value visible |
| `fw_03_calibration_output.png` | Serial Monitor after running `MPU6050_Calibration_Helper.ino` — showing the final printed `#define` block |
| `fw_04_waiting_for_tx.png` | Serial Monitor right after flashing `RX_Wheelchair_FINAL.ino` — showing the "Waiting for TX..." message |
| `fw_05_status_led.jpg` | A real photo (not a screenshot) of the blue status LED solid/lit on the wheelchair base once connected |

**How to take a clean Serial Monitor screenshot:**
1. Open Tools → Serial Monitor in Arduino IDE (make sure baud rate is set to 115200 in the dropdown, bottom-right of that window)
2. Let a few lines of relevant output print
3. Windows Key + Shift + S → drag a box around just the Serial Monitor window → it copies to clipboard
4. Paste into Paint or directly into a folder (Ctrl+V in a file explorer window on Windows 11 saves it as a file)
5. Rename to match the filename above and place in `docs/images/firmware_screenshots/`

---

## Final results (`docs/images/final/`)

| Filename | What to shoot |
|---|---|
| `final_01_glasses_worn.jpg` | Glasses being worn (you or a model), natural setting |
| `final_02_wheelchair_complete.jpg` | The complete, painted, seated wheelchair from a flattering angle |
| `final_03_demo_clip.gif` | Optional second demo clip — different angle or different command than `02_demo.gif` |

Used in the Results section of the master doc and optionally repeated near the bottom of the README.

---

## General photo tips for this kind of repo

- **Natural or soft white light** beats flash — flash on bare PCBs creates harsh glare on solder joints.
- **Fill the frame** — get close enough that the component you're documenting is the obvious subject, not lost in a cluttered desk.
- **Consistent background** across step photos (same table/mat) makes the build sequence feel like a coherent series rather than random snapshots.
- **Shoot a couple extra angles per step** even if you only use one — you'll thank yourself when Instructables/Hackster want a different crop than GitHub.
- File size: keep individual photos under ~2MB (resize to ~1600px wide if your phone shoots larger) so the repo stays fast to clone.
