#include "SLLooper.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <thread>
#include <atomic>
#include <signal.h>
#include <cmath>
#include <string>
#include <map>
#include <numeric>
#include <algorithm>
#include <cstring>

using namespace std::chrono_literals;
using namespace swt;

class TimerLoadTest {
private:
    std::shared_ptr<SLLooper> looper;
    std::vector<Timer> timers;
    std::atomic<uint64_t> timer_fires{0};
    std::atomic<bool> running{true};
    bool stress_mode = false;
    bool quiet = false;
    bool show_progress = true;

public:
    void set_stress_mode(bool enable) { stress_mode = enable; }
    void set_quiet(bool q) { quiet = q; }
    void set_show_progress(bool v) { show_progress = v; }

    // Heavy CPU work function to create measurable load
    void heavy_cpu_work(int timer_id, int base_iterations = 8000) {
        volatile double result = 0.0;
        int iterations = base_iterations;

        if (stress_mode) {
            iterations *= 3; // 3x more work in stress mode
        }

        for (int i = 0; i < iterations; ++i) {
            result += std::sin(i * 0.001) * std::cos(i * 0.002);
            result += std::sqrt(i + 1);
            if (i % 50 == 0) {
                result += std::pow(i, 1.1) + std::log(i + 1);
            }
        }

        std::vector<double> temp_data(50 + (timer_id % 100));
        for (size_t j = 0; j < temp_data.size(); ++j) {
            temp_data[j] = result + j * 0.1;
            result += temp_data[j] * 0.001;
        }

        std::string work_str = "timer_" + std::to_string(timer_id);
        for (int k = 0; k < 50; ++k) {
            work_str += std::to_string(k * result);
            if (work_str.length() > 500) {
                work_str = "reset_" + std::to_string(timer_id);
            }
        }

        volatile size_t hash = std::hash<std::string>{}(work_str);
        result += hash * 0.00001;
    }

    void create_one_shot_timers(int count) {
        if (!quiet) {
            std::cout << "Creating " << count << " one-shot timers..." << std::endl;
        }
        for (int i = 0; i < count; ++i) {
            auto timer = looper->addTimer([this, i]() {
                timer_fires++;
                heavy_cpu_work(i, 10000);
                if (i % 5 == 0) {
                    std::vector<int> data(200);
                    std::iota(data.begin(), data.end(), i);
                    volatile int sum = std::accumulate(data.begin(), data.end(), 0);
                    std::sort(data.rbegin(), data.rend());
                    sum += data[0];
                }
            }, 800 + (i % 4000)); // 0.8-4.8 seconds spread

            timers.push_back(std::move(timer));

            if (show_progress && !quiet && (i + 1) % 100 == 0) {
                std::cout << "Created " << (i + 1) << " timers" << std::endl;
            }
        }
    }

    void create_periodic_timers(int count) {
        if (!quiet) {
            std::cout << "Creating " << count << " periodic timers..." << std::endl;
        }
        for (int i = 0; i < count; ++i) {
            auto timer = looper->addPeriodicTimer([this, i]() {
                timer_fires++;
                int work_multiplier = stress_mode ? 2 : 1;
                heavy_cpu_work(i, 6000 * work_multiplier);

                std::map<int, double> temp_map;
                for (int j = 0; j < 20; ++j) {
                    temp_map[j] = j * std::sin(i + j);
                }
                double sum = 0.0;
                for (const auto& pair : temp_map) {
                    sum += pair.second * pair.second;
                }
                std::string data_str = "data_" + std::to_string(i) + "_" + std::to_string(sum);
                volatile size_t str_hash = std::hash<std::string>{}(data_str);

                if (i % 3 == 0) {
                    volatile double matrix_sum = 0.0;
                    for (int row = 0; row < 10; ++row) {
                        for (int col = 0; col < 10; ++col) {
                            matrix_sum += std::sin(row) * std::cos(col) + (row * col);
                        }
                    }
                }
            }, 80 + (i % 400)); // 80ms to 480ms intervals

            timers.push_back(std::move(timer));

            if (show_progress && !quiet && (i + 1) % 50 == 0) {
                std::cout << "Created " << (i + 1) << " periodic timers" << std::endl;
            }
        }
    }

