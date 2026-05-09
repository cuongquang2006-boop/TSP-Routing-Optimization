import subprocess
import pytest

TSP_BINARY = "tsp.exe"

def run_tsp(input_text):
    proc = subprocess.run(
        [TSP_BINARY],
        input=input_text,
        capture_output=True,
        text=True,
        timeout=60,
    )
    return proc.stdout

def parse_output(stdout):
    results = {}
    cur = None
    for line in stdout.splitlines():
        line = line.strip()
        if line.startswith("Algorithm:"):
            cur = line.replace("Algorithm:", "").strip()
            results[cur] = {"cost": None, "time": None, "path": []}
        elif line.startswith("Cost:") and cur:
            results[cur]["cost"] = float(line.replace("Cost:", "").strip())
        elif line.startswith("Avg Time (us):") and cur:
            results[cur]["time"] = float(line.replace("Avg Time (us):", "").strip())
        elif line.startswith("Time (us):") and cur:
            results[cur]["time"] = float(line.replace("Time (us):", "").strip())
        elif line.startswith("Path:") and cur:
            pass
        elif cur and results[cur]["path"] == [] and line and all(
            c.isdigit() or c == " " for c in line
        ):
            results[cur]["path"] = list(map(int, line.split()))
    return results

INPUT_5 = """5
0 0
100 0
100 100
0 100
50 50"""

INPUT_8 = """8
100 200
300 400
500 100
200 300
400 500
150 250
350 450
50 350"""

INPUT_10 = """10
639 25
275 223
736 676
892 86
501 425
687 253
893 769
38 322
665 29
272 474"""

def test_tsp_runs():
    output = run_tsp(INPUT_5)
    assert len(output) > 0, "tsp.exe khong co output"

def test_has_three_algorithms():
    output = run_tsp(INPUT_8)
    assert "Algorithm: Random" in output
    assert "Algorithm: Greedy" in output
    assert "Algorithm: Nearest Insertion" in output

def test_branch_and_bound_appears_for_small_n():
    output = run_tsp(INPUT_8)
    assert "Algorithm: Branch & Bound" in output, \
        "Branch & Bound khong xuat hien voi n=8"

def test_cost_is_positive():
    output = run_tsp(INPUT_8)
    results = parse_output(output)
    for alg, data in results.items():
        assert data["cost"] is not None, f"{alg}: khong co Cost"
        assert data["cost"] > 0, f"{alg}: Cost phai > 0"

def test_greedy_better_than_random():
    output = run_tsp(INPUT_10)
    results = parse_output(output)
    greedy_cost = results["Greedy"]["cost"]
    random_cost = results["Random"]["cost"]
    assert greedy_cost < random_cost, \
        f"Greedy ({greedy_cost}) phai tot hon Random ({random_cost})"

def test_branch_and_bound_is_optimal():
    output = run_tsp(INPUT_8)
    results = parse_output(output)
    bb_cost = results["Branch & Bound"]["cost"]
    for alg, data in results.items():
        assert bb_cost <= data["cost"] + 0.01, \
            f"Branch & Bound ({bb_cost}) phai <= {alg} ({data['cost']})"

def test_time_is_non_negative():
    output = run_tsp(INPUT_8)
    results = parse_output(output)
    for alg, data in results.items():
        assert data["time"] is not None, f"{alg}: khong co Time"
        assert data["time"] >= 0, f"{alg}: Time phai >= 0"

def test_nearest_insertion_better_than_random():
    output = run_tsp(INPUT_10)
    results = parse_output(output)
    ni_cost = results["Nearest Insertion"]["cost"]
    random_cost = results["Random"]["cost"]
    assert ni_cost < random_cost, \
        f"Nearest Insertion ({ni_cost}) phai tot hon Random ({random_cost})"
