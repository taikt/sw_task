#!/usr/bin/env python3
import pandas as pd
import matplotlib.pyplot as plt
import sys
import os
import glob

def clean_csv_file(file_path):
    """Clean malformed CSV file"""
    if not os.path.exists(file_path):
        return None
        
    print(f"Cleaning CSV file: {file_path}")
    
    with open(file_path, 'r') as f:
        lines = f.readlines()
    
    # Clean lines - remove standalone numbers and malformed lines
    cleaned_lines = []
    header_added = False
    
    for line in lines:
        line = line.strip()
        if not line:
            continue
            
        # Skip standalone numbers
        if line.isdigit():
            continue
            
        # Check if line has proper CSV format (contains commas)
        if ',' in line:
            # Count commas to ensure proper format
            if not header_added:
                cleaned_lines.append(line)
                header_added = True
                expected_commas = line.count(',')
            else:
                if line.count(',') == expected_commas:
                    cleaned_lines.append(line)
                else:
                    print(f"Skipping malformed line: {line}")
    
    # Write cleaned file
    cleaned_file = file_path + '.cleaned'
    with open(cleaned_file, 'w') as f:
        f.write('\n'.join(cleaned_lines))
    
    return cleaned_file

def find_csv_files():
    """Find CSV files in current directory or subdirectories"""
    csv_files = {}
    
    # Look for CSV files in current directory first
    for pattern in ['process_stats.csv', 'system_load.csv']:
        files = glob.glob(pattern)
        if files:
            csv_files[pattern] = files[0]
    
    # If not found, look in performance_results directories
    if not csv_files:
        perf_dirs = glob.glob('performance_results_*')
        if perf_dirs:
            # Use the most recent performance_results directory
            latest_dir = max(perf_dirs, key=os.path.getctime)
            print(f"Using data from: {latest_dir}")
            
            for pattern in ['process_stats.csv', 'system_load.csv']:
                file_path = os.path.join(latest_dir, pattern)
                if os.path.exists(file_path):
                    csv_files[pattern] = file_path
    
    return csv_files

def plot_process_stats(csv_file):
    """Plot process-specific statistics"""
    if not csv_file or not os.path.exists(csv_file):
        print(f"process_stats.csv not found at: {csv_file}")
        return
    
    # Clean the CSV file first
    cleaned_file = clean_csv_file(csv_file)
    if not cleaned_file:
        print(f"Failed to clean CSV file: {csv_file}")
        return
        
    try:
        print(f"Reading process stats from: {cleaned_file}")
        df = pd.read_csv(cleaned_file)
        
        # Ensure numeric columns
        numeric_cols = ['timestamp', 'cpu_percent', 'memory_mb', 'threads']
        for col in numeric_cols:
            if col in df.columns:
                df[col] = pd.to_numeric(df[col], errors='coerce')
        
        if 'context_switches' in df.columns:
            df['context_switches'] = pd.to_numeric(df['context_switches'], errors='coerce')
        
        # Drop rows with NaN values
        df = df.dropna()
        
        if df.empty:
            print("No valid data found in process stats")
            return None
            
        fig, axes = plt.subplots(2, 2, figsize=(15, 10))
        fig.suptitle('Timer Load Test - Process Statistics')
        
        # CPU usage
        axes[0,0].plot(df['timestamp'], df['cpu_percent'])
        axes[0,0].set_title('CPU Usage (%)')
        axes[0,0].set_xlabel('Time (seconds)')
        axes[0,0].set_ylabel('CPU %')
        axes[0,0].grid(True)
        
        # Memory usage
        axes[0,1].plot(df['timestamp'], df['memory_mb'], color='red')
        axes[0,1].set_title('Memory Usage (MB)')
        axes[0,1].set_xlabel('Time (seconds)')
        axes[0,1].set_ylabel('Memory (MB)')
        axes[0,1].grid(True)
        
        # Thread count
        axes[1,0].plot(df['timestamp'], df['threads'], color='green')
        axes[1,0].set_title('Thread Count')
        axes[1,0].set_xlabel('Time (seconds)')
        axes[1,0].set_ylabel('Threads')
        axes[1,0].grid(True)
        
        # Context switches
        if 'context_switches' in df.columns and not df['context_switches'].isna().all():
            axes[1,1].plot(df['timestamp'], df['context_switches'], color='orange')
            axes[1,1].set_title('Context Switches')
            axes[1,1].set_xlabel('Time (seconds)')
            axes[1,1].set_ylabel('Context Switches')
            axes[1,1].grid(True)
        else:
            axes[1,1].text(0.5, 0.5, 'Context switches\ndata not available', 
                          ha='center', va='center', transform=axes[1,1].transAxes)
            axes[1,1].set_title('Context Switches (N/A)')
        
        plt.tight_layout()
        plt.savefig('process_stats.png', dpi=300, bbox_inches='tight')
        print("Saved: process_stats.png")
        
        return df
        
    except Exception as e:
        print(f"Error processing process stats: {e}")
        return None

