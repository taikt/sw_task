#!/usr/bin/env python3
import pandas as pd
import matplotlib.pyplot as plt
import glob
import os
import sys
from datetime import datetime

def find_latest_results():
    """Tìm thư mục kết quả mới nhất với thông tin chi tiết"""
    sw_dirs = glob.glob('result_*')
    tiger_dirs = glob.glob('tiger_result_*')

    # Sort by creation time để lấy newest
    sw_dirs = sorted(sw_dirs, key=os.path.getctime, reverse=True)
    tiger_dirs = sorted(tiger_dirs, key=os.path.getctime, reverse=True)
    
    print(f"\n🔍 Found result directories:")
    print(f"📁 SW Task results: {len(sw_dirs)} folders")
    for i, dir_name in enumerate(sw_dirs[:3]):  # Show top 3
        ctime = datetime.fromtimestamp(os.path.getctime(dir_name))
        print(f"   {i+1}. {dir_name} (created: {ctime.strftime('%Y-%m-%d %H:%M:%S')})")
    
    print(f"📁 Tiger results: {len(tiger_dirs)} folders")  
    for i, dir_name in enumerate(tiger_dirs[:3]):  # Show top 3
        ctime = datetime.fromtimestamp(os.path.getctime(dir_name))
        print(f"   {i+1}. {dir_name} (created: {ctime.strftime('%Y-%m-%d %H:%M:%S')})")

    sw_dir = sw_dirs[0] if sw_dirs else None
    tiger_dir = tiger_dirs[0] if tiger_dirs else None

    return sw_dir, tiger_dir, sw_dirs, tiger_dirs

def load_data_safely(file_path):
    """Load CSV data với error handling - giống file gốc"""
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

def create_memory_comparison(sw_process, tiger_process):
    """Tạo biểu đồ Memory với style từ compare.py gốc"""
    
    # Setup matplotlib giống file gốc
    plt.style.use('default')
    fig, ax = plt.subplots(figsize=(12, 8))
    
    # Colors giống file gốc - đơn giản và đẹp hơn
    sw_color = '#1f77b4'    # Blue (default matplotlib blue)
    tiger_color = '#ff7f0e' # Orange (default matplotlib orange)
    
    # Track if we have any data to plot
    has_data = False
    
    # Plot SW Task data giống file gốc
    if sw_process is not None and 'memory_mb' in sw_process.columns:
        has_data = True
        ax.plot(sw_process['timestamp'], sw_process['memory_mb'], 
               color=sw_color, label='SW Task Framework', linewidth=2)
    
    # Plot Tiger Looper data với dashed line giống file gốc
    if tiger_process is not None and 'memory_mb' in tiger_process.columns:
        has_data = True
        ax.plot(tiger_process['timestamp'], tiger_process['memory_mb'], 
               color=tiger_color, label='Tiger Looper Framework', 
               linewidth=2, linestyle='--')
    
    if not has_data:
        print("❌ Không có dữ liệu memory để plot!")
        return
    
    # Formatting giống file gốc - đơn giản và sạch sẽ
    ax.set_title('Memory Usage Comparison: SW Task vs Tiger Looper', 
                fontsize=14, fontweight='bold', pad=20)
    ax.set_xlabel('Time (seconds)', fontsize=12)
    ax.set_ylabel('Memory Usage (MB)', fontsize=12)
    ax.grid(True, alpha=0.3)
    
    # Legend giống file gốc - đơn giản
    if ax.get_lines():
        ax.legend(fontsize=11, loc='best')
    
    # Statistics box theo style file gốc
    stats_lines = []
    if sw_process is not None and 'memory_mb' in sw_process.columns:
        sw_avg = sw_process['memory_mb'].mean()
        sw_peak = sw_process['memory_mb'].max()
        sw_min = sw_process['memory_mb'].min()
        sw_growth = sw_peak - sw_min
        
        stats_lines.append("SW Task:")
        stats_lines.append(f"  Avg: {sw_avg:.1f}MB")
        stats_lines.append(f"  Peak: {sw_peak:.1f}MB")
        stats_lines.append(f"  Growth: {sw_growth:.1f}MB")
    
    if tiger_process is not None and 'memory_mb' in tiger_process.columns:
        tiger_avg = tiger_process['memory_mb'].mean()
        tiger_peak = tiger_process['memory_mb'].max() 
        tiger_min = tiger_process['memory_mb'].min()
        tiger_growth = tiger_peak - tiger_min
        
        if stats_lines:
            stats_lines.append("")
        stats_lines.append("Tiger Looper:")
        stats_lines.append(f"  Avg: {tiger_avg:.1f}MB")
        stats_lines.append(f"  Peak: {tiger_peak:.1f}MB") 
        stats_lines.append(f"  Growth: {tiger_growth:.1f}MB")
    
    # Add statistics text box - style đơn giản hơn
    if stats_lines:
        stats_text = '\n'.join(stats_lines)
        ax.text(0.02, 0.98, stats_text, transform=ax.transAxes,
               verticalalignment='top', fontsize=10, fontfamily='monospace',
               bbox=dict(boxstyle='round,pad=0.4', facecolor='white', 
                        alpha=0.8, edgecolor='lightgray'))
    
    # Save với style giống file gốc
    plt.tight_layout()
    plt.savefig('memory_comparison.png', dpi=300, bbox_inches='tight')
    print("✅ Saved: memory_comparison.png")
    
    # Close để tránh overlap
    plt.close()

