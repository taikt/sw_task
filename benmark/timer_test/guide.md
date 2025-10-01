# build image
cmake -DENABLE_VALGRIND=ON ..
[cmake -DCMAKE_BUILD_TYPE=Release .. ] => no debug
make 

# test heap
timeout 60s valgrind --tool=massif \
    --time-unit=ms \
    --detailed-freq=50 \
    --max-snapshots=50 \
    --massif-out-file=sw_task.massif \
    ./sw_task_oneshot

timeout 60s valgrind --tool=massif \
    --time-unit=ms \
    --detailed-freq=50 \
    --max-snapshots=50 \
    --massif-out-file=tiger.massif \
    ./tiger_oneshot

python3 plot_massif.py sw_task.massif "SW Task" tiger.massif "Tiger Looper"

## kiem tra mem leak
valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all --track-origins=yes ./sw_task_oneshot
(ctr+c) de dung xem ket qua


## test RSS (tong bo nho)
./monitor_rss.sh ./sw_task_oneshot sw_task_rss_log.txt
./monitor_rss.sh ./tiger_oneshot tiger_rss_log.txt
python3 plot_rss.py sw_task_rss_log.txt "SW Task" tiger_rss_log.txt "Tiger Looper"

# test cpu

# binary run
# Cấu trúc tham số mới
./sw_task_oneshot [timer_count] [duration_sec] [max_timeout_sec] [stress]

# Ví dụ
./sw_task_oneshot 100 60 40        # 100 timers, 60s, timeout 2-40s
./sw_task_oneshot 100 60           # 100 timers, 60s, timeout 2-30s (default)
./sw_task_oneshot 50 120 15 stress # 50 timers, 2min, timeout 2-15s, stress mode

./cpu_checker.sh sw_task ./sw_task_oneshot 100 60 15 stress
./cpu_checker.sh tiger ./tiger_oneshot 100 60 15 normal

./cpu_checker.sh sw_task ./sw_task_oneshot 100 60 15 stress
                        │       │                │   │  │  │
                        │       │                │   │  │  └─ STRESS mode
                        │       │                │   │  └──── max_timeout = 15s
                        │       │                │   └─────── duration = 60s quan sát
                        │       │                └─────────── 100 timers
                        │       └──────────────────────────── chương trình SW Task
                        └────────────────────────────────────── framework SW Task

./cpu_checker.sh tiger ./tiger_oneshot 100 60 15 normal
                        │     │              │   │  │  │
                        │     │              │   │  │  └─ NORMAL mode
                        │     │              │   │  └──── max_timeout = 15s
                        │     │              │   └─────── duration = 60s quan sát
                        │     │              └─────────── 100 timers
                        │     └────────────────────────── chương trình Tiger
                        └──────────────────────────────── framework Tiger