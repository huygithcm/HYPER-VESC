# Kiểm tra phần cứng 7B trên COM9

Ngày: 2026-09-18. Đọc thiết bị thật trên Windows ngoài sandbox; không ghi/xóa flash. Esptool nạp stub tạm vào RAM để đọc flash ID và reset lại board sau khi hoàn tất.

## Kết quả trực tiếp từ thiết bị

| Thuộc tính | Kết quả |
| --- | --- |
| Cổng | COM9 — USB-Enhanced-SERIAL CH343 |
| USB VID:PID | 1A86:55D3 |
| Chip | ESP32-S3 (QFN56), revision v0.2 |
| CPU theo nhận diện chip | Dual Core + LP Core, 240 MHz capability |
| Crystal | 40 MHz |
| Flash phát hiện bằng flash ID | **16 MB** |
| Flash manufacturer / device | 46 / 4018 (giá trị esptool hiển thị) |
| Flash bus / voltage theo eFuse | Quad, 4 data lines / 3.3 V |
| PSRAM theo nhận diện chip | **Embedded PSRAM 8 MB (AP_3v3)** |

Lệnh đã chạy thành công:

```powershell
python -m serial.tools.list_ports -v
python -m esptool --port COM9 --baud 115200 flash-id
```

Esptool phiên bản 5.2.0. Dung lượng thực tế phù hợp cấu hình **N16R8**, flash quad + PSRAM octal (`qio_opi` trong cấu hình Arduino tham khảo).

Đã thử đọc log khởi động ở 115200 baud trong 6 giây sau reset, lọc các dòng chip/memory/display. Firmware hiện tại không phát dòng chẩn đoán phù hợp trong khoảng đọc này. Vì vậy chưa đo free heap, PSRAM khả dụng sau khởi tạo hoặc kiểm tra toàn bộ RAM. Dung lượng PSRAM 8 MB ở trên là kết quả nhận diện chip của esptool, không phải kiểm thử đọc/ghi RAM.

## Đối chiếu nhà sản xuất cho HMI 7B

Theo [Waveshare ESP32-S3-Touch-LCD-7B](https://docs.waveshare.com/ESP32-S3-Touch-LCD-7B): module N16R8, SRAM nội 512 KB, flash 16 MB, PSRAM 8 MB, LCD RGB 7 inch 1024×600 và touch I2C. SRAM 512 KB là thông số chip, không phải free heap đo được trên board.

Thông tin giao tiếp cần dùng khi port:

- CAN TX **GPIO20**, CAN RX **GPIO19**.
- EXIO5 mức cao chọn CAN, mức thấp chọn USB; GPIO19/20 dùng chung USB native.
- Touch/I2C: SDA GPIO8, SCL GPIO9, INT GPIO4; reset touch EXIO1.
- Backlight enable EXIO2; bảng giao tiếp hiện tại của hãng còn liệt kê EXIO6 là LCD_VDD_EN. Cần đối chiếu trình tự cấp nguồn/reset theo revision board và driver đã chạy, không suy ra mọi EXIO từ tên trong repo tham khảo.

Việc đọc chip qua COM9 không tự nhận diện được mã panel hoặc revision PCB. Model 7B được xác định từ phần cứng người dùng đã chỉ định và repo bring-up trước đó; chưa có kiểm tra LCD/touch trực tiếp trong lần này.

## Cấu hình bộ nhớ cho triển khai

- Flash size 16 MB; partition phải vừa 16 MB và đáp ứng yêu cầu OTA cuối cùng.
- PSRAM octal 8 MB; framebuffer RGB đặt trong PSRAM.
- Hai framebuffer RGB565 1024×600 chiếm 2,457,600 byte (~2.34 MiB).
- Bounce buffer 20 dòng theo source tham khảo dùng tổng 81,920 byte SRAM; cần đo SRAM còn lại khi ghép CAN, NimBLE/BMS và LVGL.
- Chưa thay đổi firmware đang có trên thiết bị.
