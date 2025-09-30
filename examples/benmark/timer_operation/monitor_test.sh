#!/bin/bash

# monitor_timer_test.sh - Complete version with program parameter support and debugging

# Check if program path is provided
if [ $# -lt 1 ]; then
    echo "Usage: $0 <program_path> [oneshot] [periodic] [duration] [mode]"
    echo ""
    echo "Parameters:"
    echo "  program_path  : Path to timer program (required)"
    echo "  oneshot       : Number of one-shot timers (default: 1000)"
    echo "  periodic      : Number of periodic timers (default: 100)"
    echo "  duration      : Test duration in seconds (default: 60)"
    echo "  mode          : Load mode: normal|stress (default: normal)"
    echo ""
    echo "Examples:"
    echo "  $0 ./timer_load"
    echo "  $0 ./timer_load 2000 200 120"
    echo "  $0 ./timer_load 1000 50 30 stress"
    echo "  $0 /path/to/timer_program 500 100 60 normal"
    exit 1
fi

# Get parameters
PROGRAM="$1"
PROGRAM=$(realpath "$PROGRAM")
ONE_SHOT=${2:-1000}
PERIODIC=${3:-100}
DURATION=${4:-60}
STRESS_MODE=${5:-"normal"}
OUTPUT_DIR="result_$(date +%Y%m%d_%H%M%S)"

# Check if program exists
if [ ! -f "$PROGRAM" ]; then
    echo "Error: Program not found: $PROGRAM"
    echo "Please provide the correct path to your timer program"
    exit 1
fi

# Check if program is executable
if [ ! -x "$PROGRAM" ]; then
    echo "Error: Program is not executable: $PROGRAM"
    echo "Try: chmod +x $PROGRAM"
    exit 1
fi

echo "Timer Load Test Monitoring Script"
echo "================================="
echo "Program: $PROGRAM"
echo "One-shot timers: $ONE_SHOT"
echo "Periodic timers: $PERIODIC" 
echo "Duration: $DURATION seconds"
echo "Load mode: $STRESS_MODE"
echo "Output directory: $OUTPUT_DIR"
echo ""

# Create output directory
mkdir -p $OUTPUT_DIR
cd $OUTPUT_DIR

echo "Starting timer load test..."

# Build command with stress mode
if [ "$STRESS_MODE" = "stress" ]; then
    PROGRAM_CMD="$PROGRAM $ONE_SHOT $PERIODIC $DURATION stress"
    echo "Running in STRESS mode - expect high CPU usage"
else
    PROGRAM_CMD="$PROGRAM $ONE_SHOT $PERIODIC $DURATION"
    echo "Running in NORMAL mode"
fi

# Start the test program in background
$PROGRAM_CMD > test_output.log 2>&1 &
TEST_PID=$!

echo "Test PID: $TEST_PID"
echo "Command: $PROGRAM_CMD"

# Verify process started successfully
sleep 1
if ! kill -0 $TEST_PID 2>/dev/null; then
    echo "ERROR: Test process failed to start or exited immediately!"
    echo "Check test_output.log for details:"
    cat test_output.log
    exit 1
fi

echo "Process verified running. Starting monitoring..."

# Wait a moment for process to fully initialize
sleep 3

# Enhanced monitoring function with debugging
monitor_system() {
    local pid=$1
    local duration=$2
    local output_file=$3
    
    echo "Starting monitor_system with PID=$pid, duration=$duration"
    
    # Create CSV header
    echo "timestamp,cpu_percent,memory_mb,threads,context_switches" > $output_file
    
    # Use faster sampling for stress mode
    local sample_interval=0.2
    if [ "$STRESS_MODE" = "stress" ]; then
        sample_interval=0.1
        echo "Using high-frequency monitoring (0.25s intervals) for stress mode"
    fi
    
    local samples_per_second=$(awk "BEGIN {printf \"%.0f\", 1/$sample_interval}")
    echo "Sample interval: ${sample_interval}s, samples per second: $samples_per_second"
    
    local data_points=0
    
    for ((i=0; i<=duration*samples_per_second; i++)); do
        # Check if process still exists
        if ! kill -0 $pid 2>/dev/null; then
            echo "Process $pid ended early at $((i/samples_per_second)) seconds"
            break
        fi
        
        # Initialize variables with default values
        local cpu="0.0"
        local memory_kb="0"
        local threads="1"
        local ctx_switches="0"
        
        # Method 1: ps command
        local ps_output=$(ps -p $pid -o %cpu,rss,nlwp --no-headers 2>/dev/null)
        if [ -n "$ps_output" ]; then
            cpu=$(echo "$ps_output" | awk '{print $1}')
            memory_kb=$(echo "$ps_output" | awk '{print $2}')
            threads=$(echo "$ps_output" | awk '{print $3}')
        else
            echo "Warning: ps command failed for PID $pid at iteration $i"
        fi
        
        # Method 2: top command for better CPU detection in stress mode
        if [ "$STRESS_MODE" = "stress" ] || [ "$cpu" = "0.0" ]; then
            local top_cpu=$(top -p $pid -b -n1 2>/dev/null | tail -1 | awk '{print $9}' 2>/dev/null)
            if [ -n "$top_cpu" ] && [ "$top_cpu" != "0.0" ] && [[ "$top_cpu" =~ ^[0-9]+\.?[0-9]*$ ]]; then
                cpu="$top_cpu"
            fi
        fi
        
        # Memory from /proc/status (more accurate)
        if [ -f "/proc/$pid/status" ]; then
            local vmrss=$(awk '/VmRSS/ {print $2}' /proc/$pid/status 2>/dev/null)
            if [ -n "$vmrss" ] && [ "$vmrss" != "" ]; then
                memory_kb="$vmrss"
            fi
            
            # Context switches
            local ctx_temp=$(awk '/voluntary_ctxt_switches/ {print $2}' /proc/$pid/status 2>/dev/null | head -1 | tr -d '\n\r ')
            if [ -n "$ctx_temp" ] && [[ "$ctx_temp" =~ ^[0-9]+$ ]]; then
                ctx_switches="$ctx_temp"
            fi
        fi
        
        # Validate and clean values
        [ -z "$cpu" ] || [ "$cpu" = "%CPU" ] && cpu="0.0"
        [ -z "$memory_kb" ] && memory_kb="0"
        [ -z "$threads" ] && threads="1"
        [ -z "$ctx_switches" ] && ctx_switches="0"
        
        # Convert memory from KB to MB
        local memory_mb=$(awk "BEGIN {printf \"%.2f\", $memory_kb / 1024}" 2>/dev/null || echo "0.00")
        
        # Validate CPU is numeric
        if ! [[ "$cpu" =~ ^[0-9]+\.?[0-9]*$ ]]; then
            cpu="0.0"
        fi
        
        # Validate all numeric fields
        if ! [[ "$threads" =~ ^[0-9]+$ ]]; then
            threads="1"
        fi
        
        if ! [[ "$ctx_switches" =~ ^[0-9]+$ ]]; then
            ctx_switches="0"
        fi
        
        # Calculate fractional timestamp
        local timestamp=$(awk "BEGIN {printf \"%.2f\", $i/$samples_per_second}")
        
        # Write data to file
        printf "%.2f,%.1f,%.2f,%d,%d\n" "$timestamp" "$cpu" "$memory_mb" "$threads" "$ctx_switches" >> $output_file
        
        data_points=$((data_points + 1))
        
        # Debug output every 10 seconds
        if [ $((i % (10 * samples_per_second))) -eq 0 ] && [ $i -gt 0 ]; then
            echo "Monitor checkpoint: ${timestamp}s - CPU: ${cpu}%, Memory: ${memory_mb}MB, Threads: $threads"
        fi
        
        sleep $sample_interval
    done
    
    echo "Monitor completed. Data points collected: $data_points"
    
    # Verify data was written
    local line_count=$(wc -l < $output_file)
    echo "CSV file $output_file has $line_count lines (including header)"
    
    if [ $line_count -le 1 ]; then
        echo "WARNING: No data was written to $output_file!"
        echo "File contents:"
        cat $output_file
    fi
}

# Function to monitor system load
monitor_system_load() {
    local duration=$1
    local output_file=$2
    
    echo "timestamp,load_1min,load_5min,load_15min" > $output_file
    
    local sample_interval=0.5
    if [ "$STRESS_MODE" = "stress" ]; then
        sample_interval=0.25
    fi
    
    local samples_per_second=$(awk "BEGIN {printf \"%.0f\", 1/$sample_interval}")
    
    for ((i=0; i<=duration*samples_per_second; i++)); do
        local load_data=$(cat /proc/loadavg 2>/dev/null)
        if [ -n "$load_data" ]; then
            local load_1=$(echo "$load_data" | awk '{print $1}')
            local load_5=$(echo "$load_data" | awk '{print $2}')
            local load_15=$(echo "$load_data" | awk '{print $3}')
            
            local timestamp=$(awk "BEGIN {printf \"%.2f\", $i/$samples_per_second}")
            printf "%.2f,%.2f,%.2f,%.2f\n" "$timestamp" "$load_1" "$load_5" "$load_15" >> $output_file
        fi
        
        sleep $sample_interval
    done
}

# Start monitoring in background
echo "Starting process monitoring..."
monitor_system $TEST_PID $DURATION "process_stats.csv" &
MONITOR_PID1=$!

echo "Starting system load monitoring..."
monitor_system_load $DURATION "system_load.csv" &
MONITOR_PID2=$!

# Use perf if available
if command -v perf &> /dev/null && [ "$EUID" -eq 0 ]; then
    echo "Running perf profiling..."
    if [ "$STRESS_MODE" = "stress" ]; then
        perf stat -p $TEST_PID \
            -e cycles,instructions,cache-misses,context-switches,cpu-migrations,page-faults \
            -o perf_stats.txt 2>&1 &
    else
        perf stat -p $TEST_PID -e cycles,instructions,cache-misses,context-switches \
            -o perf_stats.txt 2>&1 &
    fi
    PERF_PID=$!
fi

# Real-time monitoring display for stress mode
if [ "$STRESS_MODE" = "stress" ]; then
    echo "Stress mode active - monitoring CPU usage..."
    (
        for i in {1..10}; do
            sleep 3
            current_cpu=$(ps -p $TEST_PID -o %cpu --no-headers 2>/dev/null | tr -d ' ')
            if [ -n "$current_cpu" ]; then
                echo "  [${i}0s] Current CPU: ${current_cpu}%"
            else
                echo "  [${i}0s] Process not found"
                break
            fi
        done
    ) &
    MONITOR_DISPLAY_PID=$!
fi

# Wait for test to complete
echo "Waiting for test to complete..."
wait $TEST_PID
TEST_EXIT_CODE=$?

echo "Test completed with exit code: $TEST_EXIT_CODE"

# Clean up background processes
echo "Cleaning up monitoring processes..."
wait $MONITOR_PID1 2>/dev/null
wait $MONITOR_PID2 2>/dev/null

if [ -n "$PERF_PID" ]; then
    kill $PERF_PID 2>/dev/null
    wait $PERF_PID 2>/dev/null
fi

if [ -n "$MONITOR_DISPLAY_PID" ]; then
    kill $MONITOR_DISPLAY_PID 2>/dev/null
    wait $MONITOR_DISPLAY_PID 2>/dev/null
fi

echo "Monitoring completed. Results in $OUTPUT_DIR/"

# Verify files were created properly
echo "Verifying output files..."
for file in process_stats.csv system_load.csv; do
    if [ -f "$file" ]; then
        lines=$(wc -l < "$file")
        size=$(du -h "$file" | cut -f1)
        echo "  $file: $lines lines, $size"
        if [ $lines -le 1 ]; then
            echo "    WARNING: File appears to be empty or header-only!"
        fi
    else
        echo "  $file: NOT FOUND"
    fi
done

# Fix permissions if run with sudo
if [ "$EUID" -eq 0 ]; then
    echo "Fixing permissions..."
    if [ -n "$SUDO_USER" ]; then
        chown -R $SUDO_USER:$SUDO_USER .
        cd ..
        chown -R $SUDO_USER:$SUDO_USER result_* 2>/dev/null || true
        cd $OUTPUT_DIR
    fi
fi

# Generate enhanced summary
echo "Generating summary..."
cat > summary.txt << EOF
Timer Load Test Summary
======================
Test Parameters:
- Program: $PROGRAM
- One-shot timers: $ONE_SHOT
- Periodic timers: $PERIODIC
- Duration: $DURATION seconds
- Load mode: $STRESS_MODE
- PID: $TEST_PID
- Exit code: $TEST_EXIT_CODE

Files generated:
- test_output.log: Application output
- process_stats.csv: Process-specific metrics
- system_load.csv: System-wide metrics
- perf_stats.txt: Performance counters (if available)

Performance Metrics:
EOF

# Extract and calculate statistics
if [ -f "process_stats.csv" ] && [ $(wc -l < process_stats.csv) -gt 1 ]; then
    max_cpu=$(awk -F',' 'NR>1 {print $2}' process_stats.csv | sort -nr | head -1)
    max_mem=$(awk -F',' 'NR>1 {print $3}' process_stats.csv | sort -nr | head -1) 
    max_threads=$(awk -F',' 'NR>1 {print $4}' process_stats.csv | sort -nr | head -1)
    max_ctx=$(awk -F',' 'NR>1 {print $5}' process_stats.csv | sort -nr | head -1)
    
    avg_cpu=$(awk -F',' 'NR>1 {sum+=$2; count++} END {printf "%.2f", sum/count}' process_stats.csv)
    avg_mem=$(awk -F',' 'NR>1 {sum+=$3; count++} END {printf "%.2f", sum/count}' process_stats.csv)
    
    # Calculate non-zero CPU average
    nonzero_avg_cpu=$(awk -F',' 'NR>1 && $2>0 {sum+=$2; count++} END {if(count>0) printf "%.2f", sum/count; else print "0.00"}' process_stats.csv)
    
    echo "- Peak CPU: ${max_cpu}%" >> summary.txt
    echo "- Average CPU: ${avg_cpu}%" >> summary.txt
    echo "- Active CPU Average: ${nonzero_avg_cpu}% (when > 0%)" >> summary.txt
    echo "- Peak Memory: ${max_mem} MB" >> summary.txt  
    echo "- Average Memory: ${avg_mem} MB" >> summary.txt
    echo "- Max Threads: ${max_threads}" >> summary.txt
    echo "- Max Context Switches: ${max_ctx}" >> summary.txt
    
    # Performance assessment
    if [ "$STRESS_MODE" = "stress" ]; then
        echo "" >> summary.txt
        echo "Stress Mode Assessment:" >> summary.txt
        if command -v bc &> /dev/null && (( $(echo "$max_cpu > 20" | bc -l) )); then
            echo "✓ High CPU load achieved (${max_cpu}%)" >> summary.txt
        else
            echo "! Lower than expected CPU load (${max_cpu}%)" >> summary.txt
        fi
    fi
else
    echo "- No valid data collected" >> summary.txt
    echo ""
    echo "TROUBLESHOOTING:"
    echo "- Check if program path is correct"
    echo "- Verify program runs successfully: $PROGRAM_CMD"
    echo "- Check test_output.log for errors"
    echo "- Ensure sufficient test duration (minimum 10 seconds recommended)"
fi

echo ""
echo "Summary:"
cat summary.txt

echo ""
echo "To analyze results:"
echo "  cd $OUTPUT_DIR"
echo "  cat summary.txt"
echo "  python3 ../plot_results.py"
echo ""
echo "Usage examples:"
echo "  $0 ./timer_load 1000 50 30           # Normal load test"
echo "  $0 ./timer_load 1000 50 30 stress    # High CPU stress test"
echo "  $0 /path/to/program 2000 100 60 stress   # Heavy stress test"
echo ""
echo "For debugging:"
echo "  cat $OUTPUT_DIR/test_output.log       # Check program output"
echo "  cat $OUTPUT_DIR/process_stats.csv     # Check collected data"