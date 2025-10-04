#!/usr/bin/env python3
import subprocess
import time
import sys
import argparse
import json
import threading
import re
from datetime import datetime

class TopMonitor:
    def __init__(self):
        self.monitoring = False
        self.data = {
            'framework': '',
            'command': '',
            'start_time': None,
            'end_time': None,
            'timestamps': [],
            'memory_mb': [],
            'cpu_percent': [],
            'threads': [],
            'virtual_mb': [],
            'samples': 0
        }
        
    def check_monitoring_tools(self):
        """Check which monitoring tool is available"""
        # Check top first
        try:
            result = subprocess.run(['top', '-b', '-n', '1'], capture_output=True, text=True, timeout=2)
            if result.returncode == 0:
                return 'top'
        except:
            pass
            
        # Fallback to ps
        return 'ps'
    
    def start_monitoring_with_top(self, process_name, duration=60):
        """Monitor using top command"""
        self.monitoring = True
        start_time = time.time()
        sample_count = 0
        
        print(f"🔄 Monitoring process '{process_name}' for {duration} seconds using top...")
        
        while time.time() - start_time < duration and self.monitoring:
            try:
                current_time = time.time() - start_time
                
                # Use top in batch mode with wide format
                result = subprocess.run(['top', '-b', '-n', '1', '-w', '512'], 
                                      capture_output=True, text=True, timeout=3)
                
                if result.returncode == 0:
                    memory_mb = 0
                    cpu_percent = 0
                    threads = 0
                    virtual_mb = 0
                    
                    # Parse top output - look for our process
                    lines = result.stdout.split('\n')
                    for line in lines:
                        if process_name in line and 'top' not in line and len(line.strip()) > 0:
                            # top format: PID USER PR NI VIRT RES SHR S %CPU %MEM TIME+ COMMAND
                            parts = line.split()
                            if len(parts) >= 11:
                                try:
                                    pid = parts[0]
                                    virtual_str = parts[4]  # VIRT
                                    rss_str = parts[5]      # RES
                                    cpu_str = parts[8]      # %CPU
                                    
                                    # Parse memory values (top uses different formats)
                                    virtual_kb = self.parse_top_memory(virtual_str)
                                    rss_kb = self.parse_top_memory(rss_str)
                                    cpu_percent = float(cpu_str)
                                    
                                    memory_mb = rss_kb / 1024
                                    virtual_mb = virtual_kb / 1024
                                    
                                    # Get thread count
                                    threads = self.get_thread_count(pid)
                                    break
                                except (ValueError, IndexError) as e:
                                    continue
                    
                    self.store_sample(current_time, memory_mb, cpu_percent, threads, virtual_mb, sample_count)
                    sample_count += 1
                    
                else:
                    print(f"   ⚠️  top command failed: {result.stderr}")
                
                time.sleep(0.1)  # Sample every 0.1 second
                
            except subprocess.TimeoutExpired:
                print(f"   ⚠️  top timeout")
                time.sleep(0.1)
            except KeyboardInterrupt:
                print(f"\n   🛑 Monitoring interrupted by user")
                break
            except Exception as e:
                print(f"   ⚠️  Monitoring error: {e}")
                time.sleep(0.1)
                
        self.finish_monitoring(sample_count)
    
    def start_monitoring_with_ps(self, process_name, duration=60):
        """Monitor using ps command (fallback)"""
        self.monitoring = True
        start_time = time.time()
        sample_count = 0
        
        print(f"🔄 Monitoring process '{process_name}' for {duration} seconds using ps (fallback)...")
        
        while time.time() - start_time < duration and self.monitoring:
            try:
                current_time = time.time() - start_time
                
                # Use ps aux
                result = subprocess.run(['ps', 'aux'], capture_output=True, text=True)
                
                if result.returncode == 0:
                    memory_mb = 0
                    cpu_percent = 0
                    threads = 0
                    virtual_mb = 0
                    
                    for line in result.stdout.split('\n'):
                        if process_name in line and 'grep' not in line and 'ps' not in line and 'monitor.py' not in line:
                            parts = line.split()
                            if len(parts) >= 11:
                                try:
                                    cpu_percent = float(parts[2])  # %CPU
                                    virtual_kb = int(parts[4])     # VSZ
                                    rss_kb = int(parts[5])         # RSS
                                    memory_mb = rss_kb / 1024
                                    virtual_mb = virtual_kb / 1024
                                    pid = parts[1]
                                    threads = self.get_thread_count(pid)
                                    break
                                except (ValueError, IndexError):
                                    continue
                    
                    self.store_sample(current_time, memory_mb, cpu_percent, threads, virtual_mb, sample_count)
                    sample_count += 1
                    
                else:
                    print(f"   ⚠️  ps command failed")
                
                time.sleep(0.1)
                
            except KeyboardInterrupt:
                print(f"\n   🛑 Monitoring interrupted by user")
                break
            except Exception as e:
                print(f"   ⚠️  Monitoring error: {e}")
                time.sleep(0.1)
                
        self.finish_monitoring(sample_count)
    
    def parse_top_memory(self, mem_str):
        """Parse memory value from top output"""
        if not mem_str or mem_str == '0':
            return 0
        
        try:
            # Remove any trailing characters and convert
            mem_str = mem_str.upper()
            
            # Handle different suffixes
            if mem_str.endswith('G'):
                return int(float(mem_str[:-1]) * 1024 * 1024)  # GB to KB
            elif mem_str.endswith('M'):
                return int(float(mem_str[:-1]) * 1024)         # MB to KB
            elif mem_str.endswith('K'):
                return int(float(mem_str[:-1]))                # KB
            else:
                # Assume it's in KB
                return int(float(mem_str))
        except:
            return 0
    
    def get_thread_count(self, pid):
        """Get thread count for a process"""
        try:
            result = subprocess.run(['ps', '-o', 'nlwp', '-p', str(pid)], 
                                  capture_output=True, text=True)
            if result.returncode == 0:
                lines = result.stdout.strip().split('\n')
                if len(lines) > 1:
                    return int(lines[1])
        except:
            pass
        return 0
    
    def store_sample(self, current_time, memory_mb, cpu_percent, threads, virtual_mb, sample_count):
        """Store sample data"""
        self.data['timestamps'].append(current_time)
        self.data['memory_mb'].append(memory_mb)
        self.data['cpu_percent'].append(cpu_percent)
        self.data['threads'].append(threads)
        self.data['virtual_mb'].append(virtual_mb)
        
        # Print progress every 50 samples (5 seconds at 0.1s rate) or when memory > 0
        if sample_count % 50 == 0 or memory_mb > 0:
            print(f"   📊 [{current_time:5.1f}s] Memory: {memory_mb:6.1f}MB, "
                  f"CPU: {cpu_percent:5.1f}%, Threads: {threads:2d}, "
                  f"Virtual: {virtual_mb:6.1f}MB")
    
    def finish_monitoring(self, sample_count):
        """Finish monitoring and print summary"""
        self.monitoring = False
        self.data['samples'] = sample_count
        self.data['end_time'] = datetime.now().isoformat()
        
        print(f"✅ Monitoring completed. Collected {sample_count} samples")
        
        if self.data['memory_mb']:
            max_mem = max(self.data['memory_mb'])
            avg_mem = sum(self.data['memory_mb']) / len(self.data['memory_mb'])
            max_cpu = max(self.data['cpu_percent'])
            avg_cpu = sum(self.data['cpu_percent']) / len(self.data['cpu_percent'])
            max_threads = max(self.data['threads'])
            
            print(f"📊 Summary: Memory Max={max_mem:.1f}MB Avg={avg_mem:.1f}MB, "
                  f"CPU Max={max_cpu:.1f}% Avg={avg_cpu:.1f}%, Max Threads={max_threads}")
    
    def start_monitoring(self, process_name, duration=60):
        """Start monitoring using the best available tool"""
        tool = self.check_monitoring_tools()
        print(f"🔧 Using monitoring tool: {tool.upper()}")
        
        if tool == 'top':
            self.start_monitoring_with_top(process_name, duration)
        else:
            self.start_monitoring_with_ps(process_name, duration)

