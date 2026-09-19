# Super VESC Display — Waveshare 7B

Firmware ESP32-S3-Touch-LCD-7B: LCD RGB 1024 × 600, flash 16 MB, PSRAM octal 8 MB. UI dùng chung với [simulator Windows](simulator/7b/README.md), theo thiết kế trong `example_UI`.

## Build và nạp

```powershell
python -m platformio run -e waveshare-7b -j 2
python -m platformio run -e waveshare-7b -t upload --upload-port COM9 -j 2
python -m platformio device monitor --port COM9 --baud 115200
```

Máy hiện tại dùng môi trường Python riêng cho build; xem [biên bản triển khai](docs/DEPLOYMENT_7B.md) để dùng đúng lệnh và xem kết quả phần cứng.

Toolchain pin: pioarduino 55.03.311 / Arduino 3.3.11 / ESP-IDF 5.5.5, NimBLE-Arduino 2.5.0. Upload/log qua CH343, không dùng USB CDC trên GPIO19/20 vì hai chân này dành cho CAN. CAN TX20/RX19, 1000 kbit/s, node màn 254; target mặc định 10, đổi trong Settings rồi khởi động lại.

## Source hiện hành

- `firmware/7b`: driver LCD/GT911/IO expander, CAN/Lisp, JK BMS BLE và NVS.
- `ui/7b`: giao diện Dashboard, Ride modes, BMS và Settings.
- `simulator/7b`: host Windows, dữ liệu mẫu chỉ dành cho duyệt UI.
- `Super_VESC_Display/lvgl` và `ports`: LVGL cùng driver host đang dùng.
- `ui/7b/fonts`: bốn font Antonio cần cho build, kèm giấy phép; không cần submodule.
- Script ESC và các nguồn tham khảo được giữ local, không nằm trong bản nguồn build trên remote.
- `archive/legacy-4inch`: chỉ giữ tài liệu trên remote; mã/project cũ được giữ local.

## Thao tác

Nhấn thanh dưới để chuyển trang. Giữ vòng số để yêu cầu Park/Drive; chỉ trạng thái được Lisp xác nhận mới hiển thị P/1/2/3/R. Mode và giới hạn dòng được gửi đến ESC, có phản hồi hoặc timeout; ESC giữ quyền kiểm tra phanh/ga, reverse và giới hạn dòng.

BMS → Device → Scan, chờ khoảng 5 giây rồi mở lại Device để chọn JK BMS. Thiết bị được lưu sau khi nhận dạng đúng giao thức; dữ liệu không hợp lệ hoặc mất kết nối hiển thị `--`. Hỗ trợ xem cell/wire của layout JK 24S/32S bằng cuộn. Độ sáng được lưu sau khi ngừng chỉnh khoảng 1,5 giây.

Đồng hồ chưa đồng bộ thời gian và range chưa có thuật toán tin cậy nên để `--`. ODO/TRIP lấy từ ESC, không giả làm dữ liệu đã lưu cục bộ. Bản này không mang lại media, Android Auto, BLE keyboard, OTA hoặc BLE bridge của bản 4 inch.

## Tài liệu

[Phần cứng](docs/HARDWARE_7B_REFERENCE.md) · [UI đã duyệt](docs/UI_7B_REVIEW.md) · [Nguồn S3](docs/ESP32S3_VESC_BMS_REFERENCE.md) · [Lisp settings](docs/LISP_SETTINGS_REFERENCE.md) · [Nguồn/giấy phép](firmware/7b/NOTICE.md)

Các tài liệu migration/review cũ phản ánh thời điểm trước port; trạng thái triển khai hiện tại được ghi riêng trong `docs/DEPLOYMENT_7B.md`.
