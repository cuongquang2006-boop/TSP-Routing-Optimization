import subprocess, sys, random, csv, statistics
from pathlib import Path

# ──────────────────────────────────────────
# CAU HINH
# ──────────────────────────────────────────
TSP_BINARY  = "tsp.exe"
OUTPUT_CSV  = "results.csv"
REPEAT_RUNS = 10

# Moi dataset co file input rieng
# (ten_dataset, so_diem, ten_file_input)
DATASETS = [
    # Nho: SA + NI + B&B + Held-Karp
    ("n8",   8,   "input8.txt",   "small"),
    ("n10",  10,  "input10.txt",  "small"),
    ("n12",  12,  "input12.txt",  "small"),
    # Trung: SA + NI + Held-Karp
    ("n18",  18,  "input18.txt",  "medium"),
    # Lon: SA + NI
    ("n50",  50,  "input50.txt",  "large"),
    ("n100", 100, "input100.txt", "large"),
    ("n150", 150, "input150.txt", "large"),
    ("n300", 300, "input300.txt", "large"),
    ("n500", 500, "input500.txt", "large"),  # dung file goc san co
]

ALGORITHMS_SMALL  = ["Simulated Annealing", "Nearest Insertion", "Branch & Bound", "Held-Karp DP"]
ALGORITHMS_MEDIUM = ["Simulated Annealing", "Nearest Insertion", "Held-Karp DP"]
ALGORITHMS_LARGE  = ["Simulated Annealing", "Nearest Insertion"]

# ──────────────────────────────────────────
# BUOC 1: TAO CAC FILE INPUT RIENG
# ──────────────────────────────────────────
def create_input_files():
    print("Tao file input cho tung dataset...")
    for ds_name, n, filename, group in DATASETS:
        # Bo qua neu la file goc input500.txt (da co san)
        if filename == "input500.txt":
            print(f"  {filename} - dung file goc san co")
            continue
        # Bo qua neu file da ton tai
        if Path(filename).exists():
            print(f"  {filename} - da ton tai, bo qua")
            continue
        # Sinh file moi
        seed = n * 31 + len(ds_name)
        rng = random.Random(seed)
        lines = [str(n)]
        for _ in range(n):
            lines.append(f"{rng.uniform(0,1000):.6f} {rng.uniform(0,1000):.6f}")
        with open(filename, "w") as f:
            f.write("\n".join(lines))
        print(f"  {filename} - da tao ({n} diem)")
    print()

# ──────────────────────────────────────────
# PARSE OUTPUT
# ──────────────────────────────────────────
def parse(stdout):
    results = {}
    cur = None
    for line in stdout.splitlines():
        line = line.strip()
        if line.startswith("Algorithm:"):
            cur = line.replace("Algorithm:", "").strip()
            results[cur] = {"cost": None, "time_us": None}
        elif line.startswith("Cost:") and cur:
            results[cur]["cost"] = float(line.replace("Cost:", "").strip())
        elif line.startswith("Avg Time (us):") and cur:
            results[cur]["time_us"] = float(line.replace("Avg Time (us):", "").strip())
        elif line.startswith("Time (us):") and cur:
            results[cur]["time_us"] = float(line.replace("Time (us):", "").strip())
    return results

# ──────────────────────────────────────────
# CHAY 1 LAN VOI FILE INPUT CU THE
# ──────────────────────────────────────────
def run_once(filename):
    try:
        proc = subprocess.run(
            [TSP_BINARY, filename],
            capture_output=True,
            text=True,
            timeout=600,
        )
        if proc.returncode != 0:
            print("    LOI:", proc.stderr[:200])
            return {}
        return parse(proc.stdout)
    except subprocess.TimeoutExpired:
        print("    TIMEOUT")
        return {}
    except Exception as e:
        print("    EXCEPTION:", e)
        return {}

