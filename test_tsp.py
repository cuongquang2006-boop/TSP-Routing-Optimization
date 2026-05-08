"""
test_tsp.py - Kiem tra tsp.exe bang pytest
Chay: py -m pytest test_tsp.py -v
"""

import subprocess
import pytest

TSP_BINARY = "tsp.exe"

# ──────────────────────────────────────────
# HELPER: chay tsp.exe voi input cho truoc
# ──────────────────────────────────────────
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
    """Tach ket qua tung thuat toan tu output"""
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

# ──────────────────────────────────────────
# INPUT MAU
# ──────────────────────────────────────────
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

# ──────────────────────────────────────────
# TEST 1: tsp.exe co chay duoc khong
# ──────────────────────────────────────────
def test_tsp_runs():
    """tsp.exe phai chay duoc va co output"""
    output = run_tsp(INPUT_5)
    assert len(output) > 0, "tsp.exe khong co output"

# ──────────────────────────────────────────
# TEST 2: co du 3 thuat toan trong output
# ──────────────────────────────────────────
def test_has_three_algorithms():
    """Output phai co Random, Greedy, Nearest Insertion"""
    output = run_tsp(INPUT_8)
    assert "Algorithm: Random" in output
    assert "Algorithm: Greedy" in output
    assert "Algorithm: Nearest Insertion" in output

# ──────────────────────────────────────────
# TEST 3: Branch & Bound xuat hien khi n <= 12
# ──────────────────────────────────────────
def test_branch_and_bound_appears_for_small_n():
    """Branch & Bound phai xuat hien khi n <= 12"""
    output = run_tsp(INPUT_8)
    assert "Algorithm: Branch & Bound" in output, \
        "Branch & Bound khong xuat hien voi n=8"

# ──────────────────────────────────────────
# TEST 4: Cost phai la so duong
# ──────────────────────────────────────────
def test_cost_is_positive():
    """Cost cua tat ca thuat toan phai > 0"""
    output = run_tsp(INPUT_8)
    results = parse_output(output)
    for alg, data in results.items():
        assert data["cost"] is not None, f"{alg}: khong co Cost"
        assert data["cost"] > 0, f"{alg}: Cost phai > 0"

# ──────────────────────────────────────────
# TEST 5: Greedy phai tot hon Random
# ──────────────────────────────────────────
def test_greedy_better_than_random():
    """Greedy phai co cost nho hon Random"""
    output = run_tsp(INPUT_10)
    results = parse_output(output)
    greedy_cost = results["Greedy"]["cost"]
    random_cost = results["Random"]["cost"]
    assert greedy_cost < random_cost, \
        f"Greedy ({greedy_cost}) phai tot hon Random ({random_cost})"

# ──────────────────────────────────────────
# TEST 6: Branch & Bound phai cho ket qua tot nhat
# ──────────────────────────────────────────
def test_branch_and_bound_is_optimal():
    """Branch & Bound phai co cost nho nhat trong tat ca thuat toan"""
    output = run_tsp(INPUT_8)
    results = parse_output(output)
    bb_cost = results["Branch & Bound"]["cost"]
    for alg, data in results.items():
        assert bb_cost <= data["cost"] + 0.01, \
            f"Branch & Bound ({bb_cost}) phai <= {alg} ({data['cost']})"

# ──────────────────────────────────────────
# TEST 7: Time phai >= 0
# ──────────────────────────────────────────
def test_time_is_non_negative():
    """Thoi gian chay phai >= 0"""
    output = run_tsp(INPUT_8)
    results = parse_output(output)
    for alg, data in results.items():
        assert data["time"] is not None, f"{alg}: khong co Time"
        assert data["time"] >= 0, f"{alg}: Time phai >= 0"

# ──────────────────────────────────────────
# TEST 8: Nearest Insertion phai tot hon Random
# ──────────────────────────────────────────
def test_nearest_insertion_better_than_random():
    """Nearest Insertion phai co cost nho hon Random"""
    output = run_tsp(INPUT_10)
    results = parse_output(output)
    ni_cost = results["Nearest Insertion"]["cost"]
    random_cost = results["Random"]["cost"]
    assert ni_cost < random_cost, \
        f"Nearest Insertion ({ni_cost}) phai tot hon Random ({random_cost})"