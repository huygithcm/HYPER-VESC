# Simulator UI 7B trên Windows

UI LVGL 1024 × 600 theo ảnh `example_UI`, font Antonio từ seed đã clone. Xem [tài liệu duyệt UI](../../docs/UI_7B_REVIEW.md).

Mở `build/vesc_7b_simulator.exe`, hoặc chạy từ thư mục repo:

```powershell
.\simulator\7b\run.ps1
# Đóng simulator trước khi build lại:
.\simulator\7b\run.ps1 -Build
```

Chuột thay cảm ứng. Thanh dưới chuyển Dashboard / Ride modes / BMS / Settings. PREVIEW chọn P/1/2/3/R, mất CAN, lỗi ESC và animation. Chỉnh mode, reverse, target ID và brightness chỉ thay đổi RAM mẫu.

Không giả lập CPU ESP32-S3; không kết nối serial, CAN, BLE hoặc Lisp runtime. Chưa ghép UI vào firmware. Build dùng CMake/Ninja và MinGW32 trên máy; LVGL/Win32 trong repo, font trong `esp32s3-vesc-bms`.

Chạy executable với `--review` để self-test và xuất BMP vào working directory cùng `review-result.txt`; exit code 0 là thành công. PNG duyệt ở `review/`, render từ widget thật. Callback kiểm tra bằng LVGL event; chưa kiểm thử tự động đường nhập chuột Windows.
