# CAN P4 áp dụng cho S3 Waveshare 7B

Ngày đối chiếu: 2026-09-30. Đích: `platformio.ini`, environment `waveshare-7b`
trong HYPER-VESC; phần CYD dùng cấu hình riêng.

**Cập nhật sau đối chiếu:** người dùng đã yêu cầu build/flash. Đã nạp thành
công COM9 và kiểm tra boot; CAN đang bus-off, target thực tế là ID 11 đã lưu.
Xem [kết quả triển khai mới](DEPLOYMENT_7B.md). Các câu "chưa build/flash"
bên dưới mô tả phạm vi của bước kiểm tra source trước khi triển khai.

## Nguồn P4 và Lisp

Đã kiểm tra GitHub bằng `git ls-remote`: nhánh
[`fix/gear-circle-park-reverse`](https://github.com/huygithcm/esp32p4-android-auto/tree/fix/gear-circle-park-reverse)
ở commit `451e1392b3c04afc647618b95de3832fa3824d40`.
Checkout P4 đã ở đúng nhánh và commit này; không cần chuyển nhánh.

- [Kconfig CAN của P4](https://github.com/huygithcm/esp32p4-android-auto/blob/451e1392b3c04afc647618b95de3832fa3824d40/components/vesc_can/Kconfig).
- [Lisp đã công bố](https://github.com/huygithcm/esp32p4-android-auto/blob/451e1392b3c04afc647618b95de3832fa3824d40/lisp/main.lisp),
  được lưu nguyên byte tại [lisp/main.lisp](../lisp/main.lisp).
  SHA256: `bfa369b1acfb2c10aff6b0209001e96b1367cd1b79992e181443e517f6953a70`.
- Commit sửa Lisp gần nhất là `e44be163b5e2581fa8067c3b204b7ea9ced6c316`:
  đợi ramp phanh về ngưỡng trước khi kích hoạt lùi, nhả trạng thái phanh bằng
  `set-current 0`, không lặp off-delay khi dòng lùi bằng 0.
- Các chỉnh sửa chưa commit trong checkout P4 được giữ nguyên. Bản sao Lisp
  lấy từ commit đã công bố, không lấy từ working tree đang chỉnh sửa.

## Cấu hình đã áp dụng

Nguồn cấu hình dùng chung backend/UI: [can_config_7b.h](../include/can_config_7b.h).

| Tham số | S3 trước thay đổi | S3 hiện tại / P4 tham chiếu |
| --- | --- | --- |
| Bitrate | 1.000 kbit/s | 500 kbit/s |
| CAN ID màn | 254 | 2 |
| VESC ID mặc định | 10 | 10 |
| Khoảng nghỉ giữa vòng poll | 100 ms | 100 ms |
| Chờ phản hồi telemetry tối đa | 80 ms | 60 ms |
| Chờ trước lần poll đầu | Không có | 4.000 ms |
| CAN TX/RX | GPIO20/19 | Giữ GPIO20/19 của S3; P4 JC4880 dùng GPIO51/52 |

VESC ID đã lưu hợp lệ tiếp tục được dùng. ID đích cho phép 0..254, trừ ID màn
2; 255 là broadcast. ID đã lưu không hợp lệ dùng mặc định 10 khi khởi động.
Settings bỏ qua ID 2 khi bấm +/- và chỉ báo lưu thành công khi NVS ghi đủ dữ
liệu. Thay ID đích vẫn có hiệu lực sau khi khởi động lại như trước.
Nếu gắn đồng thời P4 và S3 lên cùng bus, phải chọn ID riêng cho từng màn.

Telemetry và ride mode vẫn dùng một task poll tuần tự. Request telemetry
giữ mask riêng phù hợp decoder/UI S3. Các file ride-mode transport, parser
và hai header giao thức format 2 đã được so sánh với commit P4 trên và khớp
về nội dung dòng; không cần chuyển giao thức. Chưa port toàn bộ transport
CAN mới của P4 hoặc các tính năng web/BLE/panel của P4.

Lisp chạy trên ESC và phải được nạp riêng bằng VESC Tool. Cập nhật source ở
đây không ghi vào ESC hoặc flash màn hình. Khớp format 2 không chứng minh
interlock, phanh hoặc lùi đã đạt trên phần cứng.

## Đối chiếu pinout 7B

Nguồn: [Waveshare Wiki](https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-7B#Pinouts),
[sơ đồ chính hãng](https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-7B/ESP32-S3-Touch-LCD-7B-Schematic.pdf),
và [demo Waveshare tại c652c902](https://github.com/waveshareteam/ESP32-S3-Touch-LCD-7B/tree/c652c902db607f7ffb376257393cfd7657aa6428).
Đã đọc bảng pinout trực tiếp trên Wiki và sơ đồ/demo trong checkout hãng.

| Nhóm | Cấu hình kiểm tra | Kết quả |
| --- | --- | --- |
| CAN | TX20, RX19; EXIO5=1 chọn CAN | Khớp Wiki và demo CAN |
| LCD điều khiển | VSYNC3, HSYNC46, DE5, PCLK7, DISP GPIO=-1 | Khớp demo RGB |
| LCD D0..D15 | 14,38,18,17,10,39,0,45,48,47,21,1,2,42,41,40 | Khớp cả 16 chân |
| I2C | SDA8, SCL9, 400 kHz | Khớp demo |
| Touch | GT911 INT4, reset EXIO1, 0x5D / fallback 0x14 | Khớp nguồn và biên bản bring-up |
| Expander | I2C 0x24, bit số IO dùng `1 << pin` | Khớp demo hãng |
| LCD reset / nguồn | EXIO3=LCD_RST; EXIO6=LCD_VDD_EN | Là hai tín hiệu riêng trên sơ đồ |
| Đèn nền / SD | EXIO2 backlight, EXIO4 SD_CS | Khớp sơ đồ; SD chưa dùng |
| Bộ nhớ | ESP32-S3 N16R8, flash 16 MB, PSRAM octal 8 MB, `qio_opi` | Khớp cấu hình và biên bản cũ |

Không có GPIO trùng nhau giữa các nhóm LCD, CAN, I2C và touch INT đang dùng.
EXIO là chân của expander, không phải GPIO ESP32 cùng số.
EXIO6 vốn đã lên cao nhờ shadow `0xFF`; nay được đặt tên và bật rõ trong
startup trước reset LCD. Không đổi pin reset EXIO3.

GPIO19/20 dùng chung USB native/CAN. `ARDUINO_USB_CDC_ON_BOOT=0` và source
không khởi động USB native; upload/log dùng CH343 qua UART. Giữ PCLK 16 MHz
đã dùng để sửa nháy trong [biên bản triển khai](DEPLOYMENT_7B.md), không đổi
theo clock demo hãng (clock là timing, không phải pinout).

Model 7B là phần cứng đã được ghi nhận trong [biên bản COM9](COM9_HARDWARE_CHECK.md).
Lần này đối chiếu source/sơ đồ, chưa đọc lại revision PCB, kết nối COM9,
build firmware, flash hay thử CAN/LCD trên thiết bị.

## Kiểm tra trong lần cập nhật này

- Host harness biên dịch `backend.cpp` thực tế với Arduino/Preferences/CAN
  mock: PASS cho cấu hình startup, thứ tự/timeout polling, toàn bộ 258 giá trị
  target từ -1 đến 256, ID đã lưu hợp lệ/không hợp lệ, chỉ áp target sau reboot,
  và tình huống NVS ghi thất bại.
- GCC kiểm tra cú pháp `preview_ui.c` ở cả chế độ simulator và hardware: PASS.
  Dùng cấu hình LVGL host; đây không phải build/link firmware ESP32-S3.
- So sánh 16 chân RGB với demo hãng, kiểm tra tổng 25 GPIO không trùng nhau,
  EXIO1..6, thứ tự chọn CAN/cấp nguồn LCD và USB CDC: PASS.
- SHA256 bản Lisp và đối chiếu bốn file ride-mode format 2 với P4: PASS.
- `python -B scripts/check_cyd_source.py`: PASS; `git diff --check`: PASS.

Mock không chứng minh TWAI/NVS hoặc motor thật hoạt động. Kiểm tra pinout
không thay thế xác nhận revision PCB và đo giao tiếp trên phần cứng.
