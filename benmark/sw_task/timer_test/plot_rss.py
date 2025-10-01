import matplotlib.pyplot as plt
import sys

def read_rss_log(filename):
    times = []
    rss_values = []
    with open(filename, 'r') as f:
        next(f)  # Bỏ dòng header
        for line in f:
            parts = line.strip().split()
            if len(parts) == 2:
                t, rss = parts
                times.append(int(t))
                rss_values.append(int(rss) / 1024)  # Đổi KB sang MB
    return times, rss_values

if __name__ == "__main__":
    if len(sys.argv) < 3 or len(sys.argv) % 2 != 1:
        print("Usage: python3 plot_rss.py <log1> <label1> [<log2> <label2> ...]")
        print("Example: python3 plot_rss.py sw_task_rss_log.txt 'SW Task' tiger_rss_log.txt 'Tiger Looper'")
        sys.exit(1)

    plt.figure(figsize=(10, 6))
    colors = ['blue', 'red', 'green', 'orange', 'purple']
    for i in range(1, len(sys.argv), 2):
        log_file = sys.argv[i]
        label = sys.argv[i+1]
        times, rss_values = read_rss_log(log_file)
        plt.plot(times, rss_values, marker='o', linewidth=2, color=colors[(i//2)%len(colors)], label=label)

    plt.xlabel('Time (seconds)', fontsize=12)
    plt.ylabel('RSS Memory (MB)', fontsize=12)
    plt.title('RSS Memory Usage Over Time', fontsize=14, fontweight='bold')
    plt.grid(True, alpha=0.3)
    plt.legend()
    plt.tight_layout()
    plt.savefig('rss_compare.png', dpi=300)
    plt.show()