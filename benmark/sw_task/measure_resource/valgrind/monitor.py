#!/usr/bin/env python3
import matplotlib.pyplot as plt
import sys
import os

def parse_massif(filename):
    """Parse Valgrind massif output file"""
    times = []
    stacks = []
    heaps = []
    
    if not os.path.exists(filename):
        print(f"❌ Error: File not found: {filename}")
        return None, None, None
    
    try:
        with open(filename, 'r') as f:
            current_time = 0
            current_stack = 0
            current_heap = 0
            
            for line in f:
                line = line.strip()
                if line.startswith('time='):
                    current_time = int(line.split('=')[1])
                elif line.startswith('mem_stacks_B='):
                    current_stack = int(line.split('=')[1]) / (1024*1024)  # Convert to MB
                elif line.startswith('mem_heap_B='):
                    current_heap = int(line.split('=')[1]) / (1024*1024)   # Convert to MB
                elif line.startswith('snapshot=') and line != 'snapshot=0':
                    # Save data for previous snapshot
                    times.append(current_time)
                    stacks.append(current_stack)
                    heaps.append(current_heap)
            
            # Save last snapshot
            if current_time > 0:
                times.append(current_time)
                stacks.append(current_stack)
                heaps.append(current_heap)
                
        print(f"✅ Parsed {filename}: {len(times)} snapshots")
        return times, stacks, heaps
        
    except Exception as e:
        print(f"❌ Error parsing {filename}: {e}")
        return None, None, None

def normalize_to_snapshots(times):
    """Use snapshot index as X-axis (most reliable)"""
    return list(range(len(times)))

def normalize_to_percentage(times):
    """Normalize times to 0-100% of execution"""
    if not times:
        return []
    
    min_time = min(times)
    max_time = max(times)
    if max_time == min_time:
        return [0] * len(times)
    
    normalized = [(t - min_time) / (max_time - min_time) * 100 for t in times]
    return normalized

def normalize_to_estimated_seconds(times, estimated_duration=30):
    """Normalize times to estimated real duration in seconds"""
    if not times:
        return []
    
    min_time = min(times)
    max_time = max(times)
    if max_time == min_time:
        return [0] * len(times)
    
    normalized = [(t - min_time) / (max_time - min_time) * estimated_duration for t in times]
    return normalized

