# Yêu cầu chuyển Super VESC Display sang màn hình Waveshare 7B

- Ngày ghi nhận: 2026-09-17.
- Nhánh triển khai: `feat/waveshare-7b`.
- Phần cứng đích: **Waveshare ESP32-S3-Touch-LCD-7B**, màn hình 7 inch, **1024 × 600**, flash 16 MB, PSRAM octal 8 MB (N16R8).
- Trạng thái: đã tạo nhánh và tài liệu; **chưa port firmware**. Các checkbox dưới đây là công việc cần thực hiện, chưa phải kết quả kiểm thử.

## 1. Mục tiêu và phạm vi

Chuyển firmware đang cấu hình cho màn hình 4 inch 480 × 480 sang board 7B, giữ các chức năng VESC hiện có: dashboard, settings, CAN telemetry, BLE bridge, tính pin, trip và đọc trạng thái Lisp.

Nguồn phần cứng là nhánh `feat/display-ui` của `huygithcm/bambu-p1s-led`, đã ghi lại trong [tài liệu phần cứng 7B](HARDWARE_7B_REFERENCE.md), gồm pinout, timing và liên kết source cố định theo commit.

Cấu hình hai chiều qua Lisp là hạng mục riêng, chưa thuộc tiêu chí hoàn thành việc chuyển màn hình. Khi triển khai hạng mục đó, dùng [tài liệu Lisp settings](LISP_SETTINGS_REFERENCE.md).

Trước khi thay script, đọc [đánh giá tương thích Lisp mới](LISP_COMPATIBILITY_REVIEW.md), đặc biệt các thay đổi về điều khiển ga/phanh/cruise và điều phối CAN.

## 2. Các thay đổi bắt buộc

### 2.1. Build và cấu hình board

- [ ] Thêm môi trường PlatformIO riêng cho board 7B, xác định rõ lệnh build/upload.
- [ ] Cấu hình flash 16 MB, PSRAM octal 8 MB, `qio_opi` và partition phù hợp dung lượng firmware/OTA đang dùng.
- [ ] Dùng toolchain hỗ trợ `num_fbs`, `bounce_buffer_size_px` và callback đồng bộ framebuffer của `esp_lcd`; lấy cấu hình đã đọc trong repo tham khảo làm điểm bắt đầu.
- [ ] Kiểm tra tương thích Arduino core, NimBLE, LVGL, OTA và các thư viện hiện tại khi đổi toolchain.
- [ ] Gom pinout/timing riêng của 7B vào cấu hình board để tránh trộn với pin của màn 4 inch.

Vị trí dự kiến: `platformio.ini`, cấu hình board mới và các include liên quan.

### 2.2. Driver LCD và bộ nhớ

- [ ] Chuyển độ phân giải sang 1024 × 600, RGB565, bus RGB 16 bit.
- [ ] Thay pinout và timing theo `board_7b.h` của repo tham khảo; cấu hình ban đầu PCLK 30 MHz, `pclk_active_neg = 1`.
- [ ] Dùng hai framebuffer trong PSRAM: tổng 2,457,600 byte, khoảng 2.34 MiB.
- [ ] Cấu hình bounce buffer 20 dòng theo nguồn tham khảo: hai buffer dùng tổng 81,920 byte SRAM.
- [ ] Ghép driver với LVGL, xác định buffer đang vẽ/đang quét và thời điểm báo flush hoàn tất để tránh ghi đè frame đang hiển thị.
- [ ] Tính cả bộ nhớ draw buffer LVGL và các module khác; kiểm tra cấp phát thất bại và tránh cấp thêm bản sao toàn màn hình không cần thiết.

Vị trí dự kiến: `src/Display_ST7701.*`, `src/LVGL_Driver.*`, cấu hình LVGL và driver RGB mới nếu cần. Cấu hình RGB 7B phải theo nguồn tham khảo, không mặc định dùng lại chuỗi khởi tạo ST7701 của board cũ.

### 2.3. IO expander, backlight và reset

- [ ] Thêm driver IO expander tại địa chỉ I2C 0x24 theo giao thức trong repo tham khảo.
- [ ] Điều khiển reset touch qua IO1, backlight qua IO2, reset LCD qua IO3; quản lý chung output shadow để không vô tình thay đổi các chân khác.
- [ ] Thay điều khiển backlight trực tiếp bằng PWM qua expander.
- [ ] Ánh xạ thang độ sáng UI 0–100% sang miền driver 0–97% của nguồn tham khảo; kiểm tra cả mức thấp nhất và khôi phục sau reboot.
- [ ] Thiết lập IO5 chọn CAN đúng thời điểm trong trình tự khởi động.

Vị trí dự kiến: driver IO expander mới, `src/Display_ST7701.*`, `src/dev_settings.cpp` và trình tự setup trong `src/main.cpp`.

### 2.4. Cảm ứng GT911

- [ ] Đổi I2C sang SDA GPIO 8, SCL GPIO 9, tốc độ 400 kHz; INT dùng GPIO 4.
- [ ] Reset GT911 qua expander, giữ INT thấp khi nhả reset như nguồn tham khảo.
- [ ] Thử địa chỉ 0x5D, fallback 0x14 khi cần.
- [ ] Cập nhật miền tọa độ 1024 × 600 và kiểm tra chiều trục, rotation, cạnh màn hình.
- [ ] Dùng chung bus I2C với expander nhất quán; tránh khởi tạo lại bus bằng pin của board cũ.

