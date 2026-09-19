# Đánh giá tương thích khi chuyển sang Lisp mới

Ngày: 2026-09-17. Phạm vi: đối chiếu source Super VESC Display hiện tại với `lisp/main.lisp` và các module panel của `huygithcm/esp32p4-android-auto`, commit `842b25ea498c9e341c28e530cf6621fce5919c0a`.

“Lisp mới” trong tài liệu là script của repo tham khảo trên, không phải một bản script khác do người dùng sửa. [Nguồn và các liên kết theo commit](LISP_SETTINGS_REFERENCE.md). Đây là review tĩnh, chưa upload script, chưa chạy motor và chưa xác nhận phiên bản firmware VESC thực tế.

## Kết luận

**Chưa tương thích để thay trực tiếp `main.lisp` rồi sử dụng đầy đủ.** Có thể tái sử dụng nền CAN và telemetry, nhưng phải thêm transport panel, sửa nguồn dữ liệu dashboard và kiểm tra thay đổi điều khiển motor. Port màn hình 7B không tự giải quyết các điểm này.

| Phần | Mức tương thích theo source | Việc cần làm |
| --- | --- | --- |
| Telemetry VESC thông thường | Có thể giữ giao thức hiện tại | Kiểm tra cùng scheduler mới và phần cứng CAN 7B. |
| Cruise/profile trên dashboard | Trùng tên biến nhưng không bảo đảm nhận đủ | Dùng gói DASH riêng, không phụ thuộc poll-all. |
| Settings panel hai chiều | Chưa có trong firmware hiện tại | Thêm REQ_UI, ACTION, REQ_STATE, parser và UI. |
| CAN phân mảnh | Có nền tảng, chưa đủ điều phối | Tuần tự hóa giao dịch và kiểm tra chiều dài gói. |
| UI LVGL | Có thể tái sử dụng hàm hiển thị | Chuyển cập nhật sang task UI, thích nghi layout 7B. |
| Hành vi ga/phanh/cruise | Thay đổi đáng kể | Xem xét riêng motor-control-loop trước khi áp dụng. |
| Firmware VESC | Chưa xác nhận | Cần phiên bản và khả năng hỗ trợ các API của script. |
| Lưu setting | Chỉ một phần trong script mới | Âm lượng được lưu EEPROM; profile mặc định về 0 khi chạy script. |

## 1. Luồng hiện tại và khoảng trống

Trong `src/main.cpp`, loop gửi polling RT và Lisp; callback CAN gọi parser RT, parser `COMM_LISP_GET_STATS`, rồi chuyển phản hồi sang BLE.

`src/vesc_lisp_poll.cpp` đọc `cruise-active`, `cruise-rpm`, `rpm-per-ms`, `current-profile`, sau đó gọi trực tiếp các hàm LVGL trong `custom.c`. Không có parser `UI_DESC`, `STATE`, `DASH` hay hàng đợi action cho panel mới. Việc đã định nghĩa `COMM_CUSTOM_APP_DATA = 36` trong `datatypes.h` chưa đồng nghĩa hỗ trợ panel.

Script mới giữ tên các biến dashboard, nhưng có nhiều global hơn. Header upstream nêu giới hạn monitor firmware là 18 biến; đây là nhận định trong source tham khảo, chưa xác minh trên firmware của người dùng. Parser hiện tại cũng chỉ đọc tối đa 32 biến dù struct có 128 chỗ. Tăng mảng ESP32 không khắc phục giới hạn phía VESC. Khi thiếu biến, code hiện tại chủ yếu ghi log, có thể để lại giá trị cũ trên UI.

## 2. Luồng đích cần tích hợp

```text
Khởi động / đổi target VESC / reconnect
  -> xóa model cũ và trạng thái pending
  -> REQ_UI -> Lisp panel-send-ui -> UI_DESC -> dựng controls trên task UI

Người dùng thao tác
  -> queue {control_id, value}
  -> CAN scheduler gửi ACTION
  -> event-data-rx -> panel-handle -> panel-action
  -> Lisp trả STATE -> cập nhật model -> task UI hiển thị giá trị xác nhận

Panel mở: REQ_STATE theo chu kỳ khoảng 200 ms
Dashboard: REQ_DASH theo chu kỳ khoảng 200 ms, độc lập panel mở/đóng
Mất phản hồi: đánh dấu stale/unavailable, không coi giá trị cũ là xác nhận mới
```

Những bước xóa model, stale và pending ở trên là yêu cầu cho bản tích hợp, không phải xác nhận upstream đã xử lý đầy đủ.

