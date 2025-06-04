import re
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
from matplotlib.ticker import ScalarFormatter, MultipleLocator


def parse_attack_times(file_path):
    """
    Parses a log file to extract attack cycles and times per image.
    Returns dictionaries with average and standard deviation for both.
    """
    attack_cycles = {}
    attack_times = {}
    current_image = None

    with open(file_path, 'r') as file:
        lines = file.readlines()

    for i in range(len(lines) - 1):
        match_image = re.match(r"Iteration \d+ for image (.+)", lines[i].strip())
        match_time = re.match(
            r"Total attack took: (\d+) cycles, ([\d.]+)s seconds", lines[i + 1].strip()
        )

        if match_image:
            current_image = match_image.group(1)
            if current_image not in attack_cycles:
                attack_cycles[current_image] = []
                attack_times[current_image] = []

        if match_time and current_image:
            cycles = int(match_time.group(1))
            seconds = float(match_time.group(2))
            attack_cycles[current_image].append(cycles)
            attack_times[current_image].append(seconds)

    avg_cycles = {img: np.mean(cycles) for img, cycles in attack_cycles.items() if cycles}
    std_cycles = {img: np.std(cycles) for img, cycles in attack_cycles.items() if cycles}
    avg_times = {img: np.mean(times) for img, times in attack_times.items() if times}
    std_times = {img: np.std(times) for img, times in attack_times.items() if times}

    return avg_cycles, std_cycles, avg_times, std_times

def format_hms(seconds):
    h = int(seconds // 3600)
    m = int((seconds % 3600) // 60)
    s = int(seconds % 60)
    if h > 0:
        return f"{h}h{m}m{s}s"
    else:
        return f"{m}m{s}s"






# File paths
file1 = "./macrobenchmark-gray/libjpeg_macrobenchmark_kernelfaulthandler_mprotect.txt"
file2 = "./macrobenchmark-gray/libjpeg_macrobenchmark_kernelfaulthandler_PTE.txt"
file3 = "./macrobenchmark-gray/libjpeg_macrobenchmark_IDT_PTE.txt"

# Parse data
avg_cycles_1, std_cycles_1, avg_times_1, std_times_1 = parse_attack_times(file1)
avg_cycles_2, std_cycles_2, avg_times_2, std_times_2 = parse_attack_times(file2)
avg_cycles_3, std_cycles_3, avg_times_3, std_times_3 = parse_attack_times(file3)


# Merge results
all_images = set(avg_cycles_1.keys()) | set(avg_cycles_2.keys()) | set(avg_cycles_3.keys())
data = {
    "Image": list(all_images),
    "Implementation 1": [avg_cycles_1.get(img, 0) for img in all_images],
    "Implementation 1 Time": [avg_times_1.get(img, 0) for img in all_images],
    "Implementation 2": [avg_cycles_2.get(img, 0) for img in all_images],
    "Implementation 2 Time": [avg_times_2.get(img, 0) for img in all_images],
    "Implementation 3": [avg_cycles_3.get(img, 0) for img in all_images],
    "Implementation 3 Time": [avg_times_3.get(img, 0) for img in all_images],
    "StdDev 1": [std_cycles_1.get(img, 0) for img in all_images],
    "StdDev 2": [std_cycles_2.get(img, 0) for img in all_images],
    "StdDev 3": [std_cycles_3.get(img, 0) for img in all_images],
}


df = pd.DataFrame(data)

# Custom order for images
custom_order = ["SIGSAC_logo_308-gray.jpg", "birds-gray.jpg", "muskox_gray.jpg", "Wapiti_from_Wagon_Trails-gray.jpg"]
df['CustomOrder'] = df['Image'].apply(lambda x: custom_order.index(x) if x in custom_order else float('inf'))
df = df.sort_values('CustomOrder').drop(columns='CustomOrder')
df = df[~df["Image"].str.contains("logo-gray", case=False)]


# Plotting
fig, ax = plt.subplots(figsize=(14, 9))

bar_width = 0.28
x = np.arange(len(df["Image"]))

bars1 = ax.bar(x, df["Implementation 1"], bar_width, 
               yerr=df["StdDev 1"], capsize=5, label="mprotect", color="#2196F3")
bars2 = ax.bar([i + bar_width for i in x], df["Implementation 2"], bar_width, 
               yerr=df["StdDev 2"], capsize=5, label="PTE", color="#4CAF50")
bars3 = ax.bar([i + 2*bar_width for i in x], df["Implementation 3"], bar_width,
               yerr=df["StdDev 3"], capsize=5, label="PTE + IDT", color="#FF9800")


def annotate_bars_with_time(bars, avg_time_seconds_col):
    for i, bar in enumerate(bars):
        height = bar.get_height()  # cycles
        avg_time_seconds = df[avg_time_seconds_col].iloc[i]
        time_str = format_hms(avg_time_seconds)  # format seconds to HH:MM:SS
        ax.text(bar.get_x() + bar.get_width() / 2, height * 1.01,  # slightly above bar
                time_str,
                ha='center', va='bottom', fontsize=18)




annotate_bars_with_time(bars1, "Implementation 1 Time")
annotate_bars_with_time(bars2, "Implementation 2 Time")
annotate_bars_with_time(bars3, "Implementation 3 Time")

ax.set_xticks(x + 0.28)
ax.set_xticklabels(["SIGSAC", "Birds", "Muskox", "Wapiti"], rotation=0, fontsize=22)
plt.tick_params(axis='y', labelsize=22)

formatter = ScalarFormatter(useMathText=True)
formatter.set_powerlimits((11, 11))  # Forces scientific notation like ×10^5
ax.yaxis.set_major_locator(MultipleLocator(5e11))
ax.yaxis.set_major_formatter(formatter)
ax.yaxis.get_offset_text().set_fontsize(26) 

    
    
    
ax.set_ylabel("Average Attack Time (CPU Cycles)", fontsize=22)
# ax.set_title("Macrobenchmark Color, N=10", fontsize=16)
ax.grid(axis='y')
ax.legend(fontsize=22)

plt.tight_layout()
plt.savefig("macrobenchmark_gray.png", dpi=500)
plt.show()
