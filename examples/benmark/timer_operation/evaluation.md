### 1. So sánh kết quả Tiger Looper và SW Task

- **CPU Usage (%):**  
  SW Task có mức sử dụng CPU ổn định hơn, dao động quanh 20-40%. Tiger Looper có nhiều biến động hơn, đặc biệt có một điểm tăng vọt cuối cùng (~90%).  
  → SW Task ổn định, Tiger Looper có thể có spike hoặc workload không đều.

- **Memory Usage (MB):**  
  SW Task giữ mức bộ nhớ ổn định (~4MB), Tiger Looper tăng dần theo thời gian, lên đến ~5.6MB.  
  → SW Task tiết kiệm bộ nhớ hơn, Tiger Looper có xu hướng tăng bộ nhớ.

- **Thread Count:**  
  SW Task giữ số thread ổn định (4), Tiger Looper chủ yếu là 3 thread, cuối cùng tăng lên 4.  
  → SW Task dùng nhiều thread hơn, Tiger Looper có thể tối ưu hơn về thread.

- **System Load (1min avg):**  
  SW Task có system load thấp hơn (~0.2-0.3), Tiger Looper cao hơn (~0.3-0.5).  
  → SW Task gây ít áp lực lên hệ thống hơn.

### 2. System Load là gì? Ảnh hưởng thế nào?

- **System Load** là chỉ số trung bình số lượng tiến trình đang chạy hoặc chờ CPU trong hệ thống (trong 1 phút, 5 phút, 15 phút).
- **Ý nghĩa:**  
  - Giá trị load = 1 nghĩa là CPU luôn bận (trên hệ thống 1 core).
  - Nếu load > số core, hệ thống bắt đầu quá tải, các tiến trình phải chờ lâu hơn.
- **Ảnh hưởng:**  
  - Load cao → hệ thống xử lý chậm, các tiến trình bị delay.
  - Load thấp → hệ thống còn dư tài nguyên, xử lý mượt mà.

**Kết luận:**  
SW Task có hiệu năng tốt hơn về CPU, memory và system load. Tiger Looper có thể cần tối ưu lại để giảm memory và load. System load càng thấp thì hệ thống càng khỏe, xử lý nhanh hơn.