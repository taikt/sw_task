#!/usr/bin/env python3
import pandas as pd
import matplotlib.pyplot as plt
import glob
import os
import sys

def find_latest_results():
    """Tìm thư mục kết quả mới nhất"""
    sw_dirs = glob.glob('result_*')
    tiger_dirs = glob.glob('tiger_result_*')

    sw_dir = max(sw_dirs, key=os.path.getctime) if sw_dirs else None
    tiger_dir = max(tiger_dirs, key=os.path.getctime) if tiger_dirs else None

    return sw_dir, tiger_dir

def load_data_safely(file_path):
    """Load CSV data với error handling"""
    try:
        if os.path.exists(file_path):
            df = pd.read_csv(file_path)
            # Convert to numeric, coerce errors to NaN
            for col in df.columns:
                if col != 'timestamp':
                    df[col] = pd.to_numeric(df[col], errors='coerce')
            # Drop rows with NaN values
            df = df.dropna()
            return df if not df.empty else None
        else:
            print(f"⚠️  File not found: {file_path}")
            return None
    except Exception as e:
        print(f"❌ Error loading {file_path}: {e}")
        return None

def main():
    print("🔍 Framework Performance Comparison Tool")
    print("=" * 50)
    
    # Tìm thư mục kết quả mới nhất
    sw_dir, tiger_dir = find_latest_results()
    
    print(f"🔷 SW Task: {sw_dir}")
    print(f"🔶 Tiger: {tiger_dir}")
    
    if not sw_dir and not tiger_dir:
        print("❌ Không tìm thấy dữ liệu nào!")
        print("Hãy chạy:")
        print("  ./monitor_test.sh program 1000 100 30")
        print("  ./tiger_monitor_test.sh program 1000 100 30")
        sys.exit(1)
    
    # Load dữ liệu
    sw_process = None
    sw_system = None
    tiger_process = None
    tiger_system = None
    
    if sw_dir:
        sw_process = load_data_safely(f'{sw_dir}/process_stats.csv')
        sw_system = load_data_safely(f'{sw_dir}/system_load.csv')
    
    if tiger_dir:
        tiger_process = load_data_safely(f'{tiger_dir}/process_stats.csv')
        tiger_system = load_data_safely(f'{tiger_dir}/system_load.csv')
    
    if sw_process is None and tiger_process is None:
        print("❌ Không có dữ liệu process hợp lệ!")
        sys.exit(1)
    
    # Tạo comparison chart
    print("\n📊 Creating comparison chart...")
    
    # Setup matplotlib
    plt.style.use('default')
    fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(15, 10))
    fig.suptitle('SW Task vs Tiger Looper Performance Comparison', fontsize=16, fontweight='bold')
    
    # Colors
    sw_color = '#1f77b4'    # Blue
    tiger_color = '#ff7f0e' # Orange
    
    # Track if we have any data to plot
    has_data = False
    
    # Plot SW Task data
    if sw_process is not None:
        print("📈 Plotting SW Task data...")
        has_data = True
        ax1.plot(sw_process['timestamp'], sw_process['cpu_percent'], 
                color=sw_color, label='SW Task', linewidth=2)
        ax2.plot(sw_process['timestamp'], sw_process['memory_mb'], 
                color=sw_color, label='SW Task', linewidth=2)
        
        if 'threads' in sw_process.columns:
            ax3.plot(sw_process['timestamp'], sw_process['threads'], 
                    color=sw_color, label='SW Task', linewidth=2)
        
        if sw_system is not None and 'load_1min' in sw_system.columns:
            ax4.plot(sw_system['timestamp'], sw_system['load_1min'], 
                    color=sw_color, label='SW Task', linewidth=2)
    
    # Plot Tiger data
    if tiger_process is not None:
        print("📈 Plotting Tiger Looper data...")
        has_data = True
        ax1.plot(tiger_process['timestamp'], tiger_process['cpu_percent'], 
                color=tiger_color, label='Tiger Looper', linewidth=2, linestyle='--')
        ax2.plot(tiger_process['timestamp'], tiger_process['memory_mb'], 
                color=tiger_color, label='Tiger Looper', linewidth=2, linestyle='--')
        
        if 'threads' in tiger_process.columns:
            ax3.plot(tiger_process['timestamp'], tiger_process['threads'], 
                    color=tiger_color, label='Tiger Looper', linewidth=2, linestyle='--')
        
        if tiger_system is not None and 'load_1min' in tiger_system.columns:
            ax4.plot(tiger_system['timestamp'], tiger_system['load_1min'], 
                    color=tiger_color, label='Tiger Looper', linewidth=2, linestyle='--')
    
    if not has_data:
        print("❌ Không có dữ liệu để plot!")
        return
    
    # Formatting - chỉ thêm legend nếu có data
    ax1.set_title('CPU Usage (%)', fontweight='bold')
    ax1.set_xlabel('Time (seconds)')
    ax1.set_ylabel('CPU %')
    ax1.grid(True, alpha=0.3)
    # Chỉ thêm legend nếu có lines
    if ax1.get_lines():
        ax1.legend()
    
    ax2.set_title('Memory Usage (MB)', fontweight='bold')
    ax2.set_xlabel('Time (seconds)')
    ax2.set_ylabel('Memory (MB)')
    ax2.grid(True, alpha=0.3)
    if ax2.get_lines():
        ax2.legend()
    
    ax3.set_title('Thread Count', fontweight='bold')
    ax3.set_xlabel('Time (seconds)')
    ax3.set_ylabel('Threads')
    ax3.grid(True, alpha=0.3)
    if ax3.get_lines():
        ax3.legend()
    
    ax4.set_title('System Load (1min avg)', fontweight='bold')
    ax4.set_xlabel('Time (seconds)')
    ax4.set_ylabel('Load Average')
    ax4.grid(True, alpha=0.3)
    if ax4.get_lines():
        ax4.legend()
    
    # Save chart
    plt.tight_layout()
    plt.savefig('comparison.png', dpi=300, bbox_inches='tight')
    print("✅ Saved: comparison.png")
    
    # Print detailed statistics
    print("\n" + "=" * 60)
    print("📊 DETAILED COMPARISON STATISTICS")
    print("=" * 60)
    
    if sw_process is not None:
        print(f"\n🔷 SW TASK FRAMEWORK:")
        print(f"  CPU - Average: {sw_process['cpu_percent'].mean():.2f}%, Peak: {sw_process['cpu_percent'].max():.2f}%")
        print(f"  Memory - Average: {sw_process['memory_mb'].mean():.2f}MB, Peak: {sw_process['memory_mb'].max():.2f}MB")
        if 'threads' in sw_process.columns:
            print(f"  Threads - Max: {sw_process['threads'].max():.0f}")
        print(f"  Duration: {sw_process['timestamp'].max():.1f} seconds")
    
    if tiger_process is not None:
        print(f"\n🔶 TIGER LOOPER FRAMEWORK:")
        print(f"  CPU - Average: {tiger_process['cpu_percent'].mean():.2f}%, Peak: {tiger_process['cpu_percent'].max():.2f}%")
        print(f"  Memory - Average: {tiger_process['memory_mb'].mean():.2f}MB, Peak: {tiger_process['memory_mb'].max():.2f}MB")
        if 'threads' in tiger_process.columns:
            print(f"  Threads - Max: {tiger_process['threads'].max():.0f}")
        print(f"  Duration: {tiger_process['timestamp'].max():.1f} seconds")
    
    # Direct comparison
    if sw_process is not None and tiger_process is not None:
        print(f"\n📈 DIRECT COMPARISON:")
        
        sw_avg_cpu = sw_process['cpu_percent'].mean()
        tiger_avg_cpu = tiger_process['cpu_percent'].mean()
        cpu_diff = ((tiger_avg_cpu - sw_avg_cpu) / sw_avg_cpu) * 100 if sw_avg_cpu > 0 else 0
        
        sw_avg_mem = sw_process['memory_mb'].mean()
        tiger_avg_mem = tiger_process['memory_mb'].mean()
        mem_diff = ((tiger_avg_mem - sw_avg_mem) / sw_avg_mem) * 100 if sw_avg_mem > 0 else 0
        
        print(f"  CPU Usage: Tiger Looper is {cpu_diff:+.1f}% vs SW Task")
        print(f"  Memory Usage: Tiger Looper is {mem_diff:+.1f}% vs SW Task")
        
        # Performance verdict
        if abs(cpu_diff) < 5:
            cpu_verdict = "✅ Similar CPU performance"
        elif cpu_diff < 0:
            cpu_verdict = f"✅ Tiger Looper is {abs(cpu_diff):.1f}% more CPU efficient"
        else:
            cpu_verdict = f"⚠️  Tiger Looper uses {cpu_diff:.1f}% more CPU"
        
        if abs(mem_diff) < 5:
            mem_verdict = "✅ Similar memory usage"
        elif mem_diff < 0:
            mem_verdict = f"✅ Tiger Looper uses {abs(mem_diff):.1f}% less memory"
        else:
            mem_verdict = f"⚠️  Tiger Looper uses {mem_diff:.1f}% more memory"
        
        print(f"\n🎯 VERDICT:")
        print(f"  {cpu_verdict}")
        print(f"  {mem_verdict}")
    
    print(f"\n🎉 Analysis completed!")
    print(f"📈 Check visualization: comparison.png")
    
    # Hiển thị chart nếu có thể
    try:
        plt.show()
    except:
        pass

if __name__ == "__main__":
    main()