Payload yêu cầu: `[36][0x56][0x50][message][reply_can_id]...`. ACTION thêm control ID và số nguyên big-endian `value × 1000`. Phản hồi có command 36 và magic `VP`; gói DASH = `0x84` chứa bốn số int32 ×1000: cruise active, cruise RPM, profile, RPM-per-m/s. Không dùng `float32_auto` để giải mã các giá trị này.

Reply ID phải là CAN ID thực tế của màn hình, khác target VESC ID. Code hiện tại mặc định controller ID 255 trong khi setter giới hạn 1–254; cần thống nhất ID node riêng hợp lệ và kiểm tra explicit reply routing. Header upstream ghi `(send-data buf 2 reply-id)` cần FW 6.05+; chưa đủ để kết luận toàn bộ script chạy được trên mọi FW từ 6.05.

Các action hiện có: ID 1 bật/tắt throttle, ID 4 beep, ID 5 âm lượng, ID 10/11/12 chọn profile. Tốc độ 25/40/60 km/h và hệ số dòng 0.5/0.67/1.0 vẫn ghi cứng. Chưa có action sửa tùy ý tốc độ/dòng từng profile.

## 3. Các điểm phải sửa ở transport và UI

- **Khóa theo giao dịch:** `comm_can_transmit_eid` hiện khóa từng CAN frame; `comm_can_send_buffer` không giữ khóa cho cả chuỗi phân mảnh. Polling và BLE có thể xen frame. Cần điều phối toàn bộ request/reply, timeout và upload; chỉ thêm mutex từng frame không đủ.
- **Reassembly:** buffer hiện được tìm theo ID đích trong CAN frame. Các phản hồi về cùng màn hình có thể dùng chung buffer. Cần tránh chồng phản hồi; không coi nhiều buffer là đã tách được mọi giao dịch.
- **Kiểm tra biên:** nhánh FILL_RX_BUFFER ngắn đang memcpy mà không kiểm tra `offset + len` như nhánh LONG; các nhánh cần kiểm tra DLC tối thiểu trước khi đọc. Với panel, kiểm tra count, version, ID, chuỗi, miền giá trị và độ dài từng record.
- **Ngữ cảnh LVGL:** RX chạy trong `can_rx`, nhưng Lisp parser hiện gọi LVGL trực tiếp trong khi main loop chạy `Lvgl_Loop`. Chuyển RX sang cập nhật model có khóa/queue; chỉ task UI thao tác widget. Không dựng panel lớn trên stack RX hiện chỉ 2048 byte.
- **Phối hợp BLE:** callback đang chuyển các phản hồi sang BLE. Cần xác định cách xử lý phản hồi polling nội bộ, request từ VESC Tool và tạm dừng polling khi truyền script; bảo toàn chức năng bridge.
- **Nguồn dashboard:** sau khi DASH hoạt động, ngừng để GET_STATS cập nhật cùng các trường cruise/profile. Nếu giữ tương thích script cũ, chọn mode rõ ràng và fallback có timeout.
- **Đổi target/restart Lisp:** hủy action đang chờ, lấy lại descriptor, không hiển thị model VESC cũ. Cần timeout, trạng thái đang chờ và báo thao tác chưa xác nhận. Không tự retry vô hạn action không idempotent như beep/toggle.

Các khả năng lỗi xen frame/race là rủi ro suy ra từ source, chưa phải lỗi đã tái hiện trên phần cứng.

## 4. Thay đổi điều khiển motor trong script mới

Nguồn: `lisp/main.lisp` upstream, các hàm `apply-profile`, `activate-cruise-control`, `throttle-out`, `brake-out`, `cruise-out`, `motor-control-loop`.

| Hành vi | Script hiện tại | Script mới |
| --- | --- | --- |
| Cruise | Dùng `set-rpm`, tắt output app khi cruise | PI tự tính dòng, gọi `set-current`; gains ghi cứng 0.02/0.05 |
| Ga/phanh thường | Chủ yếu do app ADC xử lý ngoài cruise | Lisp đọc ADC và trực tiếp phát lệnh dòng/phanh, tick mục tiêu 10 ms |
| Output app ADC | Tắt vô hạn khi cruise, bật lại khi hủy | Liên tục gia hạn `app-disable-output 1500` |
| Profile | Đặt max-speed | Đặt max-speed và current-max-scale |
| Hủy cruise bởi ga | Phát hiện chuyển từ nhả sang nhấn | Hủy khi giá trị ga đang vượt 0.05 |

