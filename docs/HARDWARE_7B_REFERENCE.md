# Phần cứng đích: Waveshare ESP32-S3-Touch-LCD-7B

**Cập nhật 2026-09-18:** đã đọc board thật tại COM9: flash **16 MB**, embedded PSRAM **8 MB**, ESP32-S3 revision v0.2. Xem [biên bản kiểm tra COM9](COM9_HARDWARE_CHECK.md). Tài liệu hãng hiện xác nhận CAN TX GPIO20 / RX GPIO19; thông tin này bổ sung cho các điểm chưa xác minh khi đọc repo bên dưới.

Người dùng xác nhận đang sử dụng màn hình 7 inch trong nhánh [bambu-p1s-led / feat/display-ui](https://github.com/huygithcm/bambu-p1s-led/tree/feat/display-ui). Đây là phần cứng đích cho công việc tiếp theo của Super VESC Display.

Đã đọc ngày 2026-09-17, commit `2ec4600e2a58b690e5c48c0b19ff9e2a75c6785a`. Firmware của dự án hiện tại vẫn cấu hình màn hình 4 inch; tài liệu này ghi nhận phần cứng đích, chưa thực hiện port hoặc kiểm tra trên thiết bị.

## Nguồn tham khảo cố định

- [board_7b.h](https://github.com/huygithcm/bambu-p1s-led/blob/2ec4600e2a58b690e5c48c0b19ff9e2a75c6785a/esp32-display/firmware/src/board_7b.h): độ phân giải, timing, pinout, IO expander.
- [platformio.ini](https://github.com/huygithcm/bambu-p1s-led/blob/2ec4600e2a58b690e5c48c0b19ff9e2a75c6785a/esp32-display/firmware/platformio.ini): toolchain và bộ nhớ.
- [display.cpp](https://github.com/huygithcm/bambu-p1s-led/blob/2ec4600e2a58b690e5c48c0b19ff9e2a75c6785a/esp32-display/firmware/src/display.cpp): `esp_lcd` RGB, framebuffer, bounce buffer, đồng bộ đổi frame.
- [io_expander.cpp](https://github.com/huygithcm/bambu-p1s-led/blob/2ec4600e2a58b690e5c48c0b19ff9e2a75c6785a/esp32-display/firmware/src/io_expander.cpp): điều khiển reset, backlight và shadow output.
- [gt911.cpp](https://github.com/huygithcm/bambu-p1s-led/blob/2ec4600e2a58b690e5c48c0b19ff9e2a75c6785a/esp32-display/firmware/src/gt911.cpp): reset và đọc cảm ứng.
- [README phần màn hình](https://github.com/huygithcm/bambu-p1s-led/blob/2ec4600e2a58b690e5c48c0b19ff9e2a75c6785a/esp32-display/README.md): sơ đồ tài nguyên, USB/CAN dùng chung GPIO 19/20.

## Cấu hình trong source đã đọc

| Thành phần | Cấu hình |
| --- | --- |
| Board | Waveshare ESP32-S3-Touch-LCD-7B |
| Bộ nhớ | Flash 16 MB, PSRAM octal 8 MB (N16R8), `qio_opi` |
| LCD | 1024 × 600, RGB565, bus 16 bit |
| PCLK | 30 MHz, `pclk_active_neg = 1`; refresh tính theo timing khoảng 32.7 Hz |
| Horizontal pulse / back / front | 162 / 152 / 48 |
| Vertical pulse / back / front | 45 / 13 / 3 |
| Framebuffer | 2 buffer trong PSRAM, mỗi buffer 1,228,800 byte |
| Bounce buffer | `1024 * 20` pixel; hai buffer RGB565 tương ứng tổng 81,920 byte SRAM |
| I2C | SDA GPIO 8, SCL GPIO 9, 400 kHz |
| Touch | GT911, INT GPIO 4, địa chỉ 0x5D; fallback 0x14 |
| IO expander | Địa chỉ 0x24; các register mode/output/input/PWM/ADC là 0x02/0x03/0x04/0x05/0x06 |

LCD: VSYNC = GPIO 3, HSYNC = GPIO 46, DE = GPIO 5, PCLK = GPIO 7, DISP = -1.

Thứ tự D0…D15 (B0…B4, G0…G5, R0…R4):

```text
14, 38, 18, 17, 10, 39, 0, 45, 48, 47, 21, 1, 2, 42, 41, 40
```

IO expander: IO1 reset touch, IO2 backlight, IO3 reset LCD, IO4 SD CS, IO5 chọn USB/CAN (0 = USB, 1 = CAN theo header). Driver backlight giới hạn 97%, chuyển phần trăm sang PWM 8 bit. Không dùng trực tiếp GPIO 2 để điều chỉnh backlight như cấu hình màn 4 inch.

Touch được giữ reset qua IO1, kéo INT thấp rồi nhả reset, sau đó trả INT về input và thử đọc product ID tại 0x5D/0x14.

## Toolchain và cách vẽ

Nhánh tham khảo dùng môi trường `waveshare-7b`, board `esp32-s3-devkitc1-n16r8`, Arduino với pioarduino release `55.03.311` (comment trong cấu hình ghi Arduino 3.3.11 / IDF 5.5.5). Driver dùng `esp_lcd_new_rgb_panel`, `num_fbs`, `bounce_buffer_size_px`, framebuffer trong PSRAM và callback `on_frame_buf_complete` trước khi tái sử dụng buffer.

Phần UI tham khảo vẽ bằng các hàm `gfx*` trực tiếp lên framebuffer. Khi dùng với LVGL của Super VESC Display, cần viết lớp flush và cơ chế sở hữu buffer phù hợp; không thể thay nguyên UI hiện tại bằng driver này rồi coi như hoàn tất.

## Những điểm cần đổi khi port Super VESC Display

1. Thêm cấu hình build cho 7B và kiểm tra khả năng tương thích thư viện khi nâng Arduino core.
2. Thay pinout/timing LCD, lớp backlight/reset qua IO expander và I2C/touch theo nguồn trên.
3. Chuyển layout LVGL từ 480 × 480 sang 1024 × 600; tính lại draw buffer và đồng bộ flush.
4. Xác minh TX/RX CAN theo schematic của board 7B trước khi cấu hình TWAI. README tham khảo ghi USB/CAN dùng GPIO 19/20 nhưng phần driver đã đọc chưa triển khai TWAI để xác nhận chiều TX/RX. Cấu hình hiện tại trong `src/comm_can.cpp` là TX 6 / RX 0, trong khi GPIO 0 thuộc bus LCD 7B nên không thể giữ nguyên.
5. Xử lý IO5 chọn CAN và đối chiếu chế độ USB trong build hiện tại (`ARDUINO_USB_CDC_ON_BOOT`, `ARDUINO_USB_MODE`) với tài nguyên USB/CAN dùng chung.

Phần giao thức Lisp settings tiếp tục tham khảo [LISP_SETTINGS_REFERENCE.md](LISP_SETTINGS_REFERENCE.md). Repo ESP32-P4 là nguồn tham khảo logic Lisp/CAN/UI; board đích của người dùng là ESP32-S3 7B được ghi nhận ở đây.
