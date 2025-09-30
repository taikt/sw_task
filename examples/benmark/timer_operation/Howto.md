## plot result
python3 -m venv venv
source venv/bin/activate
pip install matplotlib pandas
# Normal mode
./monitor_test.sh ../build/timer_load 1000 50 30  
(1000 one-shot timers, 50 periodic timers, test trong 30 sec
do cpu,memory moi 0.5 sec)

# Stress mode
./monitor_test.sh ../build/timer_load 1000 50 30 stress 

python3.13 plot_results.py result_20250913_233211/

./monitor_test.sh ../../../../tiger_looper/build/tiger_timer_load 1000 50 30

python3.13 plot_results.py result_20250913_233615/

mv result_20250913_233615/ tiger_result_20250913_233615/

python3.13 compare.py



 2049  ./monitor_test.sh ../../../../tiger_looper/benmark/build/tiger_timer_load_light 1000 50 30
 2050  ./monitor_test.sh ../build/timer_load_light 1000 50 30
 2051  python3.13 compare_simple.py 
 2052  ls
 2053  ./monitor_test.sh ../build/timer_load_light 50 10 30
 2054  ./monitor_test.sh ../../../../tiger_looper/benmark/build/tiger_timer_load_light 50 10 30
 2055  python3.13 compare_simple.py



### Note
CPU % có thể >100% nếu chương trình sử dụng nhiều hơn 1 core.

100% CPU nghĩa là sử dụng toàn bộ 1 core.
200% CPU nghĩa là sử dụng toàn bộ 2 core.
130% CPU nghĩa là sử dụng 1.3 core (ví dụ: 4 threads chạy song song, mỗi thread chiếm ~30-35% một core).
Lý do: lệnh ps và top trên Linux tính tổng %CPU của tất cả các threads/processes, nên nếu ứng dụng đa luồng hoặc event loop chạy nhiều thread, tổng %CPU sẽ vượt 100%.

Kết luận: Nếu máy có nhiều core, giá trị CPU % có thể vượt 100% là hoàn toàn bình thường!

# Số core CPU
nproc

# Thông tin chi tiết CPU
lscpu

# Tổng bộ nhớ RAM
free -h

# Thông tin phần cứng tổng quát
cat /proc/cpuinfo
cat /proc/meminfo

# Thông tin hệ thống tổng hợp
uname -a

### thong tin he thong
Thông tin hệ thống
OS: Ubuntu 20.04, Kernel 5.15.0-60-generic (x86_64)
CPU: Intel(R) Xeon(R) Gold 5218 @ 2.30GHz
Số CPU logic: 64 (nproc)
Số core vật lý: 32 (16 core mỗi socket x 2 socket)
Số luồng mỗi core: 2 (Hyperthreading)
Tốc độ CPU: 1000 MHz (min) ~ 3900 MHz (max)
Cache: L1d 1MiB, L1i 1MiB, L2 32MiB, L3 44MiB
NUMA nodes: 2
RAM: 20 GiB
Đang dùng: 7.6 GiB
Còn trống: 7.4 GiB
Available: 12 GiB
Swap: 1.5 GiB

Máy test có 64 CPU logic, 32 core vật lý, 2 socket, RAM 20GB, CPU Xeon Gold 5218, Ubuntu 20.04, kernel 5.15.