Các điểm cần quyết định/kiểm tra:

1. **Master-off đứng trước brake.** Khi `throttle-on = 0`, script gọi `set-current 0` và không vào nhánh `brake-out`. Nếu yêu cầu là “tắt ga nhưng vẫn giữ phanh điện”, hành vi hiện tại chưa đáp ứng.
2. **Ga đang giữ khi bật cruise:** motor loop mới hủy cruise ngay khi ga >0.05; khác hành vi cũ cho phép ga đã giữ lúc kích hoạt. Cần chốt trải nghiệm mong muốn.
3. **Khôi phục app ADC sau khi script dừng:** lệnh disable được gia hạn 1500 ms; khi hết hạn, app ADC có thể lấy lại quyền điều khiển. Không coi mất script là trạng thái dừng khóa cố định. Phải kiểm tra thực tế timeout, ga đang giữ, phanh và stop/restart Lisp.
4. **Scale dòng:** sau profile Slow/Medium, giới hạn scale vẫn là cấu hình đã áp dụng cho đến khi được thay đổi. Cần xác định cách khôi phục khi dừng script/chuyển sang chế độ cũ; script không có bước cleanup tổng quát được thấy trong phần đã đọc.
5. **PI cruise:** không kế thừa gains speed PID của VESC Tool; cần kiểm tra đáp ứng riêng với motor/xe. Không suy ra hành vi tương đương chỉ vì tên biến cruise không đổi.
6. **PAS:** script mới có handler nhận dòng PAS và kiểm tra độ mới 0.4 s. Nếu dự án chưa dùng PAS thì không cần port phần gửi PAS từ P4; cần xác định rõ chức năng nào được bật.

## 5. Các điểm cần gia cố ngay trong Lisp mới

- `panel-handle` chỉ kiểm tra tối thiểu 4 byte trước khi phân nhánh, nhưng ACTION đọc đến byte 8 và PAS đọc đến byte 7. Cần kiểm tra riêng ACTION >=9 byte và PAS >=8 byte (không tính byte command 36 đã được firmware bóc).
- ID 5 gán trực tiếp `beep-vol = to-i32(val)` và đánh dấu lưu. Cần kiểm tra/clamp 0–50 tại Lisp, không chỉ dựa vào giới hạn control trên màn hình.
- EEPROM slot 0 dùng cho âm lượng: đối chiếu dữ liệu ứng dụng đang có để tránh dùng trùng. Profile vẫn được khởi tạo về 0; không hứa lưu mọi setting qua reboot.
- Script dùng `@const-start/@const-end`, `conf-get` cho ADC ramp, `throttle-curve`, `set-current-rel`, `set-brake-rel` và send-data routing. Phải kiểm tra bằng đúng firmware/công cụ nạp; không chỉ kiểm tra được một lệnh REPL rồi kết luận toàn bộ tương thích.

## 6. Hướng tích hợp đề xuất

**Giai đoạn đầu:** port giao thức panel/DASH và các action cần thiết vào script hiện tại, giữ cách điều khiển motor hiện có. Việc này giúp kiểm tra màn 7B và settings mà không đồng thời đổi toàn bộ hành vi ga/phanh/cruise.

**Giai đoạn sau, nếu chọn dùng toàn bộ script mới:** xử lý các khác biệt ở mục 4, kiểm tra API theo firmware thật và kiểm tra điều khiển motor riêng. Đây là đề xuất kỹ thuật; chưa có thay đổi source hoặc upload trong lần review này.

Các bước kiểm chứng:

- [ ] Xác nhận phiên bản firmware VESC, app ADC, calibration và motor-command timeout.
- [ ] Kiểm tra encode/decode UI_DESC/STATE/DASH với gói đúng, thiếu byte, sai version, count quá giới hạn và giá trị ngoài miền.
- [ ] Kiểm tra action round-trip, chọn lại profile đang bật, beep, lưu âm lượng, reconnect và restart script.
- [ ] Kiểm tra CAN polling cùng BLE bridge, timeout, đổi target và truyền script.
- [ ] Kiểm tra chỉ task UI cập nhật LVGL; dữ liệu stale không hiện như trạng thái mới.
- [ ] Nếu đổi motor loop: kiểm tra ga/phanh/cruise, master-off, chuyển profile, script dừng và khả năng app ADC lấy lại quyền điều khiển trên bố trí thử nghiệm phù hợp trước khi vận hành xe.

**Chưa thể chốt tương thích runtime** cho đến khi có phiên bản firmware VESC và kết quả các kiểm tra liên quan trên thiết bị.