def create_cpu_comparison(sw_process, tiger_process):
    """Tạo biểu đồ CPU với style từ compare.py gốc"""
    
    plt.style.use('default')
    fig, ax = plt.subplots(figsize=(12, 8))
    
    # Colors giống file gốc
    sw_color = '#1f77b4'    # Blue
    tiger_color = '#ff7f0e' # Orange
    
    has_data = False
    
    # Plot SW Task data
    if sw_process is not None and 'cpu_percent' in sw_process.columns:
        has_data = True
        ax.plot(sw_process['timestamp'], sw_process['cpu_percent'], 
               color=sw_color, label='SW Task Framework', linewidth=2)
    
    # Plot Tiger data với dashed line
    if tiger_process is not None and 'cpu_percent' in tiger_process.columns:
        has_data = True
        ax.plot(tiger_process['timestamp'], tiger_process['cpu_percent'], 
               color=tiger_color, label='Tiger Looper Framework', 
               linewidth=2, linestyle='--')
    
    if not has_data:
        print("❌ Không có dữ liệu CPU để plot!")
        return
    
    # Formatting giống file gốc
    ax.set_title('CPU Usage Comparison: SW Task vs Tiger Looper', 
                fontsize=14, fontweight='bold', pad=20)
    ax.set_xlabel('Time (seconds)', fontsize=12)
    ax.set_ylabel('CPU Usage (%)', fontsize=12)
    ax.grid(True, alpha=0.3)
    
    # Legend
    if ax.get_lines():
        ax.legend(fontsize=11, loc='best')
    
    # Statistics box
    stats_lines = []
    if sw_process is not None and 'cpu_percent' in sw_process.columns:
        sw_avg = sw_process['cpu_percent'].mean()
        sw_peak = sw_process['cpu_percent'].max()
        sw_min = sw_process['cpu_percent'].min()
        
        stats_lines.append("SW Task:")
        stats_lines.append(f"  Avg: {sw_avg:.1f}%")
        stats_lines.append(f"  Peak: {sw_peak:.1f}%")
        stats_lines.append(f"  Min: {sw_min:.1f}%")
    
    if tiger_process is not None and 'cpu_percent' in tiger_process.columns:
        tiger_avg = tiger_process['cpu_percent'].mean()
        tiger_peak = tiger_process['cpu_percent'].max()
        tiger_min = tiger_process['cpu_percent'].min()
        
        if stats_lines:
            stats_lines.append("")
        stats_lines.append("Tiger Looper:")
        stats_lines.append(f"  Avg: {tiger_avg:.1f}%")
        stats_lines.append(f"  Peak: {tiger_peak:.1f}%")
        stats_lines.append(f"  Min: {tiger_min:.1f}%")
    
    if stats_lines:
        stats_text = '\n'.join(stats_lines)
        ax.text(0.02, 0.98, stats_text, transform=ax.transAxes,
               verticalalignment='top', fontsize=10, fontfamily='monospace',
               bbox=dict(boxstyle='round,pad=0.4', facecolor='white', 
                        alpha=0.8, edgecolor='lightgray'))
    
    plt.tight_layout()
    plt.savefig('cpu_comparison.png', dpi=300, bbox_inches='tight')
    print("✅ Saved: cpu_comparison.png")
    plt.close()

