# Tên hiển thị 7B

Tên mặc định: **HYPER ESC**.

Đổi `DISPLAY_BRAND_NAME` trong `ui/7b/branding.h`, sau đó build lại simulator hoặc build và nạp firmware lên màn. Hiện đây là cấu hình lúc build, chưa phải tùy chọn đổi tên trong Settings/Lisp.

Dashboard và demo startup dùng chung `DISPLAY_BRAND_NAME`. Demo có chữ hiện dần, nét cam mở rộng và chuyển dashboard sau 2.2 giây; đã bật trong firmware và nạp lên màn thật qua COM9 ngày 2026-09-18. Trên phần cứng hiệu ứng chạy một lần khi khởi động; RGB giữ 16 MHz.

Chạy `simulator/7b/build/vesc_7b_simulator.exe --startup-demo` để xem lặp lại mỗi 5.5 giây. Chạy không tham số thì hiệu ứng chỉ xuất hiện một lần.

Nên dùng tên ASCII ngắn, tối đa 16 ký tự với bộ font hiện tại. Tên nhận diện giao thức CAN/Bluetooth được cấu hình riêng.
