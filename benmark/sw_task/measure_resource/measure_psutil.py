#!/usr/bin/env python3
# universal_monitor.py - Monitor any program with any parameters
import psutil
import time
import json
import subprocess
import sys
import threading
from datetime import datetime
import matplotlib
matplotlib.use('Agg')

class PerformanceMonitor:
    def __init__(self):
        self.metrics = []
        self.monitoring = False
        self.process = None
        
    def start_monitoring(self, pid, duration=300):
        """Start monitoring a process"""
        self.monitoring = True
        try:
            self.process = psutil.Process(pid)
            start_time = time.time()
            
            print(f"Monitoring PID {pid} for {duration} seconds...")
            
            while time.time() - start_time < duration and self.monitoring:
                try:
                    # Process metrics
                    cpu_percent = self.process.cpu_percent()
                    memory_mb = self.process.memory_info().rss / 1024 / 1024
                    thread_count = self.process.num_threads()
                    
                    # System metrics
                    system_cpu_per_core = psutil.cpu_percent(percpu=True)
                    system_cpu_avg = psutil.cpu_percent()
                    cores_used = sum(1 for cpu in system_cpu_per_core if cpu > 5.0)
                    
                    # Memory
                    system_memory = psutil.virtual_memory()
                    
                    metric = {
                        'timestamp': time.time(),
                        'process_cpu': cpu_percent,
                        'process_memory_mb': memory_mb,
                        'process_threads': thread_count,
                        'system_cpu_avg': system_cpu_avg,
                        'system_cpu_cores': system_cpu_per_core,
                        'cores_active': cores_used,
                        'system_memory_mb': system_memory.used / 1024 / 1024,
                        'system_memory_percent': system_memory.percent
                    }
                    
                    self.metrics.append(metric)
                    
                    # Print real-time stats
                    print(f"CPU: {cpu_percent:6.1f}% | "
                         f"Mem: {memory_mb:6.1f}MB | "
                         f"Threads: {thread_count:2d} | "
                         f"Cores: {cores_used:2d}/{len(system_cpu_per_core)}")
                    
                    time.sleep(0.5)  # Sample every 500ms
                    
                except psutil.NoSuchProcess:
                    print("Process terminated")
                    break
                except Exception as e:
                    print(f"Monitoring error: {e}")
                    break
                    
        except Exception as e:
            print(f"Failed to monitor process: {e}")
            
    def stop_monitoring(self):
        self.monitoring = False
        
    def save_results(self, filename):
        """Save monitoring results to JSON file"""
        with open(filename, 'w') as f:
            json.dump({
                'start_time': self.metrics[0]['timestamp'] if self.metrics else 0,
                'end_time': self.metrics[-1]['timestamp'] if self.metrics else 0,
                'duration': self.metrics[-1]['timestamp'] - self.metrics[0]['timestamp'] if len(self.metrics) > 1 else 0,
                'metrics': self.metrics
            }, f, indent=2)
            
    def get_summary(self):
        """Get performance summary"""
        if not self.metrics:
            return {}
            
        cpu_values = [m['process_cpu'] for m in self.metrics]
        memory_values = [m['process_memory_mb'] for m in self.metrics]
        thread_values = [m['process_threads'] for m in self.metrics]
        cores_values = [m['cores_active'] for m in self.metrics]
        
        return {
            'cpu_avg': sum(cpu_values) / len(cpu_values),
            'cpu_max': max(cpu_values),
            'memory_avg': sum(memory_values) / len(memory_values),
            'memory_max': max(memory_values),
            'threads_avg': sum(thread_values) / len(thread_values),
            'threads_max': max(thread_values),
            'cores_avg': sum(cores_values) / len(cores_values),
            'cores_max': max(cores_values),
            'duration': self.metrics[-1]['timestamp'] - self.metrics[0]['timestamp'] if len(self.metrics) > 1 else 0
        }

