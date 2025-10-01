#!/bin/bash

# monitor_test_fixed.sh - Universal monitoring for sw_task and tiger looper (NO REGENERATION)

if [ $# -lt 2 ]; then
    echo "Usage: $0 <framework> <program_path> [timer_count] [duration] [max_timeout] [mode]"
    echo ""
    echo "Frameworks (BOTH NO REGENERATION):"
    echo "  sw_task   : SW Task framework"
    echo "  tiger     : Tiger Looper framework"
    echo ""
    echo "Parameters (SAME FORMAT FOR BOTH):"
    echo "  program_path  : Path to executable (required)"
    echo "  timer_count   : Number of timers (default: 100)"
    echo "  duration      : Test duration in seconds (default: 60)"
    echo "  max_timeout   : Maximum timer timeout in seconds (default: 30)"
    echo "  mode          : Load mode: normal|stress (default: normal)"
    echo ""
    echo "Examples:"
    echo "  $0 sw_task ./sw_task_oneshot 100 60 15 stress"
    echo "  $0 tiger ./tiger_oneshot 100 60 15 stress"
    echo "  $0 sw_task ./sw_task_oneshot 200 120 5"
    echo "  $0 tiger ./tiger_oneshot 200 120 5"
    exit 1
fi

# Get framework and parameters (UNIFIED FORMAT)
FRAMEWORK="$1"
PROGRAM="$2"
PROGRAM=$(realpath "$PROGRAM")
TIMER_COUNT=${3:-100}
DURATION=${4:-60}
MAX_TIMEOUT=${5:-30}        # ✅ UNIFIED: Both frameworks use max_timeout
STRESS_MODE=${6:-"normal"}

# Validate framework
if [ "$FRAMEWORK" != "sw_task" ] && [ "$FRAMEWORK" != "tiger" ]; then
    echo "Error: Invalid framework '$FRAMEWORK'. Use 'sw_task' or 'tiger'"
    exit 1
fi

# Set framework-specific settings (SAME COMMAND FORMAT)
if [ "$FRAMEWORK" = "sw_task" ]; then
    OUTPUT_DIR="sw_task_result_$(date +%Y%m%d_%H%M%S)"
    FRAMEWORK_NAME="SW Task"
    
elif [ "$FRAMEWORK" = "tiger" ]; then
    OUTPUT_DIR="tiger_result_$(date +%Y%m%d_%H%M%S)"
    FRAMEWORK_NAME="Tiger Looper"
fi

# ✅ UNIFIED: Both frameworks use SAME command format
if [ "$STRESS_MODE" = "stress" ]; then
    PROGRAM_CMD="$PROGRAM $TIMER_COUNT $DURATION $MAX_TIMEOUT stress"
else
    PROGRAM_CMD="$PROGRAM $TIMER_COUNT $DURATION $MAX_TIMEOUT"
fi

# Validate program
if [ ! -f "$PROGRAM" ]; then
    echo "Error: Program not found: $PROGRAM"
    exit 1
fi

if [ ! -x "$PROGRAM" ]; then
    echo "Error: Program is not executable: $PROGRAM"
    echo "Try: chmod +x $PROGRAM"
    exit 1
fi

echo "$FRAMEWORK_NAME Timer Test Monitoring (NO REGENERATION)"
echo "==========================================="
echo "Framework: $FRAMEWORK_NAME"
echo "Program: $PROGRAM" 
echo "Timer count: $TIMER_COUNT"
echo "Duration: $DURATION seconds"
echo "Timer timeout range: 2-$MAX_TIMEOUT seconds"
echo "Load mode: $STRESS_MODE"
echo "Output directory: $OUTPUT_DIR"
echo "Command: $PROGRAM_CMD"
echo ""

# Create output directory
mkdir -p $OUTPUT_DIR
cd $OUTPUT_DIR

# Start program
$PROGRAM_CMD > "${FRAMEWORK}_output.log" 2>&1 &
TEST_PID=$!

echo "$FRAMEWORK_NAME PID: $TEST_PID"

# Verify process started
sleep 2
if ! kill -0 $TEST_PID 2>/dev/null; then
    echo "ERROR: $FRAMEWORK_NAME failed to start!"
    echo "Output:"
    cat "${FRAMEWORK}_output.log"
    exit 1
fi

echo "Process running. Starting monitoring..."

# CPU monitoring function
monitor_cpu() {
    local pid=$1
    local duration=$2
    local cpu_file=$3
    
    echo "timestamp,cpu_percent" > $cpu_file
    
    # High frequency sampling
    local sample_interval=0.5
    if [ "$STRESS_MODE" = "stress" ]; then
        sample_interval=0.2
        echo "Using high-frequency sampling (0.2s) for stress mode"
    fi
    
    local samples_per_second=$(awk "BEGIN {printf \"%.0f\", 1/$sample_interval}")
    echo "CPU sample interval: ${sample_interval}s"
    
    for ((i=0; i<=duration*samples_per_second; i++)); do
        if ! kill -0 $pid 2>/dev/null; then
            echo "Process ended at $((i/samples_per_second))s"
            break
        fi
        
        # Use TOP command for accurate CPU measurement
        local top_line=$(top -p $pid -b -n1 2>/dev/null | grep "^[[:space:]]*$pid")
        
        if [ -n "$top_line" ]; then
            local cpu=$(echo "$top_line" | awk '{print $9}')
            
            # Validate CPU value
            if ! [[ "$cpu" =~ ^[0-9]+\.?[0-9]*$ ]]; then
                cpu="0.0"
            fi
            
            local timestamp=$(awk "BEGIN {printf \"%.2f\", $i/$samples_per_second}")
            printf "%.2f,%.1f\n" "$timestamp" "$cpu" >> $cpu_file
            
            # Real-time display every 10 seconds
            if [ $((i % (10 * samples_per_second))) -eq 0 ] && [ $i -gt 0 ]; then
                printf "[%.0fs] CPU: %s%%\n" "$timestamp" "$cpu"
            fi
        else
            # Fallback to ps
            local ps_cpu=$(ps -p $pid -o %cpu --no-headers 2>/dev/null | tr -d ' ')
            if [ -n "$ps_cpu" ]; then
                local timestamp=$(awk "BEGIN {printf \"%.2f\", $i/$samples_per_second}")
                printf "%.2f,%.1f\n" "$timestamp" "$ps_cpu" >> $cpu_file
            fi
        fi
        
        sleep $sample_interval
    done
}

# Memory monitoring function
monitor_memory() {
    local pid=$1
    local duration=$2
    local mem_file=$3
    
    echo "timestamp,memory_mb,threads" > $mem_file
    
    # Memory sampling (less frequent than CPU)
    local sample_interval=1.0
    local samples_per_second=1
    echo "Memory sample interval: ${sample_interval}s"
    
    for ((i=0; i<=duration; i++)); do
        if ! kill -0 $pid 2>/dev/null; then
            echo "Process ended at ${i}s (memory monitor)"
            break
        fi
        
        # Get memory from TOP
        local top_line=$(top -p $pid -b -n1 2>/dev/null | grep "^[[:space:]]*$pid")
        
        if [ -n "$top_line" ]; then
            local mem_kb=$(echo "$top_line" | awk '{print $6}')
            local threads=1
            
            # Get thread count
            if [ -f "/proc/$pid/status" ]; then
                threads=$(awk '/^Threads:/ {print $2}' /proc/$pid/status 2>/dev/null || echo "1")
            fi
            
            # Convert memory to MB
            local mem_mb="0.00"
            if [[ "$mem_kb" =~ ^[0-9]+$ ]]; then
                mem_mb=$(awk "BEGIN {printf \"%.2f\", $mem_kb / 1024}")
            elif [[ "$mem_kb" =~ g$ ]]; then
                local mem_g=$(echo "$mem_kb" | sed 's/g//')
                mem_mb=$(awk "BEGIN {printf \"%.2f\", $mem_g * 1024}")
            elif [[ "$mem_kb" =~ m$ ]]; then
                mem_mb=$(echo "$mem_kb" | sed 's/m//')
            fi
            
            printf "%.2f,%.2f,%d\n" "$i" "$mem_mb" "$threads" >> $mem_file
            
            # Real-time display every 10 seconds
            if [ $((i % 10)) -eq 0 ] && [ $i -gt 0 ]; then
                printf "[%ds] Memory: %s MB, Threads: %s\n" "$i" "$mem_mb" "$threads"
            fi
        else
            # Fallback to ps
            local ps_info=$(ps -p $pid -o rss,nlwp --no-headers 2>/dev/null)
            if [ -n "$ps_info" ]; then
                read -r mem_kb threads <<< "$ps_info"
                local mem_mb=$(awk "BEGIN {printf \"%.2f\", $mem_kb / 1024}")
                printf "%.2f,%.2f,%d\n" "$i" "$mem_mb" "$threads" >> $mem_file
            fi
        fi
        
        sleep $sample_interval
    done
}

# Start monitoring in parallel
echo "Starting CPU and Memory monitoring..."
monitor_cpu $TEST_PID $DURATION "cpu_stats.csv" &
CPU_MONITOR_PID=$!

monitor_memory $TEST_PID $DURATION "memory_stats.csv" &
MEM_MONITOR_PID=$!

# Wait for test completion
wait $TEST_PID
TEST_EXIT_CODE=$?

echo "$FRAMEWORK_NAME completed with exit code: $TEST_EXIT_CODE"

# Stop monitoring
kill $CPU_MONITOR_PID 2>/dev/null
kill $MEM_MONITOR_PID 2>/dev/null
wait $CPU_MONITOR_PID 2>/dev/null
wait $MEM_MONITOR_PID 2>/dev/null

# Show results
echo ""
echo "Results saved in: $OUTPUT_DIR/"
echo "Files:"
echo "  - cpu_stats.csv: CPU usage data"
echo "  - memory_stats.csv: Memory usage and thread count data" 
echo "  - ${FRAMEWORK}_output.log: Program output"

# Quick stats
if [ -f "cpu_stats.csv" ] && [ $(wc -l < cpu_stats.csv) -gt 1 ]; then
    max_cpu=$(awk -F',' 'NR>1 {print $2}' cpu_stats.csv | sort -nr | head -1)
    avg_cpu=$(awk -F',' 'NR>1 {sum+=$2; count++} END {printf "%.2f", sum/count}' cpu_stats.csv)
    echo ""
    echo "Quick Stats:"
    echo "  Peak CPU: ${max_cpu}%"
    echo "  Average CPU: ${avg_cpu}%"
fi

if [ -f "memory_stats.csv" ] && [ $(wc -l < memory_stats.csv) -gt 1 ]; then
    max_mem=$(awk -F',' 'NR>1 {print $2}' memory_stats.csv | sort -nr | head -1)
    avg_mem=$(awk -F',' 'NR>1 {sum+=$2; count++} END {printf "%.2f", sum/count}' memory_stats.csv)
    echo "  Peak Memory: ${max_mem} MB"
    echo "  Average Memory: ${avg_mem} MB"
fi

echo ""
echo "Usage examples (UNIFIED FORMAT):"
echo "  $0 sw_task ./sw_task_oneshot 100 30 10 stress"
echo "  $0 tiger ./tiger_oneshot 100 30 10 stress"
echo "  $0 sw_task ./sw_task_oneshot 200 120 5"
echo "  $0 tiger ./tiger_oneshot 200 120 5"