def run_program_and_monitor(cmd_args, output_file, duration=60):
    """Run program and monitor it using top/ps"""
    process_name = cmd_args[0].split('/')[-1] if cmd_args else ''
    print(f"🚀 Starting program: {' '.join(cmd_args)}")
    print(f"📋 Process name to monitor: {process_name}")
    
    monitor = TopMonitor()
    monitor.data['command'] = ' '.join(cmd_args)
    monitor.data['start_time'] = datetime.now().isoformat()
    
    if 'tiger' in process_name.lower():
        monitor.data['framework'] = 'tiger'
    elif 'stress_timer' in process_name:
        monitor.data['framework'] = 'sw_task'
    else:
        monitor.data['framework'] = 'unknown'
    
    try:
        process = subprocess.Popen(cmd_args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        monitor_thread = threading.Thread(target=monitor.start_monitoring, args=(process_name, duration))
        monitor_thread.start()
        
        try:
            stdout, stderr = process.communicate(timeout=duration + 10)
            return_code = process.returncode
            print(f"🏁 Program completed with return code: {return_code}")
            if stdout.strip():
                print(f"📄 Program output:\n{stdout}")
            if stderr.strip():
                print(f"⚠️  Program errors:\n{stderr}")
        except subprocess.TimeoutExpired:
            print(f"⏰ Program timeout, terminating...")
            process.terminate()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
            stdout, stderr = process.communicate()
            return_code = -1
        
        monitor.monitoring = False
        monitor_thread.join(timeout=5)
        monitor.data['return_code'] = return_code
        monitor.data['stdout'] = stdout
        monitor.data['stderr'] = stderr
        
        with open(output_file, 'w') as f:
            json.dump(monitor.data, f, indent=2)
        
        print(f"💾 Results saved to: {output_file}")
        return monitor.data
        
    except Exception as e:
        print(f"❌ Failed to run program: {e}")
        monitor.monitoring = False
        return None

def print_usage():
    print("📊 Top-based Memory and CPU Monitor")
    print("=" * 60)
    print("Uses system monitoring tools to track process performance:")
    print("  1. top (preferred) - real-time process information")
    print("  2. ps (fallback) - process snapshot")
    print()
    print("Usage:")
    print("  python3 monitor.py -o <output.json> -d <duration> -- <program> [args]")
    print()
    print("Parameters:")
    print("  -o <output.json>   Output JSON file (default: monitor_result.json)")
    print("  -d <duration>      Monitoring duration in seconds (default: 60)")
    print("  <program> [args]   Program to run and monitor")
    print()
    print("Examples:")
    print("  python3 monitor.py -o sw_result.json -d 30 -- ./stress_timer2 100 10")
    print("  python3 monitor.py -o tiger_result.json -d 30 -- ./tiger_stress_timer2 100 10")
    print()
    print("Features:")
    print("  - High resolution sampling (0.1 seconds)")
    print("  - Memory usage tracking (RSS + Virtual)")
    print("  - CPU usage monitoring")
    print("  - Thread count tracking")
    print("  - Process lifetime monitoring")
    print()
    print("After monitoring, compare results with:")
    print("  python3 plot.py sw_result.json tiger_result.json")
    print("=" * 60)

def main():
    parser = argparse.ArgumentParser(description='Top-based Memory and CPU Monitor', add_help=False)
    parser.add_argument('-o', '--output', default='monitor_result.json',
                       help='Output JSON file (default: monitor_result.json)')
    parser.add_argument('-d', '--duration', type=int, default=60,
                       help='Monitoring duration in seconds (default: 60)')
    parser.add_argument('command', nargs=argparse.REMAINDER,
                       help='Command to run and monitor')
    
    args, unknown = parser.parse_known_args()
    
    if not args.command:
        print_usage()
        return 1
    
    if args.command and args.command[0] == '--':
        args.command = args.command[1:]
    
    print("📊 Top-based Memory and CPU Monitor")
    print("=" * 50)
    print(f"Command: {' '.join(args.command)}")
    print(f"Duration: {args.duration} seconds")
    print(f"Output: {args.output}")
    print(f"Sample rate: 0.1 seconds (high resolution)")
    print("=" * 50)
    
    result = run_program_and_monitor(args.command, args.output, args.duration)
    
    if result:
        print(f"\n🎉 Monitoring completed successfully!")
        return 0
    else:
        print(f"\n❌ Monitoring failed!")
        return 1

if __name__ == "__main__":
    sys.exit(main())