# Kế hoạch triển khai màn hình CYD đọc VESC qua CAN

Ngày chốt phạm vi: 2026-09-24.
Nhánh: `feature/cyd-2.8-3.5-4.3`.
Trạng thái: đặc tả để triển khai; chưa có firmware CYD được build hoặc kiểm chứng phần cứng.

## 1. Mục tiêu và phần cứng

Hỗ trợ các màn CYD 2.8, 3.5 và 4.3 inch làm đồng hồ hiển thị thông số VESC.
Cùng một bộ tính năng, bố cục thích nghi với độ phân giải; màn nhỏ chia trang
để chữ và vùng chạm đủ lớn.

Kích thước màn không xác định duy nhất bo mạch. Trước khi viết driver phải chốt
mã bo, chip ESP32/ESP32-S3, LCD, độ phân giải, loại cảm ứng, flash/PSRAM và sơ đồ chân.
Mỗi bo có profile build riêng; không đưa lựa chọn driver LCD hoặc GPIO vào menu.

Người dùng sẽ gắn module CAN. Với bo có TWAI, dùng transceiver tương thích logic
3.3 V, chọn chân TX/RX không trùng LCD/cảm ứng hoặc ngoại vi khác. Xác nhận nguồn,
GND, CAN H/L và điện trở kết thúc theo bus thực tế trước khi đấu nối. Không mặc
định tái sử dụng GPIO20/19 hay bitrate 1 Mbit/s của bản Waveshare 7B.

## 2. Phạm vi chức năng

Màn chỉ đọc thông tin. Được gửi yêu cầu lấy dữ liệu và tìm thiết bị qua CAN;
không gửi lệnh điều khiển motor, đổi mode, ghi cấu hình VESC hoặc chạy Lisp.
Không cần script Lisp riêng cho telemetry tiêu chuẩn.

| Thông tin | Nguồn và điều kiện |
| --- | --- |
| Tốc độ km/h hoặc mph | VESC; phụ thuộc cấu hình đúng bánh xe, số cực và tỉ số truyền |
| Điện áp pin | VESC |
| Phần trăm pin | Ước tính do VESC trả về, không phải SOC của BMS; chỉ hiện khi hợp lệ |
| Dòng pin và dòng motor | Hai đại lượng riêng, cần nhãn rõ ràng |
| Công suất điện W/kW | Điện áp nhân dòng pin; giữ ý nghĩa dấu theo giao thức |
| Nhiệt độ ESC và motor | Nhiệt motor chỉ khả dụng khi có cảm biến hợp lệ |
| TRIP và ODO | Lấy từ VESC; khả năng lưu qua reboot phụ thuộc firmware VESC |
| Ah/Wh tiêu thụ và thu hồi | Các bộ đếm VESC cung cấp; không tự khẳng định là số liệu trọn đời |
| Lỗi VESC | Hiện mã lỗi và mô tả khi có ánh xạ đã xác minh |
| Trạng thái kết nối | Chờ kết nối, đang nhận, dữ liệu hết hạn hoặc lỗi CAN |
| Cảnh báo tại màn | Điện áp thấp, nhiệt ESC/motor cao theo ngưỡng đã cấu hình |

Loại bỏ toàn bộ BMS/Bluetooth, quét BMS, thông tin cell, mode 1/2/3, P/R,
profile, chỉnh giới hạn dòng và giao diện thao tác Lisp. Không mang các chức
năng điều khiển của bản 7B vào firmware CYD.

## 3. Các trang

1. **Đồng hồ:** tốc độ, điện áp, phần trăm pin ước tính nếu có, các thông số phụ
   được chọn và trạng thái kết nối. Tốc độ và vùng cảnh báo có vị trí cố định.
2. **Thông số chi tiết:** các dữ liệu còn lại, chia trang hoặc cuộn tùy kích thước.
3. **Cài đặt:** kết nối, hiển thị, cảnh báo, chẩn đoán và hệ thống.