def plot_comparison(tiger_file, sw_file, time_mode="snapshots"):
    """Plot memory comparison between Tiger Looper and SW Task"""
    
    # Parse both files
    tiger_times, tiger_stacks, tiger_heaps = parse_massif(tiger_file)
    sw_times, sw_stacks, sw_heaps = parse_massif(sw_file)
    
    if not tiger_times or not sw_times:
        print("❌ Failed to parse input files")
        return False
    
    # Normalize times based on selected mode
    if time_mode == "snapshots":
        tiger_norm_times = normalize_to_snapshots(tiger_times)
        sw_norm_times = normalize_to_snapshots(sw_times)
        x_label = "Snapshot Number"
        time_suffix = "snapshots"
    elif time_mode == "percentage":
        tiger_norm_times = normalize_to_percentage(tiger_times)
        sw_norm_times = normalize_to_percentage(sw_times)
        x_label = "Execution Progress (%)"
        time_suffix = "percentage"
    else:  # estimated_seconds
        tiger_norm_times = normalize_to_estimated_seconds(tiger_times, 30)
        sw_norm_times = normalize_to_estimated_seconds(sw_times, 30)
        x_label = "Estimated Time (seconds)"
        time_suffix = "estimated_seconds"
    
    # Create figure with subplots
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(14, 10))
    
    # Plot 1: Stack Memory Comparison
    ax1.plot(tiger_norm_times, tiger_stacks, 'r-', linewidth=2, marker='o', markersize=3, 
             label=f'Tiger Looper (SIGEV_THREAD) - {len(tiger_times)} snapshots', alpha=0.8)
    ax1.plot(sw_norm_times, sw_stacks, 'b-', linewidth=2, marker='s', markersize=3,
             label=f'SW Task (timerfd+epoll) - {len(sw_times)} snapshots', alpha=0.8)
    
    ax1.set_title('📊 Stack Memory Comparison: Tiger Looper vs SW Task Framework', fontweight='bold', fontsize=14)
    ax1.set_xlabel(x_label)
    ax1.set_ylabel('Stack Memory Usage (MB)')
    ax1.legend(fontsize=11)
    ax1.grid(True, alpha=0.3)
    
    # Add statistics for stack
    tiger_max_stack = max(tiger_stacks) if tiger_stacks else 0
    sw_max_stack = max(sw_stacks) if sw_stacks else 0
    tiger_avg_stack = sum(tiger_stacks) / len(tiger_stacks) if tiger_stacks else 0
    sw_avg_stack = sum(sw_stacks) / len(sw_stacks) if sw_stacks else 0
    
    stats_text = f'Tiger: Max={tiger_max_stack:.1f}MB, Avg={tiger_avg_stack:.1f}MB\n'
    stats_text += f'SW Task: Max={sw_max_stack:.1f}MB, Avg={sw_avg_stack:.1f}MB\n'
    stats_text += f'Difference: {abs(tiger_max_stack - sw_max_stack):.1f}MB (max)'
    
    ax1.text(0.02, 0.98, stats_text, transform=ax1.transAxes, 
            verticalalignment='top', bbox=dict(boxstyle='round', facecolor='lightcyan', alpha=0.8),
            fontsize=10)
    
    # Plot 2: Heap Memory Comparison
    ax2.plot(tiger_norm_times, tiger_heaps, 'orange', linewidth=2, marker='o', markersize=3,
             label=f'Tiger Looper Heap - {len(tiger_times)} snapshots', alpha=0.8, linestyle='--')
    ax2.plot(sw_norm_times, sw_heaps, 'green', linewidth=2, marker='s', markersize=3,
             label=f'SW Task Heap - {len(sw_times)} snapshots', alpha=0.8, linestyle='--')
    
    ax2.set_title('🗄️  Heap Memory Comparison: Tiger Looper vs SW Task Framework', fontweight='bold', fontsize=14)
    ax2.set_xlabel(x_label)
    ax2.set_ylabel('Heap Memory Usage (MB)')
    ax2.legend(fontsize=11)
    ax2.grid(True, alpha=0.3)
    
    # Add statistics for heap
    tiger_max_heap = max(tiger_heaps) if tiger_heaps else 0
    sw_max_heap = max(sw_heaps) if sw_heaps else 0
    tiger_avg_heap = sum(tiger_heaps) / len(tiger_heaps) if tiger_heaps else 0
    sw_avg_heap = sum(sw_heaps) / len(sw_heaps) if sw_heaps else 0
    
    heap_stats_text = f'Tiger: Max={tiger_max_heap:.1f}MB, Avg={tiger_avg_heap:.1f}MB\n'
    heap_stats_text += f'SW Task: Max={sw_max_heap:.1f}MB, Avg={sw_avg_heap:.1f}MB\n'
    heap_stats_text += f'Difference: {abs(tiger_max_heap - sw_max_heap):.1f}MB (max)'
    
    ax2.text(0.02, 0.98, heap_stats_text, transform=ax2.transAxes, 
            verticalalignment='top', bbox=dict(boxstyle='round', facecolor='lightyellow', alpha=0.8),
            fontsize=10)
    
    plt.tight_layout()
    
    # Save plot
    filename = f"massif_comparison_tiger_vs_swtask_{time_suffix}.png"
    plt.savefig(filename, dpi=300, bbox_inches='tight')
    plt.close()
    
    print(f"✅ Saved comparison plot: {filename}")
    
    # Print summary
    print(f"\n📊 Memory Usage Summary:")
    print(f"{'Framework':<15} {'Snapshots':<10} {'Stack Max':<12} {'Stack Avg':<12} {'Heap Max':<12} {'Heap Avg':<12}")
    print(f"{'='*15:<15} {'='*10:<10} {'='*12:<12} {'='*12:<12} {'='*12:<12} {'='*12:<12}")
    print(f"{'Tiger Looper':<15} {len(tiger_times):<10} {tiger_max_stack:<11.1f}MB {tiger_avg_stack:<11.1f}MB {tiger_max_heap:<11.1f}MB {tiger_avg_heap:<11.1f}MB")
    print(f"{'SW Task':<15} {len(sw_times):<10} {sw_max_stack:<11.1f}MB {sw_avg_stack:<11.1f}MB {sw_max_heap:<11.1f}MB {sw_avg_heap:<11.1f}MB")
    
    stack_diff = tiger_max_stack - sw_max_stack
    heap_diff = tiger_max_heap - sw_max_heap
    snapshot_diff = len(tiger_times) - len(sw_times)
    
    print(f"\n🔍 Analysis:")
    if abs(stack_diff) > 1.0:
        winner = "SW Task" if stack_diff > 0 else "Tiger Looper"
        print(f"📈 Stack Memory: {winner} uses {abs(stack_diff):.1f}MB less (better)")
    else:
        print(f"📈 Stack Memory: Similar usage (difference < 1MB)")
    
    if abs(heap_diff) > 0.1:
        winner = "SW Task" if heap_diff > 0 else "Tiger Looper"
        print(f"🗄️  Heap Memory: {winner} uses {abs(heap_diff):.1f}MB less (better)")
    else:
        print(f"🗄️  Heap Memory: Similar usage (difference < 0.1MB)")
    
    if snapshot_diff != 0:
        longer = "Tiger Looper" if snapshot_diff > 0 else "SW Task"
        print(f"⏱️  Execution: {longer} took {abs(snapshot_diff)} more snapshots (ran longer or more complex)")
    
    return True

