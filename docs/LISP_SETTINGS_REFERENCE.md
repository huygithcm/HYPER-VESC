# Tài liệu tham khảo: Lisp settings qua CAN

Xem [đánh giá tương thích và luồng tích hợp](LISP_COMPATIBILITY_REVIEW.md) trước khi thay script hiện tại.

Repo tham khảo: [huygithcm/esp32p4-android-auto](https://github.com/huygithcm/esp32p4-android-auto).

Đã đọc source ngày 2026-09-17 tại commit `842b25ea498c9e341c28e530cf6621fce5919c0a`. Các liên kết dưới đây cố định theo commit để tra cứu đúng phiên bản. Đây là tài liệu tham khảo triển khai; chưa tích hợp chức năng này vào Super VESC Display và chưa kiểm chứng trên phần cứng.

## Các file cần đọc

| Nguồn | Nội dung |
| --- | --- |
| [lisp/README.md](https://github.com/huygithcm/esp32p4-android-auto/blob/842b25ea498c9e341c28e530cf6621fce5919c0a/lisp/README.md) | Hướng dẫn thêm control và đồng bộ mô tả UI, trạng thái, action. |
| [lisp/main.lisp](https://github.com/huygithcm/esp32p4-android-auto/blob/842b25ea498c9e341c28e530cf6621fce5919c0a/lisp/main.lisp) | `panel-send-ui`, `panel-send-state`, `panel-action`, `panel-handle`, đăng ký `event-data-rx`, áp dụng profile và lưu âm lượng. |
| [vesc_lisp_panel.h](https://github.com/huygithcm/esp32p4-android-auto/blob/842b25ea498c9e341c28e530cf6621fce5919c0a/components/vesc_can/include/vesc_can/vesc_lisp_panel.h) | Định nghĩa giao thức, loại control, model và API. |
| [vesc_lisp_panel.c](https://github.com/huygithcm/esp32p4-android-auto/blob/842b25ea498c9e341c28e530cf6621fce5919c0a/components/vesc_can/vesc_lisp_panel.c) | Gửi yêu cầu CAN, hàng đợi action, polling, parse UI/state, mutex bảo vệ model. |
| [custom/lisp_panel.c](https://github.com/huygithcm/esp32p4-android-auto/blob/842b25ea498c9e341c28e530cf6621fce5919c0a/Super_VESC_Display/custom/lisp_panel.c) | Dựng giao diện LVGL từ mô tả do Lisp trả về và gửi thao tác người dùng. |
| [vesc_lisp_code.c](https://github.com/huygithcm/esp32p4-android-auto/blob/842b25ea498c9e341c28e530cf6621fce5919c0a/components/vesc_can/vesc_lisp_code.c) | Đọc/upload script, bật/tắt script và REPL. |
| [main/lisp_http.h](https://github.com/huygithcm/esp32p4-android-auto/blob/842b25ea498c9e341c28e530cf6621fce5919c0a/main/lisp_http.h) | API web editor: đọc/upload code, run/stop, REPL, console. |

## Luồng settings hai chiều

1. Màn hình mở panel, gửi `REQ_UI` qua `COMM_CUSTOM_APP_DATA` (36).
2. VESC chuyển payload vào Lisp qua `event-data-rx`; `panel-handle` gọi `panel-send-ui`.
3. Lisp trả `UI_DESC`; màn hình dựng control theo ID, loại, nhãn, giới hạn, bước và giá trị.
4. Người dùng thao tác; callback LVGL gọi `vesc_lisp_panel_send_action`, đưa `{id, value}` vào queue.
5. CAN poll task lấy action, gửi `ACTION`; Lisp gọi `panel-action` để cập nhật biến hoặc thực thi hàm.
6. Lisp trả `STATE` sau action. Khi panel mở, màn hình tiếp tục yêu cầu trạng thái theo chu kỳ 200 ms.

Lisp trả lời bằng `(send-data pbuf 2 reply-id)`, chỉ rõ CAN ID nhận. Header upstream ghi cơ chế này cần firmware VESC 6.05+; cần đối chiếu firmware đích khi port.

### Giao thức chính

Payload ứng dụng bắt đầu bằng magic `0x56 0x50` (`VP`). Byte `COMM_CUSTOM_APP_DATA` nằm ở lớp lệnh bao ngoài.

| Chiều | Message | Payload sau message |
| --- | --- | --- |
| Màn hình → Lisp | `REQ_UI` = `0x01` | `reply_can_id` |
| Màn hình → Lisp | `ACTION` = `0x02` | `reply_can_id`, `ctrl_id`, `int32(value × 1000)` |
| Màn hình → Lisp | `REQ_STATE` = `0x03` | `reply_can_id` |
| Lisp → màn hình | `UI_DESC` = `0x81` | version, count, danh sách mô tả control |
| Lisp → màn hình | `STATE` = `0x82` | count, các cặp ID/giá trị |

Số nguyên nhiều byte dùng big-endian. Giá trị số dùng fixed-point ×1000, không dùng `float32_auto`. Model hỗ trợ tối đa 16 control: toggle, button, number và label chỉ đọc. Repo còn có gói dashboard `REQ_DASH`/`DASH` riêng cho cruise và profile.

## Những setting thực sự có trong script tham khảo

- Bật/tắt throttle; nút phát beep; chỉnh âm lượng beep từ 0 đến 50, bước 5.
- Chọn ba profile Slow/Medium/Fast. `apply-profile` đặt `max-speed` tương ứng 25/40/60 km/h và `l-current-max-scale` tương ứng 0.5/0.67/1.0 bằng `conf-set`.
- Tốc độ và hệ số dòng của từng profile vẫn ghi cứng trong script. Panel chọn profile, chưa phải form sửa tùy ý các giới hạn này.
- Âm lượng dùng `eeprom-read-i`/`eeprom-store-i`; action đánh dấu dirty, vòng lặp lưu mỗi 2 giây khi có thay đổi, đồng thời flush khi shutdown. Không suy ra mọi setting đều được lưu bền vững từ cơ chế lưu âm lượng này.

Để thêm setting, sửa đồng bộ `panel-send-ui`, `panel-send-state` và `panel-action`: ID phải duy nhất; count phải đúng số phần tử của từng gói (button không cần giá trị trong STATE).

## REPL và chỉnh script là luồng riêng

`vesc_lisp_code_repl` gửi `COMM_LISP_REPL_CMD` với biểu thức kết thúc NUL. Kết quả đi qua `COMM_LISP_PRINT`, không phải ACK xác nhận setting. Web editor cung cấp `/lisp/api/repl` cùng API đọc/upload/run/stop script.

Luồng quick-action/settings ở trên dùng custom app data và action ID; không gửi biểu thức REPL cho mỗi lần bấm nút.

## Đối chiếu với Super VESC Display hiện tại

| Phần | Trạng thái hiện tại | Hướng tham khảo |
| --- | --- | --- |
| [Lisp polling](../src/vesc_lisp_poll.cpp) | Đọc biến bằng `COMM_LISP_GET_STATS`, cập nhật cruise/profile trên UI. | Bổ sung transport custom app data hai chiều và state có cấu trúc. |
| [Script VESC](../main.lisp) | Profile ghi cứng, chuyển bằng nút TX; chưa có handler settings từ màn hình. | Thêm mô tả UI, handler action và phản hồi state. |
| [Device settings](../src/dev_settings.cpp) | Lưu cấu hình màn hình trong NVS ESP32. | Giữ rõ quyền sở hữu: cấu hình màn hình ở ESP32, setting do Lisp quản lý ở VESC. |
| [Motor limits](../src/vesc_limits.cpp) | Gửi trực tiếp `COMM_SET_MCCONF` với serialization đơn giản hóa, chưa qua Lisp. | Tham khảo action có định nghĩa rõ và `conf-set` phía Lisp cho các tham số cần hỗ trợ. |

Khi port từ ESP-IDF/ESP32-P4 sang Arduino/ESP32-S3, cần thích nghi CAN task, queue/mutex và vòng đời UI. Upstream tuần tự hóa request ở CAN poll task và tạm dừng polling khi truyền code; đây là phần cần giữ khi tích hợp để tránh chồng phản hồi. Cần bổ sung kiểm tra chiều dài payload, miền giá trị phía nhận và xác nhận trạng thái thực tế trước khi coi thao tác đã thành công. Các mục này là đề xuất cho việc port, không phải xác nhận upstream đã xử lý đầy đủ.
