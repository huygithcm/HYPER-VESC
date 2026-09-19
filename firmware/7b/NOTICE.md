# Nguồn của port 7B

- `vesc/*.c`, các header `vesc/include`, `bms/*`: từ `huygithcm/esp32p4-android-auto`, nhánh `port/esp32s3-vesc-bms`, commit `664213605dffeed5d48e9653e6fe38dbcc28ae9c`. Giữ copyright/header và GPL-3.0 của nguồn, xem LICENSE tại root và clone.
- `board_7b.h`, `io_expander.*`, `gt911.*`: dựa trên `huygithcm/bambu-p1s-led`, commit `2ec4600e2a58b690e5c48c0b19ff9e2a75c6785a`, đường dẫn `esp32-display/firmware/src`. Bản nguồn tham khảo ở `docs/reference/7b`. GT911 được bổ sung giữ trạng thái giữa các frame và nhả khi lỗi I2C/timeout.
- `display_port.cpp`: adapter LVGL mới theo pin/timing và callback framebuffer của nguồn 7B nêu trên; không kéo renderer, mạng hoặc tính năng máy in.
- `backend.cpp`, `telemetry_decode.*`, `bms_transport.cpp`: integration native S3 mới. Transport BMS dùng NimBLE-Arduino 2.5.0; parser/model giữ từ seed. Không mang ESP-Hosted/C6/P4 vào build.
- UI ở `ui/7b`; font Antonio giữ tại `ui/7b/fonts` và LVGL giữ tại `Super_VESC_Display/lvgl`. Giữ nguyên các giấy phép đi kèm.