Cảnh báo phải nhìn thấy trên mọi trang. Mất dữ liệu hiện `--`, không biến thành
số 0 và không giữ nguyên số cũ như thể vẫn đang đo.

## 4. Cài đặt màn hình

| Nhóm | Nội dung | Yêu cầu |
| --- | --- | --- |
| CAN | Bitrate, CAN ID VESC, CAN ID màn | ID màn khác thiết bị trên bus; kiểm tra miền giá trị theo giao thức |
| Tìm thiết bị | Quét VESC và nhập ID thủ công | Quét ở bitrate đã chọn; xử lý trường hợp không tìm thấy hoặc có nhiều VESC |
| Hiển thị | Độ sáng, xoay 0/180 độ, km/mile, Celsius/Fahrenheit | Lưu cục bộ trên CYD |
| Cảm ứng | Hiệu chỉnh và khôi phục hiệu chỉnh | Chỉ hiện khi driver cần, thường với cảm ứng điện trở |
| Bố cục | Chọn thông số phụ trang chính | Không che tốc độ hoặc cảnh báo |
| Cảnh báo | Điện áp thấp, nhiệt ESC/motor cao | Chỉ cảnh báo; không thay đổi bảo vệ VESC |
| Chẩn đoán | Bus, thời gian từ gói cuối, lỗi CAN, nhóm dữ liệu nhận được | Phân biệt không có khung CAN, không có phản hồi từ ID đã chọn và thiếu thông số |
| Hệ thống | Phiên bản, mã bo, đặt lại cài đặt | Reset chỉ xóa cấu hình CYD, không tác động VESC |

Ngưỡng điện áp thấp phải do người dùng đặt phù hợp bộ pin; không dùng một
ngưỡng chung cho mọi xe. Các ngưỡng chưa cấu hình cần hiển thị rõ trạng thái
chưa bật. Ngưỡng nhiệt cũng không thay thế bảo vệ nhiệt của VESC.

Mặc định giữ màn sáng khi vận hành. Chưa triển khai tự tắt hoặc tự chỉnh sáng
nếu không có tín hiệu phần cứng phù hợp. Không thêm cài đặt pin/BMS vào màn.

## 5. Luồng sử dụng

### Lần bật đầu

Firmware đúng mã bo → hiệu chỉnh cảm ứng nếu cần → chọn bitrate → tìm/chọn
VESC hoặc nhập ID → xác nhận ID màn → áp dụng, lưu → vào đồng hồ.
Cho phép hoàn tất cấu hình khi VESC chưa bật; hiển thị trạng thái đang chờ.

### Bật lại và phục hồi kết nối

Vào đồng hồ ngay với cấu hình đã lưu. Tự kết nối lại, không bắt cấu hình lại
khi VESC chưa bật. Trước gói hợp lệ đầu tiên, hiển thị “Đang chờ VESC” và `--`.
Khi bus lỗi hoặc VESC ngắt, báo trạng thái và tự thử phục hồi có giới hạn tần suất.

### Tính hợp lệ và độ mới của dữ liệu

Theo dõi thời gian nhận riêng cho từng nhóm dữ liệu. Một gói mới không làm
các số liệu cũ của nhóm khác trở thành hợp lệ. Kiểm tra độ dài, loại gói, nguồn
và miền giá trị trước khi cập nhật. Chỉ nhận telemetry của VESC đã chọn.

Thông số không được firmware hỗ trợ hoặc cảm biến không hợp lệ hiện `--`;
các thông số còn lại vẫn hoạt động. Chốt chu kỳ polling và timeout theo nhóm
trong lúc triển khai; tránh tải bus quá mức và tránh cảnh báo chập chờn.

### Thay đổi cài đặt

Tiếp tục nhận CAN khi mở cài đặt. Bitrate/ID có nút **Áp dụng**, khởi tạo lại
kết nối và báo kết quả. Bỏ dữ liệu cũ khi đổi VESC để không trộn hai thiết bị.
Nếu chưa nhận được phản hồi, vẫn cho sửa cấu hình và thử lại.

