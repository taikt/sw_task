#include "SLLooper.h"
#include "TimerManager.h"
#include "PureOneShotTest.h"
#include <random>

class TigerPureOneShotTest : public PureOneShotTestBase {
private:
    std::shared_ptr<SLLooper> looper;
    std::shared_ptr<Handler> handler;
    std::unique_ptr<TimerManager> timerManager;
    std::vector<timer_t> active_timers;
    std::thread looper_thread;
    std::mutex timers_mutex;
    int next_timer_id = 0;
    
    // ✅ Random number generators
    std::random_device rd;
    std::mt19937 gen;
    
    // ✅ Memory management counters
    size_t cleanup_counter = 0;
    
    // ✅ NEW: Configurable timeout range
    int max_timeout_sec = 30; // Default 30 seconds
    
    // ✅ Handler class để xử lý timer messages
    class TigerTimerHandler : public Handler {
    private:
        TigerPureOneShotTest* mTest;
        
    public:
        TigerTimerHandler(std::shared_ptr<SLLooper>& looper, TigerPureOneShotTest* test) 
            : Handler(looper), mTest(test) {}
        
        void handleMessage(const std::shared_ptr<Message>& msg) override {
            if (mTest) {
                mTest->handleTimerMessage(msg);
            }
        }
    };
    
public:
    TigerPureOneShotTest(int timeout_max = 30) : gen(rd()), max_timeout_sec(timeout_max) {
        // Initialize Tiger Looper framework
        looper = std::make_shared<SLLooper>();
        
        // Create handler for timer messages
        handler = std::make_shared<TigerTimerHandler>(looper, this);
        
        // Create timer manager
        timerManager = std::make_unique<TimerManager>(handler);
        
        // Start event loop in separate thread
        looper_thread = std::thread([this]() {
            looper->loop();
        });
        
        // Wait for loop to start
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        std::cout << "✅ Tiger Looper framework initialized (NO REGENERATION)" << std::endl;
        std::cout << "📊 Timer timeout range: 2-" << max_timeout_sec << " seconds" << std::endl;
    }
    
    ~TigerPureOneShotTest() {
        cleanup_framework();
    }
    
    // ✅ Handle timer messages from TimerManager
    void handleTimerMessage(const std::shared_ptr<Message>& msg) {
        timer_fires++;
        timers_completed++;
        
        int timer_id = msg->what - 1000; // Base message ID
        
        // Log when timer fires
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - test_start_time).count();
        
        std::cout << "🔥 [" << elapsed << "s] Tiger Timer " << timer_id 
                 << " FIRED (Message ID: " << msg->what << ")" << std::endl;
        
        // Perform work
        light_cpu_work(timer_id, stress_mode ? 1500 : 1000);
        
        // ✅ REMOVED: No more regeneration/replacement logic
    }
    
    // PureOneShotTestBase implementation
    size_t get_active_timer_count() override {
        std::lock_guard<std::mutex> lock(timers_mutex);
        return active_timers.size();
    }
    
    void create_oneshot_timer_batch(int batch_size, int base_delay_ms) override {
        std::lock_guard<std::mutex> lock(timers_mutex);
        
        // ✅ Clean up BEFORE creating new timers
        cleanup_completed_timers_internal();
        
        // ✅ FIXED: Configurable timeout distribution (always 2 to max_timeout_sec)
        std::uniform_int_distribution<> timeout_dist(2000, max_timeout_sec * 1000);
        std::string timeout_range = "2-" + std::to_string(max_timeout_sec) + "s";
        
        std::cout << "  🎲 Creating " << batch_size << " timers with random timeouts (" 
                  << timeout_range << "), current active: " << active_timers.size() << std::endl;
        
        for (int i = 0; i < batch_size; ++i) {
            int timer_id = ++next_timer_id;
            int message_id = 1000 + timer_id; // Base message ID
            
            // ✅ Random timeout using configurable range
            int delay_ms = timeout_dist(gen);
            
            std::cout << "    Tiger Timer " << timer_id << " (msg:" << message_id << ") -> " 
                      << std::fixed << std::setprecision(1) << (delay_ms/1000.0) << "s" << std::endl;
            
            // ✅ Create one-shot timer using TimerManager
            timer_t posix_timer = timerManager->startTimer(message_id, delay_ms);
            
            if (posix_timer != (timer_t)0) {
                active_timers.push_back(posix_timer);
                timers_created++;
            }
        }
        
        std::cout << "  ✅ Created " << batch_size << " one-shot timers, total active: " 
                  << active_timers.size() << std::endl;
        
        // ✅ Cleanup after batch creation
        cleanup_completed_timers_internal();
    }
    
    void cleanup_framework() override {
        running.store(false);
        
        {
            std::lock_guard<std::mutex> lock(timers_mutex);
            std::cout << "🧹 Final cleanup: cancelling " << active_timers.size() << " timers" << std::endl;
            
            // ✅ Stop all active timers
            for (auto timer_id : active_timers) {
                timerManager->stopTimer(timer_id);
            }
            
            active_timers.clear(); // Clear vector
            active_timers.shrink_to_fit(); // Free memory
        }
        
        if (looper) {
            looper->exit();
        }
        if (looper_thread.joinable()) {
            looper_thread.join();
        }
        
        std::cout << "✅ Tiger Looper framework cleaned up (memory freed)" << std::endl;
    }
    
    std::string get_framework_name() override {
        return "Tiger_Looper";
    }
    
    // ✅ OVERRIDE: Disable file export completely
    void export_detailed_results(const std::string& framework_name) {
        // Do nothing - no file export
        std::cout << "📊 File export disabled for CPU monitoring test" << std::endl;
    }
    
