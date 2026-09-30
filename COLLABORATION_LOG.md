# Collaboration log

## 2026-09-30 — Codex — Authorized S3 7B build, COM9 flash and boot check

- User explicitly requested build and flash to the connected display. Live
  enumeration found COM9 CH343; esptool confirmed ESP32-S3 v0.2, 16 MB flash,
  embedded 8 MB PSRAM. No other device or ESC was flashed.
- Full build found the separate UI BuildSources environment lacked `include/`.
  Fixed `scripts/build_7b.py` CPPPATH, then build/link succeeded in 288.65 s.
  Static RAM 43,344 B; program flash 1,091,651 B; app image 1,102,624 B.
- Firmware SHA256: `a628533a2903d085c94d166c11891d8066c2e4506b4b1544f58e5188faae74fd`

- First upload hit a Windows cp1252 progress-output exception; stopped the
  process and repeated with PYTHONUTF8=1/PYTHONIOENCODING=utf-8. Second upload
  SUCCESS, all four written regions hash-verified, then RTS reset.
- Captured 35 s of boot UART. GT911 911 at 0x5d; LCD READY 1024x600; 16 MB
  flash/8 MB PSRAM; health at 30.318 s had 96 frames, zero frame timeouts,
  stable internal heap 126,672 B/PSRAM free 5,867,096 B. No panic or unplanned
  reboot observed; no touch events or visual acceptance obtained.
- CAN reports TX20/RX19 500k local=2 target=11 (existing NVS target retained).
  Repeated TWAI bus-off/recovery; physical CAN/VESC acceptance remains open.
  Need connected/powered peer, wiring/termination/bitrate check; target ID
  alone is not a cause of bus-off. ESC Lisp was not uploaded.
- Evidence: `.pio/verification/boot-7b-2026-09-30.log`, build outputs and
  `docs/DEPLOYMENT_7B.md`. Prior CYD and P4 collaborator edits preserved.

## 2026-09-30 — Codex — Align S3 7B CAN with published P4 and verify pinout

- User requested P4 `fix/gear-circle-park-reverse` CAN settings on the current
  S3 display, the published fixed Lisp, and a pinout check for the current board.
- Reference: GitHub branch and P4 HEAD both `451e139`; latest published Lisp
  fix is `e44be16`. P4 has unrelated uncommitted changes, including Lisp; it
  was read only. HYPER-VESC stays on `feature/cyd-2.8-3.5-4.3` with all prior
  CYD changes retained. No checkout, commit or push was needed/performed.
- Changes: shared `include/can_config_7b.h`, 500 kbit/s, display ID 2, default
  target 10, 100 ms loop delay, 60 ms telemetry reply wait, 4 s startup quiet
  window. Backend and settings UI share these values; reject self/broadcast
  targets and surface failed target persistence. Target changes still apply
  after reboot. Keep S3 TX20/RX19 and EXIO5 high.
- Added explicit EXIO6 LCD power enable; official schematic confirms EXIO3
  reset and EXIO6 power are separate. Existing shadow 0xFF already held EXIO6
  high. LCD/touch GPIOs, PCLK 16 MHz and memory profile retained.
- Copied published `lisp/main.lisp` directly from the commit, with a specific
  ignore exception; SHA256 recorded in `docs/CAN_P4_S3_ALIGNMENT.md`.
  Updated reference docs to distinguish the current source from old reviews.
- Checks: host harness using the real backend with mocked hardware passed
  startup/poll behavior, 258 target values, stored-ID migration and NVS failure;
  host C syntax passed for both UI modes. Compared 25 distinct GPIOs against
  official Waveshare demo/wiki/schematic; checked EXIO/startup/USB. Lisp hash
  and four ride-mode source/header comparisons passed. CYD static audit and
  diff whitespace passed. Temporary host harness is under ignored `.pio`.
- Status: source/config update complete. No firmware build/link, flash, ESC
  upload, physical CAN/LCD check or PCB-revision readback performed. Lisp runs
  on the ESC separately. P4 and S3 need distinct IDs if connected together.

## 2026-09-24 — Codex — CYD implementation specification

- Branch: `feature/cyd-2.8-3.5-4.3`, created from `master` at `a491dfc`.
- Scope: record the user-approved direction for a CAN telemetry-only CYD
  display for 2.8, 3.5 and 4.3 inch boards; no BMS or ride modes.
- Files: added `docs/CYD_IMPLEMENTATION_PLAN.md` and this log.
- Initial state: clean working tree; no pre-existing edits in this checkout.
- Checks: reviewed current 7B telemetry, CAN backend and display constraints;
  ran `git diff --cached --check` before commit. Firmware tests not run because
  this change contains documentation only.
- Status: specification recorded for implementation; no CYD firmware claimed.
- Handoff: confirm exact board models and CAN transceiver before pin mapping;
  preserve the existing 7B build. Commit locally only, as requested.