Độ sáng xem trước trực tiếp; lưu sau khi ngừng chỉnh để hạn chế ghi flash.
Lưu cấu hình ngoài luồng vẽ UI. Đổi hướng màn phải đổi tọa độ cảm ứng tương ứng.

### Cảm ứng và reset

Có đường vào hiệu chỉnh lại không phụ thuộc vào tọa độ cảm ứng đang sai;
cơ chế cụ thể chọn theo nút vật lý khả dụng của từng bo. Reset cấu hình cần
xác nhận và chỉ reset màn. Sau reset trở về luồng thiết lập ban đầu.

### Quãng đường và bộ đếm

Bản đầu dùng TRIP/ODO và Ah/Wh do VESC cung cấp, ghi rõ nguồn. Không tự tạo
TRIP cục bộ, không gửi lệnh reset bộ đếm VESC. Ghi nhận hành vi bộ đếm qua
reboot VESC trong kiểm thử phần cứng trước khi mô tả khả năng lưu bền vững.

## 6. Điểm tích hợp với source hiện tại

- `platformio.ini` hiện chỉ build `waveshare-7b`; cần thêm môi trường CYD riêng.
- `firmware/7b/main.cpp` yêu cầu PSRAM 8 MB; không áp điều kiện này cho mọi CYD.
- `firmware/7b/board_7b.h` và `display_port.cpp` dành cho LCD RGB 1024x600;
  cần driver theo từng bo, không chỉ thay độ phân giải.
- `ui/7b/preview_ui.c` có bố cục cố định 1024x600; cần UI CYD thích nghi.
- `firmware/7b/telemetry_decode.c` đang đọc tốc độ, dòng motor, điện áp, SOC,
  nhiệt, TRIP/ODO và lỗi bằng `COMM_GET_VALUES_SETUP_SELECTIVE`.
- `firmware/7b/vesc/comm_can.c` đã có parser Status 1–6; đối chiếu phiên bản
  VESC đích trước khi tái sử dụng và bổ sung kiểm tra gói nếu cần.
- `firmware/7b/backend.cpp` đang gắn CAN với polling ride-mode, GPIO và
  IO expander của 7B; backend CYD phải tách các phụ thuộc này.

Giữ bản build 7B hoạt động; ưu tiên chia sẻ decoder/protocol có kiểm thử,
không sao chép toàn bộ giao diện hoặc logic điều khiển 7B vào CYD.

## 7. Thứ tự triển khai và nghiệm thu

1. Chốt mã bo và module CAN cho mỗi kích thước; lập bảng chân và profile build.
2. Dựng LCD/cảm ứng/độ sáng, kiểm tra tài nguyên RAM/flash trên từng bo.
3. Tách backend chỉ đọc, hỗ trợ chọn bitrate/ID và phục hồi kết nối.
4. Dựng ba trang, dữ liệu hợp lệ/hết hạn và cảnh báo.
5. Thêm cài đặt, lưu bền vững, hiệu chỉnh cảm ứng và reset.
6. Build từng profile CYD và kiểm tra bản 7B vẫn build được.
7. Test decoder với gói hợp lệ, thiếu byte, sai ID và giá trị không hợp lệ;
   test riêng độ mới theo nhóm, đổi target và chuyển đơn vị.
8. Kiểm chứng trên từng bo: chạm sau xoay màn, độ sáng, lưu qua reboot,
   sai bitrate/ID, rút/cắm CAN, VESC reboot, thiếu cảm biến và cảnh báo.
9. Kiểm tra đường gửi CAN chỉ chứa các lệnh khám phá/đọc cần thiết; không có
   lệnh điều khiển, thay mode, ghi cấu hình hoặc reset bộ đếm.

Chưa xác nhận hỗ trợ phần cứng chỉ bằng build thành công. Các mã bo, chân CAN,
bitrate thực tế, phiên bản firmware VESC và cơ chế vào hiệu chỉnh bằng nút
vật lý là các thông tin còn cần chốt khi triển khai.
