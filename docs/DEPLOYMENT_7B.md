# Triển khai firmware 7B

## Build và flash ngày 2026-09-30

Đã build và nạp environment `waveshare-7b` vào COM9 (CH343 VID:PID 1A86:55D3).
Esptool nhận ESP32-S3 revision v0.2, flash 16 MB, PSRAM 8 MB và xác minh hash
cả bootloader, partition table, boot_app0, application; upload kết thúc SUCCESS.

- Đã sửa `scripts/build_7b.py` để thêm `include/` vào CPPPATH của các nguồn UI
  biên dịch riêng. Build/link thực tế thành công sau sửa.
- RAM tĩnh: 43.344 / 327.680 byte; flash chương trình: 1.091.651 / 6.553.600 byte.
- `.pio/build/waveshare-7b/firmware.bin`: 1.102.624 byte,
  SHA256 `a628533a2903d085c94d166c11891d8066c2e4506b4b1544f58e5188faae74fd`.
- Lần upload đầu gặp lỗi hiển thị tiến độ `UnicodeEncodeError` (cp1252);
  đã dừng và nạp lại đầy đủ với `PYTHONUTF8=1`, `PYTHONIOENCODING=utf-8`.

Log sau reset: `FLASH=16777216 PSRAM=8388608`, GT911 `911` ở `0x5d`,
`READY 1024x600 RGB565 double framebuffer; hardware UI`.
CAN TX20/RX19, **500 kbit/s, local ID 2, target ID 11**. ID 11 là giá trị
đã lưu trên màn; mặc định source là 10 và không ghi đè setting hợp lệ.

Đã đọc UART trong 35 giây, log lưu tại
`.pio/verification/boot-7b-2026-09-30.log`. Health tới uptime 30.318 ms:
96 frame, `timeouts=0`, heap nội 126.672 byte, PSRAM trống 5.867.096 byte;
không thấy panic/reboot sau lần reset chủ động. Touch controller được nhận,
nhưng không có lần chạm trong mẫu (`touch=0`), chưa xác nhận hình ảnh bằng mắt.

**CAN chưa đạt:** log liên tục `TWAI bus-off` và phục hồi. Chưa có bằng chứng
giao tiếp VESC thành công; cần kiểm tra ESC có nối/cấp nguồn, CAN H/L/GND,
termination và bitrate. Sai target ID đơn thuần không giải thích bus-off.
Không nạp Lisp vào ESC trong bước flash màn này.

[Cấu hình CAN và pinout](CAN_P4_S3_ALIGNMENT.md). Các kết quả dưới đây thuộc
lần triển khai trước ngày 2026-09-30.

Ngày 2026-09-18. Đã build và nạp thành công trên COM9, esptool xác minh hash flash. Người dùng xác nhận giao diện hiển thị đủ và cảm ứng bấm đúng vị trí. Đã nạp bản sửa 16 MHz; người dùng xác nhận hết nháy đường dọc trên màn thật.

## Cấu hình

Waveshare ESP32-S3-Touch-LCD-7B N16R8, RGB565 1024 × 600, PCLK 16 MHz (bản đầu 30 MHz). Hai framebuffer PSRAM (2.34 MiB), bounce 20 dòng; bản đầu dùng full refresh, bản tối ưu dùng direct mode và cơ chế đồng bộ vùng thay đổi đã có trong LVGL vendored. Chỉ đổi framebuffer ở lần flush cuối và trả buffer sau callback frame complete. Heap LVGL cấp trong PSRAM. Touch GT911 SDA8/SCL9/INT4 và IO expander 0x24. CAN TX20/RX19, EXIO5 high; UART CH343 trên COM9 dùng để upload/log.

## Build trên máy này

```powershell
$env:PLATFORMIO_CORE_DIR = 'C:\Super_VESC_DIsplay\.pio\core'
python -m platformio run -e waveshare-7b -j 2
```

Môi trường `.pio/core/penv` riêng xử lý xung đột Python 3.11/3.12. Packages/platforms dùng junction đến cache có sẵn, esptool lấy từ package tool-esptoolpy bằng đường dẫn ưu tiên trong penv. Không cần áp dụng workaround này nếu PlatformIO máy khác hoạt động bình thường.

## Kiểm tra đã có

- Sao lưu đủ flash 16,777,216 byte vào `.pio/backups/com9-before-7b.bin`; log `.pio/backups/read-flash.log`.
- SHA256 bản sao: `DFFAB0DD410657CB30C7B2FD7F2586A4792E8472E58882B3532581F8111A646D`.
- Parser telemetry: đúng đơn vị/dấu, từ chối mọi độ dài thiếu byte, mask sai, command sai và không ghi đè snapshot khi bị từ chối.
- Ride safety parser: 2,532 checks, 0 failures.
- Ride transport: 53 checks, 0 failures, không gửi khi giữ lock; source transport được đối chiếu SHA256 với seed.
- Simulator sau tách source UI dùng chung: self-test và 10 ảnh render, 0 lỗi.
- Boot thực tế: flash 16,777,216 byte; PSRAM 8,388,608 byte; GT911 `911` tại `0x5D`; LCD báo READY 1024 × 600. Theo dõi bản đầu 90 giây: không panic/reset/timeout framebuffer; heap nội khoảng 126,704 byte, PSRAM trống 5,867,120 byte ổn định.
- Người dùng xác nhận hiển thị và cảm ứng đúng; xác nhận **chưa nối VESC**. CAN log bus-off/recovery khi chưa có kết nối, chưa được tính là CAN/VESC nghiệm thu.

## Những giới hạn cần ghi rõ

Firmware màn không nạp Lisp vào ESC. Chưa xác nhận CAN với ESC thật hoặc BMS với pack thật. Đồng hồ và range để `--`; trip/ODO lấy từ ESC. Các kiểm tra host không thay thế kiểm tra interlock ga/phanh/Park/Reverse trên xe. Chưa chạy bài kiểm tra đồng thời CAN + BMS 30 phút.

## Xử lý lỗi nháy đường dọc

Sau lần nạp đầu, người dùng báo một đường dọc nháy/chồng hình. Xác nhận bố cục và cảm ứng ở trên chưa đồng nghĩa chất lượng quét hình đã ổn định.

Bản sửa dùng PCLK **16 MHz** thay cho 30 MHz (giữ porch, khoảng 17.5 Hz), đồng thời dùng LVGL direct mode để giảm lượng vẽ vào PSRAM. Hai framebuffer và bounce buffer 20 dòng được giữ lại. Người dùng đã xác nhận hết nháy sau lần nạp này. Giữ cấu hình 16 MHz; chưa kiểm tra tải đồng thời CAN/BMS. Tham số hiện hành xem `firmware/7b/board_7b.h`.

Log nạp: `.pio/upload-7b.log`; log khởi động bản sửa: `.pio/boot-7b-final.log`. Chưa nghiệm thu CAN/BMS vì chưa kết nối thiết bị thật.