def interactive_directory_selection(sw_dirs, tiger_dirs):
    """Cho phép user chọn thư mục cụ thể nếu có nhiều options"""
    if len(sw_dirs) <= 1 and len(tiger_dirs) <= 1:
        return sw_dirs[0] if sw_dirs else None, tiger_dirs[0] if tiger_dirs else None
    
    print(f"\n🤔 Multiple result directories found. Choose which ones to compare:")
    
    # SW Task selection
    sw_choice = None
    if len(sw_dirs) > 1:
        print(f"\n📋 SW Task directories:")
        for i, dir_name in enumerate(sw_dirs):
            ctime = datetime.fromtimestamp(os.path.getctime(dir_name))
            print(f"   {i+1}. {dir_name} ({ctime.strftime('%Y-%m-%d %H:%M:%S')})")
        
        while True:
            try:
                choice = input(f"Choose SW Task directory (1-{len(sw_dirs)}, or 0 to skip): ")
                choice = int(choice)
                if choice == 0:
                    sw_choice = None
                    break
                elif 1 <= choice <= len(sw_dirs):
                    sw_choice = sw_dirs[choice-1]
                    break
                else:
                    print("Invalid choice!")
            except ValueError:
                print("Please enter a number!")
    else:
        sw_choice = sw_dirs[0] if sw_dirs else None
    
    # Tiger selection
    tiger_choice = None
    if len(tiger_dirs) > 1:
        print(f"\n📋 Tiger Looper directories:")
        for i, dir_name in enumerate(tiger_dirs):
            ctime = datetime.fromtimestamp(os.path.getctime(dir_name))
            print(f"   {i+1}. {dir_name} ({ctime.strftime('%Y-%m-%d %H:%M:%S')})")
        
        while True:
            try:
                choice = input(f"Choose Tiger directory (1-{len(tiger_dirs)}, or 0 to skip): ")
                choice = int(choice)
                if choice == 0:
                    tiger_choice = None
                    break
                elif 1 <= choice <= len(tiger_dirs):
                    tiger_choice = tiger_dirs[choice-1]
                    break
                else:
                    print("Invalid choice!")
            except ValueError:
                print("Please enter a number!")
    else:
        tiger_choice = tiger_dirs[0] if tiger_dirs else None
    
    return sw_choice, tiger_choice

def print_comparison_statistics(sw_process, tiger_process):
    """In thống kê so sánh chi tiết - giống file gốc"""
    print("\n" + "=" * 60)
    print("📊 DETAILED COMPARISON STATISTICS")
    print("=" * 60)
    
    if sw_process is not None:
        print(f"\n🔷 SW TASK FRAMEWORK:")
        if 'cpu_percent' in sw_process.columns:
            print(f"  CPU - Average: {sw_process['cpu_percent'].mean():.2f}%, Peak: {sw_process['cpu_percent'].max():.2f}%")
        if 'memory_mb' in sw_process.columns:
            print(f"  Memory - Average: {sw_process['memory_mb'].mean():.2f}MB, Peak: {sw_process['memory_mb'].max():.2f}MB")
        if 'timestamp' in sw_process.columns:
            print(f"  Duration: {sw_process['timestamp'].max():.1f} seconds")
        print(f"  Data Points: {len(sw_process)} samples")
    
    if tiger_process is not None:
        print(f"\n🔶 TIGER LOOPER FRAMEWORK:")
        if 'cpu_percent' in tiger_process.columns:
            print(f"  CPU - Average: {tiger_process['cpu_percent'].mean():.2f}%, Peak: {tiger_process['cpu_percent'].max():.2f}%")
        if 'memory_mb' in tiger_process.columns:
            print(f"  Memory - Average: {tiger_process['memory_mb'].mean():.2f}MB, Peak: {tiger_process['memory_mb'].max():.2f}MB")
        if 'timestamp' in tiger_process.columns:
            print(f"  Duration: {tiger_process['timestamp'].max():.1f} seconds")
        print(f"  Data Points: {len(tiger_process)} samples")
    
    # Direct comparison
    if sw_process is not None and tiger_process is not None:
        print(f"\n📈 DIRECT COMPARISON:")
        
        # CPU comparison
        if 'cpu_percent' in sw_process.columns and 'cpu_percent' in tiger_process.columns:
            sw_avg_cpu = sw_process['cpu_percent'].mean()
            tiger_avg_cpu = tiger_process['cpu_percent'].mean()
            cpu_diff = ((tiger_avg_cpu - sw_avg_cpu) / sw_avg_cpu) * 100 if sw_avg_cpu > 0 else 0
            print(f"  CPU Usage: Tiger Looper is {cpu_diff:+.1f}% vs SW Task")
        
        # Memory comparison  
        if 'memory_mb' in sw_process.columns and 'memory_mb' in tiger_process.columns:
            sw_avg_mem = sw_process['memory_mb'].mean()
            tiger_avg_mem = tiger_process['memory_mb'].mean()
            mem_diff = ((tiger_avg_mem - sw_avg_mem) / sw_avg_mem) * 100 if sw_avg_mem > 0 else 0
            print(f"  Memory Usage: Tiger Looper is {mem_diff:+.1f}% vs SW Task")
        
        # Performance verdict giống file gốc
        print(f"\n🎯 VERDICT:")
        
        if 'cpu_percent' in sw_process.columns and 'cpu_percent' in tiger_process.columns:
            if abs(cpu_diff) < 5:
                cpu_verdict = "✅ Similar CPU performance"
            elif cpu_diff < 0:
                cpu_verdict = f"✅ Tiger Looper is {abs(cpu_diff):.1f}% more CPU efficient"
            else:
                cpu_verdict = f"⚠️  Tiger Looper uses {cpu_diff:.1f}% more CPU"
            print(f"  {cpu_verdict}")
        
        if 'memory_mb' in sw_process.columns and 'memory_mb' in tiger_process.columns:
            if abs(mem_diff) < 5:
                mem_verdict = "✅ Similar memory usage"
            elif mem_diff < 0:
                mem_verdict = f"✅ Tiger Looper uses {abs(mem_diff):.1f}% less memory"
            else:
                mem_verdict = f"⚠️  Tiger Looper uses {mem_diff:.1f}% more memory"
            print(f"  {mem_verdict}")

