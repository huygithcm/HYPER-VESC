# Triển khai firmware 7B

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