    void run_test(int one_shot_count, int periodic_count, int duration_seconds) {
        if (!quiet) {
            std::cout << "\n=== Timer Load Test ===" << std::endl;
            std::cout << "One-shot timers : " << one_shot_count << std::endl;
            std::cout << "Periodic timers : " << periodic_count << std::endl;
            std::cout << "Duration (sec)  : " << duration_seconds << std::endl;
            std::cout << "Stress mode     : " << (stress_mode ? "ENABLED" : "disabled") << std::endl;
            std::cout << "PID             : " << getpid() << " (use this for monitoring)" << std::endl;
        }

        looper = std::make_shared<SLLooper>();
        std::this_thread::sleep_for(100ms);

        if (!quiet) {
            std::cout << "\nEvent loop started. Creating timers..." << std::endl;
        }
        create_one_shot_timers(one_shot_count);
        create_periodic_timers(periodic_count);

        if (!quiet) {
            std::cout << "\nAll timers created!" << std::endl;
            std::cout << "Active timers: " << looper->getActiveTimerCount() << std::endl;
            std::cout << "Expected CPU load: " << (stress_mode ? "HIGH" : "MEDIUM") << std::endl;
            std::cout << "Starting monitoring phase..." << std::endl;
        }

        auto start_time = std::chrono::steady_clock::now();
        for (int i = 0; i < duration_seconds; ++i) {
            std::this_thread::sleep_for(1s);
            if (!quiet) {
                auto current_time = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                                    current_time - start_time).count();
                std::cout << "[" << elapsed << "s] "
                          << "Timer fires: " << timer_fires.load()
                          << ", Active: " << looper->getActiveTimerCount()
                          << ", Rate: " << timer_fires.load() / (elapsed + 1) << " fires/sec"
                          << std::endl;
            }
        }

        if (!quiet) {
            std::cout << "\nTest completed. Cleaning up..." << std::endl;
        }
        timers.clear();
        looper->exit();

        if (!quiet) {
            std::cout << "Final stats:" << std::endl;
            std::cout << "Total timer fires: " << timer_fires.load() << std::endl;
            std::cout << "Average rate     : " << timer_fires.load() / duration_seconds << " fires/sec" << std::endl;
            std::cout << "Test completed successfully!" << std::endl;
        }
    }
};

// ------------------ Help / CLI ------------------