private:
    void cleanup_completed_timers() {
        std::lock_guard<std::mutex> lock(timers_mutex);
        cleanup_completed_timers_internal();
    }
    
    // ✅ Internal cleanup method (assumes lock is held)
    void cleanup_completed_timers_internal() {
        cleanup_counter++;
        
        size_t before = active_timers.size();
        
        // ✅ Tiger Looper's TimerManager automatically cleans up completed one-shot timers
        // But we can remove them from our tracking vector periodically
        if (before > 1000) { // Prevent vector from growing too large
            std::cout << "    🧹 Trimming timer tracking vector (size: " << before << ")" << std::endl;
            active_timers.erase(active_timers.begin(), active_timers.begin() + before/2);
        }
        
        // ✅ Shrink vector capacity periodically
        if (cleanup_counter % 10 == 0) { // Every 10th cleanup
            if (active_timers.capacity() > active_timers.size() * 2 && 
                active_timers.capacity() > 50) {
                size_t old_capacity = active_timers.capacity();
                active_timers.shrink_to_fit();
                std::cout << "    💾 Shrunk vector capacity: " << old_capacity 
                         << " -> " << active_timers.capacity() << std::endl;
            }
        }
    }
};

void signal_handler(int signal) {
    std::cout << "\nReceived signal " << signal << ". Exiting gracefully..." << std::endl;
    exit(0);
}

int main(int argc, char* argv[]) {
    // Setup signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // ✅ NEW: Updated parameter structure (same as SW Task)
    int initial_timer_count = 100;
    int duration = 60;
    int max_timeout = 30;           // NEW: Configurable timeout range
    bool stress_mode = false;
    
    if (argc >= 2) initial_timer_count = std::atoi(argv[1]);
    if (argc >= 3) duration = std::atoi(argv[2]);
    if (argc >= 4) max_timeout = std::atoi(argv[3]);     // NEW: max timeout parameter
    if (argc >= 5) stress_mode = (std::string(argv[4]) == "stress");
    
    // ✅ Ensure minimum timeout is 2 seconds
    if (max_timeout < 2) {
        max_timeout = 2;
        std::cout << "⚠️  Warning: max_timeout adjusted to minimum value of 2 seconds" << std::endl;
    }
    
    std::cout << "Tiger Looper Pure One-Shot Timer Test (NO REGENERATION, NO FILE EXPORT)" << std::endl;
    std::cout << "Usage: " << argv[0] << " [timer_count] [duration_sec] [max_timeout_sec] [stress]" << std::endl;
    std::cout << "Timer Behavior:" << std::endl;
    std::cout << "  - One-shot timers: Random timeouts 2-" << max_timeout << " seconds" << std::endl;
    std::cout << "  - NO regeneration: Timers fire once and stop" << std::endl;
    std::cout << "  - NO file export: Only CPU monitoring files created" << std::endl;
    std::cout << "  - Memory Management: Automatic cleanup of completed timers" << std::endl;
    std::cout << "  - Architecture: POSIX timers -> Handler messages -> Event loop" << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << argv[0] << " 100 60          # 100 timers, 60s test, 2-30s timeouts" << std::endl;
    std::cout << "  " << argv[0] << " 100 60 40       # 100 timers, 60s test, 2-40s timeouts" << std::endl;
    std::cout << "  " << argv[0] << " 150 120 20 stress # 150 timers, 2min test, 2-20s timeouts, stress mode" << std::endl;
    std::cout << std::endl;
    
    try {
        // ✅ REMOVED: regeneration parameter, added max_timeout
        TigerPureOneShotTest test(max_timeout);
        test.set_stress_mode(stress_mode);
        test.run_pure_oneshot_test(initial_timer_count, duration, false); // Always disable regeneration
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}