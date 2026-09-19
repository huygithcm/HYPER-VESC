# Dọn source cho bản 7B

Ngày 2026-09-18. Theo yêu cầu thay UI và triển khai màn 7 inch, build mặc định đã chuyển từ bản 4 inch sang `waveshare-7b`.

## Đã loại khỏi source build hiện hành

- Driver ST7701/I2C/GT911/LVGL cũ và entry point 4 inch.
- BLE media, music, keyboard, bridge và OTA cũ phụ thuộc app/UI trước đây.
- UI generated/custom 480 × 480, project GUI Guider, asset/import và thư mục tạm của project cũ.
- Cấu hình PlatformIO, LVGL và script tích hợp GUI cũ.
- Hai bản sao `simulator/7b/preview_ui.c/.h`: thay bằng một source dùng chung trong `ui/7b`.
- Font Antonio 50/100 không dùng: không còn trong danh sách build.
- Xóa 7 header vừa copy nhưng không có trong dependency build: `packet_parser`, `vesc_io_data`, `vesc_lisp_code`, `vesc_lisp_console`, `vesc_lisp_panel`, `vesc_lisp_poll`, `vesc_rt_data`. Bản tham khảo gốc vẫn ở clone.
- Widget video cũ: tắt trong cấu hình LVGL để không kéo SD/video vào firmware.

Mã 4 inch được chuyển nguyên vẹn vào `archive/legacy-4inch`, gồm `src`, `ble_example`, các thư mục generated/custom/import/temp/lib của GUI và các cấu hình tương ứng. README trước port ở `archive/legacy-4inch/README-before-7b.md`. Archive không được PlatformIO hoặc simulator biên dịch.

Bộ duyệt tự động từ chối xóa đệ quy hàng loạt source/asset vì phạm vi quá rộng. Do đó bản này dùng lưu trữ có thể khôi phục, chưa xóa hẳn archive. Những file người dùng đã xóa trước tác vụ không được khôi phục.

## Giữ lại có chủ đích

`esp32s3-vesc-bms` là nguồn tham khảo và cung cấp font; `example_UI` là mẫu đã duyệt; `lisp` chứa script ESC. LVGL/Win32 driver và giấy phép vẫn cần cho firmware/simulator. Các nguồn VESC CAN, ride-mode và JK BMS được copy có chọn lọc vào firmware kèm nguồn gốc trong NOTICE, không sửa clone tham khảo.