def main():
    print("🔍 Framework Performance Comparison Tool")
    print("=" * 50)
    
    # Tìm thư mục kết quả mới nhất
    sw_dir, tiger_dir, sw_dirs, tiger_dirs = find_latest_results()
    
    if not sw_dirs and not tiger_dirs:
        print("❌ Không tìm thấy dữ liệu nào!")
        print("Hãy chạy:")
        print("  ./monitor_test.sh program 200 30 60")
        print("  # Rename result to tiger_result_* for Tiger framework")
        sys.exit(1)
    
    # Interactive selection nếu có nhiều directories
    sw_dir, tiger_dir = interactive_directory_selection(sw_dirs, tiger_dirs)
    
    print(f"\n📂 Selected directories:")
    print(f"🔷 SW Task: {sw_dir}")
    print(f"🔶 Tiger: {tiger_dir}")
    
    # Load dữ liệu process
    sw_process = None
    tiger_process = None
    
    if sw_dir:
        sw_process = load_data_safely(f'{sw_dir}/process_stats.csv')
    
    if tiger_dir:
        tiger_process = load_data_safely(f'{tiger_dir}/process_stats.csv')
    
    if sw_process is None and tiger_process is None:
        print("❌ Không có dữ liệu process hợp lệ!")
        sys.exit(1)
    
    # Tạo comparison charts với style từ file gốc
    print("\n📊 Creating performance comparison charts...")
    
    # CPU comparison chart
    if (sw_process is not None and 'cpu_percent' in sw_process.columns) or \
       (tiger_process is not None and 'cpu_percent' in tiger_process.columns):
        print("📈 Creating CPU usage comparison...")
        create_cpu_comparison(sw_process, tiger_process)
    
    # Memory comparison chart  
    if (sw_process is not None and 'memory_mb' in sw_process.columns) or \
       (tiger_process is not None and 'memory_mb' in tiger_process.columns):
        print("📈 Creating Memory usage comparison...")
        create_memory_comparison(sw_process, tiger_process)
    
    # Print detailed statistics
    print_comparison_statistics(sw_process, tiger_process)
    
    print(f"\n🎉 Analysis completed!")
    print(f"📊 Generated charts:")
    print(f"  - cpu_comparison.png: CPU usage comparison")
    print(f"  - memory_comparison.png: Memory usage comparison")

if __name__ == "__main__":
    main()