def plot_system_load(csv_file):
    """Plot system load statistics"""
    if not csv_file or not os.path.exists(csv_file):
        print(f"system_load.csv not found at: {csv_file}")
        return
    
    # Clean the CSV file first
    cleaned_file = clean_csv_file(csv_file)
    if not cleaned_file:
        print(f"Failed to clean CSV file: {csv_file}")
        return
        
    try:
        print(f"Reading system load from: {cleaned_file}")
        df = pd.read_csv(cleaned_file)
        
        # Ensure numeric columns
        numeric_cols = ['timestamp', 'load_1min', 'load_5min', 'load_15min']
        for col in numeric_cols:
            if col in df.columns:
                df[col] = pd.to_numeric(df[col], errors='coerce')
        
        # Drop rows with NaN values
        df = df.dropna()
        
        if df.empty:
            print("No valid data found in system load")
            return None
        
        plt.figure(figsize=(12, 6))
        plt.plot(df['timestamp'], df['load_1min'], label='1 min')
        plt.plot(df['timestamp'], df['load_5min'], label='5 min') 
        plt.plot(df['timestamp'], df['load_15min'], label='15 min')
        
        plt.title('System Load Average')
        plt.xlabel('Time (seconds)')
        plt.ylabel('Load Average')
        plt.legend()
        plt.grid(True)
        
        plt.savefig('system_load.png', dpi=300, bbox_inches='tight')
        print("Saved: system_load.png")
        
        return df
        
    except Exception as e:
        print(f"Error processing system load: {e}")
        return None

def print_statistics(process_df, system_df):
    """Print summary statistics"""
    if process_df is not None and not process_df.empty:
        print("\nProcess Statistics Summary:")
        print(f"Peak CPU: {process_df['cpu_percent'].max():.2f}%")
        print(f"Average CPU: {process_df['cpu_percent'].mean():.2f}%")
        print(f"Peak Memory: {process_df['memory_mb'].max():.2f} MB")
        print(f"Average Memory: {process_df['memory_mb'].mean():.2f} MB")
        print(f"Max Threads: {process_df['threads'].max():.0f}")
        
        if 'context_switches' in process_df.columns and not process_df['context_switches'].isna().all():
            print(f"Max Context Switches: {process_df['context_switches'].max():.0f}")
    
    if system_df is not None and not system_df.empty:
        print("\nSystem Load Summary:")
        print(f"Peak 1-min load: {system_df['load_1min'].max():.2f}")
        print(f"Average 1-min load: {system_df['load_1min'].mean():.2f}")

def main():
    """Main function with command line argument support"""
    print("Analyzing timer load test results...")
    print(f"Current directory: {os.getcwd()}")
    
    # Check for command line argument for custom directory
    if len(sys.argv) > 1:
        target_dir = sys.argv[1]
        if os.path.exists(target_dir):
            os.chdir(target_dir)
            print(f"Changed to directory: {target_dir}")
        else:
            print(f"Directory not found: {target_dir}")
            return
    
    # Find CSV files
    csv_files = find_csv_files()
    
    if not csv_files:
        print("No CSV files found in current directory or performance_results_* subdirectories")
        print("Available files:")
        for f in os.listdir('.'):
            if f.endswith('.csv'):
                print(f"  {f}")
        return
    
    # Plot data
    process_df = plot_process_stats(csv_files.get('process_stats.csv'))
    system_df = plot_system_load(csv_files.get('system_load.csv'))
    
    # Print statistics
    print_statistics(process_df, system_df)
    print("\nAnalysis complete!")
    print("Generated files:")
    if os.path.exists('process_stats.png'):
        print("  - process_stats.png")
    if os.path.exists('system_load.png'):
        print("  - system_load.png")

if __name__ == "__main__":
    main()