def print_help():
    """Print usage help"""
    print("📊 Valgrind Massif Memory Comparison Tool")
    print("Compare memory usage between Tiger Looper (SIGEV_THREAD) and SW Task (timerfd+epoll)")
    print()
    print("Usage:")
    print(f"  {sys.argv[0]} <tiger_massif_file> <sw_massif_file> [time_mode]")
    print()
    print("Parameters:")
    print("  tiger_massif_file   - Valgrind massif output for Tiger Looper")
    print("  sw_massif_file      - Valgrind massif output for SW Task")
    print("  time_mode          - Optional: snapshots | percentage | estimated_seconds")
    print("                       Default: snapshots")
    print()
    print("Time modes:")
    print("  snapshots          - X-axis shows snapshot number (most reliable)")
    print("  percentage         - X-axis shows execution progress 0-100%")
    print("  estimated_seconds  - X-axis shows estimated time in seconds")
    print()
    print("Examples:")
    print(f"  {sys.argv[0]} tiger_massif.out.2963 sw_massif.out.3271")
    print(f"  {sys.argv[0]} tiger_massif.out.2963 sw_massif.out.3271 snapshots")
    print(f"  {sys.argv[0]} tiger_massif.out.2963 sw_massif.out.3271 percentage")
    print(f"  {sys.argv[0]} results/tiger.massif results/sw.massif estimated_seconds")
    print()
    print("Output:")
    print("  - massif_comparison_tiger_vs_swtask_[time_mode].png (comparison chart)")
    print("  - Console summary with memory statistics")
    print()
    print("How to generate massif files:")
    print("  valgrind --tool=massif --stacks=yes ./tiger_timer 10 2")
    print("  valgrind --tool=massif --stacks=yes ./sw_timer 10 2")
    print()
    print("Note:")
    print("  - 'snapshots' mode is recommended (most accurate)")
    print("  - Instruction count != real time")
    print("  - Different frameworks may have different execution patterns")
    print("  - Stack memory shows thread overhead (SIGEV_THREAD vs timerfd)")

def main():
    if len(sys.argv) == 1:
        print_help()
        return 0
    
    if len(sys.argv) < 3:
        print("❌ Error: Missing required arguments")
        print()
        print_help()
        return 1
    
    tiger_file = sys.argv[1]
    sw_file = sys.argv[2]
    time_mode = sys.argv[3] if len(sys.argv) > 3 else "snapshots"
    
    if time_mode not in ["snapshots", "percentage", "estimated_seconds"]:
        print("❌ Error: Invalid time_mode. Use: snapshots | percentage | estimated_seconds")
        return 1
    
    print("🔄 Starting Valgrind Massif comparison...")
    print(f"Tiger Looper file: {tiger_file}")
    print(f"SW Task file: {sw_file}")
    print(f"Time mode: {time_mode}")
    print()
    
    if plot_comparison(tiger_file, sw_file, time_mode):
        print("\n🎉 Comparison completed successfully!")
        return 0
    else:
        print("\n❌ Comparison failed!")
        return 1

if __name__ == "__main__":
    sys.exit(main())