def run_test_with_monitoring(cmd_args, output_file, duration=300):
    """Run any command and monitor its performance"""
    
    print(f"Starting command: {' '.join(cmd_args)}")
    
    # Start the process
    process = subprocess.Popen(cmd_args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    
    # Start monitoring
    monitor = PerformanceMonitor()
    
    # Monitor in separate thread
    monitor_thread = threading.Thread(
        target=monitor.start_monitoring, 
        args=(process.pid, duration)
    )
    monitor_thread.start()
    
    # Wait for process to complete
    stdout, stderr = process.communicate()
    
    # Stop monitoring
    monitor.stop_monitoring()
    monitor_thread.join()
    
    # Save results
    monitor.save_results(output_file)
    
    # Print process output
    print("=== Process Output ===")
    print(stdout)
    if stderr:
        print("=== Process Errors ===")
        print(stderr)
    
    # Print summary
    summary = monitor.get_summary()
    print(f"\n=== Performance Summary ===")
    print(f"Duration: {summary.get('duration', 0):.1f} seconds")
    print(f"CPU: avg={summary.get('cpu_avg', 0):.1f}%, max={summary.get('cpu_max', 0):.1f}%")
    print(f"Memory: avg={summary.get('memory_avg', 0):.1f}MB, max={summary.get('memory_max', 0):.1f}MB")
    print(f"Threads: avg={summary.get('threads_avg', 0):.1f}, max={summary.get('threads_max', 0)}")
    print(f"CPU Cores: avg={summary.get('cores_avg', 0):.1f}, max={summary.get('cores_max', 0)}")
    
    return process.returncode, summary

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Universal Performance Monitor")
        print("Usage: python3 universal_monitor.py [OPTIONS] -- <command> [args...]")
        print()
        print("Options:")
        print("  -o, --output FILE    Output JSON file (default: auto-generated)")
        print("  -d, --duration SEC   Monitoring duration in seconds (default: 300)")
        print()
        print("Examples:")
        print("  python3 universal_monitor.py -- ./task_runner 10 2")
        print("  python3 universal_monitor.py -o result.json -d 60 -- ./task_runner 5 8")
        print("  python3 universal_monitor.py -- python3 my_script.py arg1 arg2")
        print("  python3 universal_monitor.py -- java -jar app.jar -param value")
        print("  python3 universal_monitor.py -- ./compile.sh")
        sys.exit(1)
    
    # Parse arguments
    output_file = None
    duration = 300
    cmd_start_idx = 1
    
    i = 1
    while i < len(sys.argv):
        if sys.argv[i] == '--':
            cmd_start_idx = i + 1
            break
        elif sys.argv[i] in ['-o', '--output']:
            if i + 1 < len(sys.argv):
                output_file = sys.argv[i + 1]
                i += 2
            else:
                print("Error: --output requires a filename")
                sys.exit(1)
        elif sys.argv[i] in ['-d', '--duration']:
            if i + 1 < len(sys.argv):
                try:
                    duration = int(sys.argv[i + 1])
                except ValueError:
                    print("Error: --duration must be a number")
                    sys.exit(1)
                i += 2
            else:
                print("Error: --duration requires a number")
                sys.exit(1)
        elif sys.argv[i].startswith('-'):
            print(f"Error: Unknown option {sys.argv[i]}")
            sys.exit(1)
        else:
            # No '--' separator, assume everything from here is the command
            cmd_start_idx = i
            break
    
    if cmd_start_idx >= len(sys.argv):
        print("Error: No command specified")
        sys.exit(1)
    
    # Extract command and arguments
    cmd_args = sys.argv[cmd_start_idx:]
    
    # Generate output filename if not specified
    if output_file is None:
        cmd_name = cmd_args[0].split('/')[-1]  # Get basename
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        output_file = f"monitor_{cmd_name}_{timestamp}.json"
    
    print(f"Configuration:")
    print(f"  Command: {' '.join(cmd_args)}")
    print(f"  Output file: {output_file}")
    print(f"  Monitoring duration: {duration} seconds")
    print()
    
    run_test_with_monitoring(cmd_args, output_file, duration)