# ──────────────────────────────────────────
# CHAY BENCHMARK 1 DATASET
# ──────────────────────────────────────────
def run_dataset(ds_name, n, filename, algorithms, rows):
    all_costs = {a: [] for a in algorithms}
    all_times = {a: [] for a in algorithms}

    for i in range(1, REPEAT_RUNS + 1):
        print(f"    Lan {i}/{REPEAT_RUNS}...", end=" ", flush=True)
        result = run_once(filename)
        if result:
            for alg in algorithms:
                if alg in result and result[alg]["cost"] is not None:
                    all_costs[alg].append(result[alg]["cost"])
                    all_times[alg].append(result[alg]["time_us"])
            print("OK")
        else:
            print("THAT BAI")

    print()
    print(f"    {'Thuat toan':<22} {'Best':>12} {'Avg':>12} {'Worst':>12} {'Time_avg(us)':>14}")
    print(f"    {'-'*22} {'-'*12} {'-'*12} {'-'*12} {'-'*14}")

    for alg in algorithms:
        costs = all_costs[alg]
        times = all_times[alg]
        if not costs:
            print(f"    {alg:<22} {'N/A':>12}")
            continue
        best  = min(costs)
        avg   = statistics.mean(costs)
        worst = max(costs)
        tavg  = statistics.mean(times)
        std   = statistics.stdev(costs) if len(costs) > 1 else 0.0
        print(f"    {alg:<22} {best:>12.2f} {avg:>12.2f} {worst:>12.2f} {tavg:>14.2f}")
        rows.append({
            "Dataset":     ds_name,
            "N_nodes":     n,
            "Algorithm":   alg,
            "Best":        round(best,  4),
            "Avg":         round(avg,   4),
            "Worst":       round(worst, 4),
            "Std":         round(std,   4),
            "Time_avg_us": round(tavg,  4),
            "Time_min_us": round(min(times), 4),
            "Time_max_us": round(max(times), 4),
            "Runs":        len(costs),
        })

# ──────────────────────────────────────────
# MAIN
# ──────────────────────────────────────────
def main():
    print("=" * 60)
    print("  TSP BENCHMARK")
    print(f"  So lan lap: {REPEAT_RUNS}")
    print("=" * 60)
    print()

    if not Path(TSP_BINARY).exists():
        print(f"KHONG TIM THAY {TSP_BINARY}")
        print("Hay chay: g++ -O2 -o tsp.exe tsp.cpp")
        sys.exit(1)

    # Buoc 1: Tao file input
    create_input_files()

    rows = []

    for ds_name, n, filename, group in DATASETS:
        if not Path(filename).exists():
            print(f"\n>>> SKIP {ds_name}: khong tim thay {filename}")
            continue

        # Chon bo thuat toan theo nhom
        if group == "small":
            algorithms = ALGORITHMS_SMALL
            label = "SA + NI + B&B + Held-Karp"
        elif group == "medium":
            algorithms = ALGORITHMS_MEDIUM
            label = "SA + NI + Held-Karp"
        else:
            algorithms = ALGORITHMS_LARGE
            label = "SA + NI"

        print(f"\n>>> Dataset: {ds_name} ({n} diem) [{label}] - file: {filename}")
        run_dataset(ds_name, n, filename, algorithms, rows)

    # Ghi CSV
    if rows:
        fields = ["Dataset", "N_nodes", "Algorithm", "Best", "Avg", "Worst", "Std",
                  "Time_avg_us", "Time_min_us", "Time_max_us", "Runs"]
        with open(OUTPUT_CSV, "w", newline="", encoding="utf-8") as f:
            w = csv.DictWriter(f, fieldnames=fields)
            w.writeheader()
            w.writerows(rows)
        print(f"\n>>> Xong! Da luu: {OUTPUT_CSV}  ({len(rows)} dong)")
    else:
        print("\n>>> Khong co ket qua.")

if __name__ == "__main__":
    main()