static void print_help(const char* prog) {
    std::cout
        << "Timer Load Test Utility\n"
        << "Usage:\n"
        << "  " << prog << " [one_shot_count] [periodic_count] [duration_seconds] [stress]\n"
        << "  " << prog << " [options] [positional arguments]\n\n"
        << "Positional arguments (all optional; defaults shown):\n"
        << "  one_shot_count      Number of one-shot timers to create (default 1000)\n"
        << "  periodic_count      Number of periodic timers to create (default 100)\n"
        << "  duration_seconds    Run duration before shutdown (default 60)\n"
        << "  stress              Literal word 'stress' to enable stress mode\n\n"
        << "Options:\n"
        << "  -h, --h, --help     Show this help and exit\n"
        << "  --stress            Enable stress mode (same as adding positional 'stress')\n"
        << "  --quiet             Suppress per-second and creation progress logs\n"
        << "  --no-progress       Disable intermediate creation progress messages\n"
        << "  --seed <N>          (Reserved) Provide a seed value for future deterministic tests\n\n"
        << "Behavior:\n"
        << "  Creates a large set of timers (one-shot + periodic) to generate load.\n"
        << "  Each timer callback performs mixed CPU + memory + algorithmic work.\n"
        << "  Stress mode multiplies the work size to amplify CPU usage.\n\n"
        << "Monitoring suggestions:\n"
        << "  Basic RSS / CPU:   /usr/bin/time -v " << prog << "\n"
        << "  Perf profiling:    perf record -g -- " << prog << " 500 50 20\n"
        << "  Heap profiling:    valgrind --tool=massif " << prog << " 500 50 20\n"
        << "  /proc sampling:    python3 monitor_proc.py --cmd " << prog << " 500 50 20 -d 30 -i 0.1 -o out.json\n"
        << "  Comparison:        python3 compare_proc.py sw_monitor_proc.json tiger_monitor_proc.json --plot\n\n"
        << "Examples:\n"
        << "  " << prog << "                # Run default 1000/100/60 normal load\n"
        << "  " << prog << " 800 80 30      # Smaller test for 30s\n"
        << "  " << prog << " 1200 200 45 stress   # Heavier run in stress mode\n"
        << "  " << prog << " --stress 500 100 20  # Stress via flag + custom counts\n"
        << "  " << prog << " --quiet 1000 100 30  # Minimal console output\n\n"
        << "Exit codes:\n"
        << "  0 on success; non-zero on internal failure.\n\n"
#ifdef TIMER_USE_SIGEV_THREAD
        << "Backend: SIGEV_THREAD (Tiger style)\n"
#elif defined(TIMER_USE_TIMERFD_EPOLL)
        << "Backend: timerfd+epoll (SW Task style)\n"
#else
        << "Backend: (default / unspecified build)\n"
#endif
        << std::endl;
}

void signal_handler(int signal) {
    std::cout << "\nReceived signal " << signal << ". Exiting gracefully..." << std::endl;
    exit(0);
}

int main(int argc, char* argv[]) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Defaults
    int one_shot_count = 1000;
    int periodic_count = 100;
    int duration = 60;
    bool stress_mode = false;
    bool quiet = false;
    bool show_progress = true;
    long seed = 0;
    bool seed_set = false;

    // First pass: detect help quickly
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-h") == 0 || std::strcmp(argv[i], "--h") == 0 ||
            std::strcmp(argv[i], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        }
    }

    // Parse tokens
    std::vector<std::string> positionals;
    for (int i = 1; i < argc; ++i) {
        std::string tok = argv[i];
        if (tok == "-h" || tok == "--h" || tok == "--help") {
            print_help(argv[0]);
            return 0;
        } else if (tok == "--stress") {
            stress_mode = true;
        } else if (tok == "--quiet") {
            quiet = true;
        } else if (tok == "--no-progress") {
            show_progress = false;
        } else if (tok == "--seed") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value after --seed\n";
                return 1;
            }
            seed = std::stol(argv[++i]);
            seed_set = true;
        } else if (!tok.empty() && tok[0] == '-') {
            std::cerr << "Unknown option: " << tok << "\n";
            print_help(argv[0]);
            return 1;
        } else {
            positionals.push_back(tok);
        }
    }

    // Positional overrides
    if (positionals.size() >= 1) one_shot_count = std::atoi(positionals[0].c_str());
    if (positionals.size() >= 2) periodic_count = std::atoi(positionals[1].c_str());
    if (positionals.size() >= 3) duration = std::atoi(positionals[2].c_str());
    if (positionals.size() >= 4) {
        if (positionals[3] == "stress") stress_mode = true;
    }

    if (seed_set && !quiet) {
        std::cout << "Seed (currently unused for deterministic logic): " << seed << std::endl;
    }

    if (!quiet) {
        std::cout << "Timer Load Test - PID: " << getpid() << std::endl;
        std::cout << "Use --help for more details.\n";
    }

    TimerLoadTest test;
    test.set_stress_mode(stress_mode);
    test.set_quiet(quiet);
    test.set_show_progress(show_progress);
    test.run_test(one_shot_count, periodic_count, duration);
    return 0;
}