Vị trí dự kiến: `src/I2C_Driver.*`, `src/Touch_GT911.*`, phần input LVGL.

### 2.5. Giao diện LVGL

- [ ] Bố trí lại dashboard theo màn ngang 1024 × 600, giữ đầy đủ các chỉ số và trạng thái hiện có.
- [ ] Bố trí lại settings, nút điều hướng, vùng cuộn và các vùng chạm.
- [ ] Cập nhật ảnh nền, asset phụ thuộc kích thước và font khi cần; không chỉ đổi kích thước màn rồi để UI nằm trong vùng 480 × 480.
- [ ] Đồng bộ source UI với project GUI Guider nếu vẫn dùng công cụ này, tránh lần generate sau ghi đè layout mới.
- [ ] Kiểm tra nội dung dài, số nhiều chữ số, chuyển trang và thao tác chạm ở các góc.

Vị trí dự kiến: `Super_VESC_Display/generated/`, `Super_VESC_Display/custom/`, project `.guiguider` và asset liên quan.

### 2.6. CAN và USB

- [ ] Xác minh TX/RX theo schematic của đúng board 7B trước khi cấu hình TWAI. Tài liệu repo tham khảo ghi GPIO 19/20 dùng chung USB/CAN, nhưng chưa xác nhận chiều TX/RX trong phần driver đã đọc.
- [ ] Bỏ cấu hình CAN TX 6 / RX 0 hiện tại: GPIO 0 là chân dữ liệu LCD trên 7B.
- [ ] Thiết lập IO5 theo chế độ CAN và xác minh hoạt động của transceiver trên board.
- [ ] Đối chiếu `ARDUINO_USB_CDC_ON_BOOT`, `ARDUINO_USB_MODE`, đường upload và log với tài nguyên USB/CAN dùng chung.
- [ ] Giữ cơ chế cấu hình CAN baudrate, controller ID, target VESC ID; kiểm tra telemetry, phản hồi lệnh và BLE–CAN bridge.

Vị trí dự kiến: `src/comm_can.cpp`, `src/main.cpp`, `platformio.ini` và cấu hình board mới.

### 2.7. Các chức năng cần bảo toàn

- [ ] Kiểm tra tính pin, trip và lưu/đọc settings trong NVS sau reboot.
- [ ] Kiểm tra BLE bridge, các chức năng BLE đang bật và OTA sau thay đổi toolchain/bộ nhớ.
- [ ] Kiểm tra polling Lisp và hiển thị cruise/profile trên layout mới.
- [ ] Giữ logic giao thức VESC hiện có trừ những thay đổi cần thiết để thích nghi board/toolchain; ghi nhận lỗi có sẵn riêng để không nhầm với lỗi port.

## 3. Thứ tự triển khai

1. Tạo cấu hình build 7B, chạy LCD, backlight và touch với màn kiểm tra đơn giản.
2. Ghép driver LVGL và kiểm tra đồng bộ framebuffer/bộ nhớ.
3. Chuyển dashboard và settings sang layout 1024 × 600.
4. Cấu hình CAN, khôi phục telemetry, polling Lisp và BLE bridge.
5. Kiểm tra chức năng lưu trữ, OTA và độ ổn định khi các module hoạt động đồng thời.
6. Cập nhật hướng dẫn build/upload, pinout và kết quả kiểm tra thực tế.

## 4. Tiêu chí nghiệm thu

- [ ] Build môi trường 7B thành công; ghi lại toolchain và lệnh đã dùng.
- [ ] Thiết bị nhận đúng PSRAM, cấp phát framebuffer thành công và khởi động lại ổn định.
- [ ] LCD hiển thị đúng màu, đúng 1024 × 600; không có xé hình/nhiễu quan sát được khi chuyển trang và cập nhật telemetry.
- [ ] Touch khớp vị trí ở tâm, bốn góc và các nút điều khiển.
- [ ] Độ sáng thay đổi đúng và được khôi phục sau reboot.
- [ ] CAN nhận dữ liệu từ VESC thật; BLE bridge giao tiếp được với VESC Tool.
- [ ] Dashboard, settings, cruise/profile, pin và trip hoạt động như trước sau khi bố trí lại.
- [ ] Chạy thử liên tục ít nhất 30 phút với LCD, touch, CAN và BLE cùng hoạt động; ghi lại reset/watchdog, lỗi cấp phát và bộ nhớ còn lại nếu có.
- [ ] Upload, log và OTA đang dùng được xác minh với cấu hình USB/CAN đã chọn.

Build thành công chỉ xác nhận khả năng biên dịch. Các mục hiển thị, touch, CAN và chạy đồng thời phải được đánh dấu dựa trên kiểm tra phần cứng thực tế, kèm kết quả và lỗi còn tồn tại.

## 5. Điểm cần xác minh trước khi chốt triển khai

- Chiều TX/RX của cặp GPIO 19/20 và đường upload/log khi chọn CAN.
- Khả năng tương thích thư viện hiện tại với toolchain của cấu hình tham khảo.
- Cách tích hợp double framebuffer với phiên bản LVGL đang có và lượng SRAM/PSRAM còn lại khi chạy đầy đủ module.
- Partition cuối cùng đáp ứng firmware và cơ chế OTA của dự án.
