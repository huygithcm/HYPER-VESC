# Nguồn chức năng đích cho màn hình 7B

Ngày: 2026-09-18. Người dùng chỉ định nhánh này làm nguồn các chức năng muốn triển khai trên Waveshare ESP32-S3-Touch-LCD-7B.

- Repo: https://github.com/huygithcm/esp32p4-android-auto.git
- Nhánh: `port/esp32s3-vesc-bms`.
- Commit đã clone: `664213605dffeed5d48e9653e6fe38dbcc28ae9c`.
- Clone local độc lập: [`../esp32s3-vesc-bms/`](../esp32s3-vesc-bms/README.md), single branch, depth 1.
- Phần cứng đích: [7B, 1024×600, N16R8](HARDWARE_7B_REFERENCE.md).

## Phạm vi chức năng từ source seed

1. Dashboard VESC và telemetry qua CAN.
2. Chỉnh dòng từng mode, đọc giá trị yêu cầu/hiệu dụng và lưu cấu hình ở VESC.
3. Hiển thị số 1/2/3/R/P, trạng thái chưa xác nhận, sequence/timeout và Park/Reverse.
4. JK BMS qua BLE: scan, chọn thiết bị, kết nối, model/parser và UI overview/cell/wire.
5. Settings, trip/statistics và các thành phần UI hỗ trợ đã được giữ trong snapshot.
6. Lisp chạy trên ESC; host regression tests cho parser/transport/gear và một phần logic Lisp.

Cadence/PAS, QR, editor Lisp và các UI phụ thuộc vẫn có trong source; sự hiện diện của chúng không tự chốt rằng mọi tính năng phụ đều phải bật trong sản phẩm cuối. Checklist [FEATURE_SELECTION.md](FEATURE_SELECTION.md) dùng để chọn chi tiết khi cần; chưa tự động xóa source cũ chỉ từ việc clone.

## Các đường dẫn chính

| Nhóm | Nguồn |
| --- | --- |
| Tổng quan/phạm vi | [README](../esp32s3-vesc-bms/README.md) |
| Dashboard/UI | `esp32s3-vesc-bms/Super_VESC_Display/custom/` và `generated/` |
| Mode/P/R transport | `components/vesc_can/vesc_ride_mode.c`, `vesc_ride_mode_parse.c`, `include/vesc_can/vesc_ride_mode_wire.h` trong clone |
| JK BMS | `components/bms/`, `main/ble_bms_client.*`, `custom/bms_view.c` trong clone |
| Lisp | [lisp/main.lisp](../esp32s3-vesc-bms/lisp/main.lisp) |
| Kiểm thử | `scripts/tests/`, `scripts/test_lisp_safety.py` trong clone |

## Giới hạn và tài liệu cũ

- Đây là **source seed, chưa build/flash được trên S3**. CMake cố ý chặn build cho tới khi có startup và dependency graph S3.
- UI nguồn thiết kế 800×480; cần bố trí lại 1024×600 và ghép driver 7B, LVGL lock, NVS, CAN và NimBLE native S3.
- Android Auto, ESP-Hosted/C6 và BSP P4 đã bị loại; không đưa chúng trở lại chỉ để giải quyết include/phụ thuộc cũ.
- `docs/RIDE_MODE_REVERSE_BE_CONTRACT.md` tự đánh dấu SUPERSEDED; không dùng định dạng speed/percent và pin-ppm cũ để triển khai.
- Tài liệu bring-up còn câu lệnh nạp P4, nhắc Android Auto và liên kết tới file không có trong snapshot. Cần chuyển thành hướng dẫn S3 trước khi sử dụng; không chạy các lệnh nạp đó cho 7B.
- Log upstream ghi kết quả host tests; lần clone này chưa chạy lại tests và chưa xác nhận BMS/ESC thật.
- Đã so sánh `lisp/main.lisp` ở workspace với bản trong clone: không có khác biệt nội dung khi bỏ qua kết thúc dòng. Hash file thô khác nhau do định dạng kết thúc dòng.

Lần này chỉ clone và ghi nhận nguồn tham khảo, chưa merge, copy đè source hoặc triển khai firmware.
