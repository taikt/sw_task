#!/usr/bin/env python3
import json
import sys
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from datetime import datetime

class TopComparisonVisualizer:  # ✅ Đổi tên class từ HtopComparisonVisualizer
    def __init__(self):
        self.sw_data = None
        self.tiger_data = None

    def load_data(self, sw_file, tiger_file):
        """Load monitoring data from JSON files"""
        try:
            with open(sw_file, 'r') as f:
                self.sw_data = json.load(f)
            print(f"✅ Loaded SW Task data: {len(self.sw_data.get('timestamps', []))} samples")
            
            with open(tiger_file, 'r') as f:
                self.tiger_data = json.load(f)
            print(f"✅ Loaded Tiger data: {len(self.tiger_data.get('timestamps', []))} samples")
            
            return True
        except Exception as e:
            print(f"❌ Error loading data: {e}")
            return False

    def create_comparison_plots(self):  # ✅ Đổi tên method
        """Create top-style comparison plots"""
        if not self.sw_data or not self.tiger_data:
            print("❌ No data to plot")
            return False
        
        # Create 2x2 plot layout
        fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(16, 10))
        
        # Plot 1: Memory Usage (RSS)
        self.plot_memory_comparison(ax1)
        
        # Plot 2: CPU Usage
        self.plot_cpu_comparison(ax2)
        
        # Plot 3: Thread Count
        self.plot_thread_comparison(ax3)
        
        # Plot 4: Virtual Memory
        self.plot_virtual_memory_comparison(ax4)
        
        plt.tight_layout()
        
        # Save plot
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"top_comparison_{timestamp}.png"  # ✅ Đổi tên file
        plt.savefig(filename, dpi=300, bbox_inches='tight')
        plt.close()
        
        print(f"✅ Top comparison plot saved: {filename}")
        
        # Print detailed comparison
        self.print_comparison()  # ✅ Đổi tên method
        
        return True

    def plot_memory_comparison(self, ax):
        """Plot memory usage comparison (top RSS style)"""
        sw_times = self.sw_data.get('timestamps', [])
        sw_memory = self.sw_data.get('memory_mb', [])
        tiger_times = self.tiger_data.get('timestamps', [])
        tiger_memory = self.tiger_data.get('memory_mb', [])
        
        ax.plot(sw_times, sw_memory, 'blue', linewidth=2, label='SW Task Memory', marker='o', markersize=3)
        ax.plot(tiger_times, tiger_memory, 'red', linewidth=2, label='Tiger Memory', marker='s', markersize=3)
        
        ax.set_title('Memory Usage Comparison (RSS - top style)', fontweight='bold', fontsize=12)  # ✅ Sửa title
        ax.set_xlabel('Time (seconds)')
        ax.set_ylabel('Memory Usage (MB)')
        ax.legend()
        ax.grid(True, alpha=0.3)
        
        # Add statistics
        sw_max = max(sw_memory) if sw_memory else 0
        tiger_max = max(tiger_memory) if tiger_memory else 0
        sw_avg = np.mean(sw_memory) if sw_memory else 0
        tiger_avg = np.mean(tiger_memory) if tiger_memory else 0
        
        stats_text = f'SW Task: Max={sw_max:.1f}MB, Avg={sw_avg:.1f}MB\n'
        stats_text += f'Tiger: Max={tiger_max:.1f}MB, Avg={tiger_avg:.1f}MB'
        
        ax.text(0.02, 0.98, stats_text, transform=ax.transAxes, 
               verticalalignment='top', bbox=dict(boxstyle='round', facecolor='lightblue', alpha=0.8),
               fontsize=10)

    def plot_cpu_comparison(self, ax):
        """Plot CPU usage comparison (top %CPU style)"""
        sw_times = self.sw_data.get('timestamps', [])
        sw_cpu = self.sw_data.get('cpu_percent', [])
        tiger_times = self.tiger_data.get('timestamps', [])
        tiger_cpu = self.tiger_data.get('cpu_percent', [])
        
        ax.plot(sw_times, sw_cpu, 'green', linewidth=2, label='SW Task CPU%', marker='o', markersize=3)
        ax.plot(tiger_times, tiger_cpu, 'orange', linewidth=2, label='Tiger CPU%', marker='s', markersize=3)
        
        ax.set_title('CPU Usage Comparison (%CPU - top style)', fontweight='bold', fontsize=12)  # ✅ Sửa title
        ax.set_xlabel('Time (seconds)')
        ax.set_ylabel('CPU Usage (%)')
        ax.legend()
        ax.grid(True, alpha=0.3)
        
        # Add statistics
        sw_max = max(sw_cpu) if sw_cpu else 0
        tiger_max = max(tiger_cpu) if tiger_cpu else 0
        sw_avg = np.mean(sw_cpu) if sw_cpu else 0
        tiger_avg = np.mean(tiger_cpu) if tiger_cpu else 0
        
        stats_text = f'SW Task: Max={sw_max:.1f}%, Avg={sw_avg:.1f}%\n'
        stats_text += f'Tiger: Max={tiger_max:.1f}%, Avg={tiger_avg:.1f}%'
        
        ax.text(0.02, 0.98, stats_text, transform=ax.transAxes, 
               verticalalignment='top', bbox=dict(boxstyle='round', facecolor='lightgreen', alpha=0.8),
               fontsize=10)

    def plot_thread_comparison(self, ax):
        """Plot thread count comparison (top THR style)"""
        sw_times = self.sw_data.get('timestamps', [])
        sw_threads = self.sw_data.get('threads', [])
        tiger_times = self.tiger_data.get('timestamps', [])
        tiger_threads = self.tiger_data.get('threads', [])
        
        ax.plot(sw_times, sw_threads, 'purple', linewidth=2, label='SW Task Threads', marker='o', markersize=3)
        ax.plot(tiger_times, tiger_threads, 'brown', linewidth=2, label='Tiger Threads', marker='s', markersize=3)
        
        ax.set_title('Thread Count Comparison (THR - top style)', fontweight='bold', fontsize=12)  # ✅ Sửa title
        ax.set_xlabel('Time (seconds)')
        ax.set_ylabel('Number of Threads')
        ax.legend()
        ax.grid(True, alpha=0.3)
        
        # Add statistics
        sw_max = max(sw_threads) if sw_threads else 0
        tiger_max = max(tiger_threads) if tiger_threads else 0
        sw_avg = np.mean(sw_threads) if sw_threads else 0
        tiger_avg = np.mean(tiger_threads) if tiger_threads else 0
        
        stats_text = f'SW Task: Max={sw_max:.0f}, Avg={sw_avg:.1f}\n'
        stats_text += f'Tiger: Max={tiger_max:.0f}, Avg={tiger_avg:.1f}'
        
        ax.text(0.02, 0.98, stats_text, transform=ax.transAxes, 
               verticalalignment='top', bbox=dict(boxstyle='round', facecolor='plum', alpha=0.8),
               fontsize=10)

    def plot_virtual_memory_comparison(self, ax):
        """Plot virtual memory comparison (top VIRT style)"""
        sw_times = self.sw_data.get('timestamps', [])
        sw_virtual = self.sw_data.get('virtual_mb', [])
        tiger_times = self.tiger_data.get('timestamps', [])
        tiger_virtual = self.tiger_data.get('virtual_mb', [])
        
        ax.plot(sw_times, sw_virtual, 'cyan', linewidth=2, label='SW Task Virtual', marker='o', markersize=3)
        ax.plot(tiger_times, tiger_virtual, 'magenta', linewidth=2, label='Tiger Virtual', marker='s', markersize=3)
        
        ax.set_title('Virtual Memory Comparison (VIRT - top style)', fontweight='bold', fontsize=12)  # ✅ Sửa title
        ax.set_xlabel('Time (seconds)')
        ax.set_ylabel('Virtual Memory (MB)')
        ax.legend()
        ax.grid(True, alpha=0.3)

    def print_comparison(self):  # ✅ Đổi tên method
        """Print top-style comparison summary"""
        print(f"\n📊 Top-style Framework Comparison")  # ✅ Sửa title
        print("=" * 60)
        
        # Memory comparison
        sw_mem = self.sw_data.get('memory_mb', [])
        tiger_mem = self.tiger_data.get('memory_mb', [])
        
        sw_mem_max = max(sw_mem) if sw_mem else 0
        sw_mem_avg = np.mean(sw_mem) if sw_mem else 0
        tiger_mem_max = max(tiger_mem) if tiger_mem else 0
        tiger_mem_avg = np.mean(tiger_mem) if tiger_mem else 0
        
        # CPU comparison
        sw_cpu = self.sw_data.get('cpu_percent', [])
        tiger_cpu = self.tiger_data.get('cpu_percent', [])
        
        sw_cpu_max = max(sw_cpu) if sw_cpu else 0
        sw_cpu_avg = np.mean(sw_cpu) if sw_cpu else 0
        tiger_cpu_max = max(tiger_cpu) if tiger_cpu else 0
        tiger_cpu_avg = np.mean(tiger_cpu) if tiger_cpu else 0
        
        # Thread comparison
        sw_threads = self.sw_data.get('threads', [])
        tiger_threads = self.tiger_data.get('threads', [])
        
        sw_thread_max = max(sw_threads) if sw_threads else 0
        sw_thread_avg = np.mean(sw_threads) if sw_threads else 0
        tiger_thread_max = max(tiger_threads) if tiger_threads else 0
        tiger_thread_avg = np.mean(tiger_threads) if tiger_threads else 0
        
        print(f"🧠 Memory (RSS) Usage:")
        print(f"{'Metric':<15} {'SW Task':<15} {'Tiger':<15} {'Winner':<15}")
        print("-" * 60)
        print(f"{'Max Memory':<15} {sw_mem_max:<14.1f} {tiger_mem_max:<14.1f} {('SW Task' if sw_mem_max < tiger_mem_max else 'Tiger'):<15}")
        print(f"{'Avg Memory':<15} {sw_mem_avg:<14.1f} {tiger_mem_avg:<14.1f} {('SW Task' if sw_mem_avg < tiger_mem_avg else 'Tiger'):<15}")
        
        print(f"\n⚡ CPU Usage:")
        print(f"{'Max CPU':<15} {sw_cpu_max:<14.1f} {tiger_cpu_max:<14.1f} {('SW Task' if sw_cpu_max < tiger_cpu_max else 'Tiger'):<15}")
        print(f"{'Avg CPU':<15} {sw_cpu_avg:<14.1f} {tiger_cpu_avg:<14.1f} {('SW Task' if sw_cpu_avg < tiger_cpu_avg else 'Tiger'):<15}")
        
        print(f"\n🧵 Thread Count:")
        print(f"{'Max Threads':<15} {sw_thread_max:<14.0f} {tiger_thread_max:<14.0f} {('SW Task' if sw_thread_max < tiger_thread_max else 'Tiger'):<15}")
        print(f"{'Avg Threads':<15} {sw_thread_avg:<14.1f} {tiger_thread_avg:<14.1f} {('SW Task' if sw_thread_avg < tiger_thread_avg else 'Tiger'):<15}")
        
        # Overall winner
        sw_wins = 0
        tiger_wins = 0
        
        if sw_mem_avg < tiger_mem_avg: sw_wins += 1
        else: tiger_wins += 1
        
        if sw_cpu_avg < tiger_cpu_avg: sw_wins += 1
        else: tiger_wins += 1
        
        if sw_thread_avg < tiger_thread_avg: sw_wins += 1
        else: tiger_wins += 1
        
        print(f"\n🏆 Overall Winner: {'SW Task' if sw_wins > tiger_wins else 'Tiger'} ({max(sw_wins, tiger_wins)}/3 metrics)")
        
        # Efficiency analysis
        print(f"\n📈 Efficiency Analysis:")
        if sw_mem_avg > 0 and tiger_mem_avg > 0:
            mem_ratio = tiger_mem_avg / sw_mem_avg
            print(f"   Memory: Tiger uses {mem_ratio:.1f}x memory compared to SW Task")
        
        if sw_cpu_avg > 0 and tiger_cpu_avg > 0:
            cpu_ratio = tiger_cpu_avg / sw_cpu_avg
            print(f"   CPU: Tiger uses {cpu_ratio:.1f}x CPU compared to SW Task")

def main():
    if len(sys.argv) != 3:
        print("Top-style Framework Comparison Tool")  # ✅ Sửa title
        print(f"Usage: {sys.argv[0]} <sw_result.json> <tiger_result.json>")  # ✅ Sửa tên file
        print()
        print("Compare memory, CPU, and thread usage using top-style monitoring")  # ✅ Sửa description
        print()
        print("Example:")
        print(f"  {sys.argv[0]} sw_result.json tiger_result.json")  # ✅ Sửa tên file
        return 1
    
    sw_file = sys.argv[1]
    tiger_file = sys.argv[2]
    
    print("🔄 Starting top-style comparison...")  # ✅ Sửa title
    print(f"SW Task file: {sw_file}")
    print(f"Tiger file: {tiger_file}")
    
    visualizer = TopComparisonVisualizer()  # ✅ Dùng class mới
    
    if not visualizer.load_data(sw_file, tiger_file):
        return 1
    
    if visualizer.create_comparison_plots():  # ✅ Dùng method mới
        print("\n🎉 Top-style comparison completed successfully!")
        return 0
    else:
        print("\n❌ Comparison failed!")
        return 1

if __name__ == "__main__":
    sys.exit(main())