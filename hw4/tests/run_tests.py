#!/usr/bin/env python3
import subprocess
import sys
import os
import hashlib
import tempfile
import time

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
BUILD = os.path.join(SCRIPT_DIR, "..", "build")
APP = os.path.join(BUILD, "app")
SRV = os.path.join(BUILD, "server")


def sha1_md5(key: str) -> str:
    md5 = hashlib.md5(key.encode()).digest()
    return hashlib.sha1(md5).hexdigest()


def make_hashrate_file() -> str:
    with tempfile.NamedTemporaryFile(mode="w", suffix=".txt", delete=False) as f:
        f.write("500000\n")
        return f.name


def run_test(
    name: str,
    port: int,
    start: str,
    end: str,
    target_key: str | None,
    num_clients: int,
    timeout: int = 30,
) -> bool:
    print(f"\n=== TEST: {name} (port={port}, clients={num_clients}) ===")

    target_hash = sha1_md5(target_key) if target_key else "0" * 40

    hashrate_files = [make_hashrate_file() for _ in range(num_clients)]

    srv_proc = subprocess.Popen(
        [SRV, str(port), start, end, target_hash, str(num_clients)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )

    time.sleep(0.3)

    client_procs = [
        subprocess.Popen(
            [APP, "127.0.0.1", str(port), hf],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        for hf in hashrate_files
    ]

    try:
        sout, serr = srv_proc.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        srv_proc.kill()
        for p in client_procs:
            p.kill()
        for p in client_procs:
            p.communicate()
        for hf in hashrate_files:
            os.unlink(hf)
        print(f"FAIL: {name} — timeout after {timeout}s")
        return False

    for p in client_procs:
        try:
            p.communicate(timeout=5)
        except subprocess.TimeoutExpired:
            p.kill()
            p.communicate()

    for hf in hashrate_files:
        os.unlink(hf)

    output = sout.decode(errors="replace")
    exit_code = srv_proc.returncode

    if target_key is not None:
        if exit_code != 0:
            print(f"FAIL: {name} — server exit={exit_code}, expected 0 (key found)")
            print("  server stdout:", output.strip())
            print("  server stderr:", serr.decode(errors="replace").strip())
            return False
        if f"RESULT {target_key}" not in output:
            print(f"FAIL: {name} — expected 'RESULT {target_key}' in output")
            print("  server stdout:", output.strip())
            return False
        print(f"PASS: {name} — found key={target_key!r}")
        return True
    else:
        if exit_code != 1:
            print(f"FAIL: {name} — server exit={exit_code}, expected 1 (not found)")
            print("  server stdout:", output.strip())
            return False
        if "RESULT NOT_FOUND" not in output:
            print(f"FAIL: {name} — expected 'RESULT NOT_FOUND' in output")
            print("  server stdout:", output.strip())
            return False
        print(f"PASS: {name} — NOT_FOUND as expected")
        return True


TESTS = [
    ("single_client_found", 25000, "a", "z", "b", 1),
    ("single_client_not_found", 25001, "a", "z", None, 1),
    ("single_client_numeric", 25002, "100", "999", "500", 1),
    ("two_clients_key_in_first", 25003, "a0", "zz", "b5", 2),
    ("two_clients_key_in_second", 25004, "a0", "zz", "z0", 2),
    ("two_clients_not_found", 25005, "a0", "az", None, 2),
    ("three_clients_found", 25006, "aa", "zz", "mm", 3),
    ("three_clients_not_found", 25007, "aa", "az", None, 3),
    ("single_char_range_found", 25008, "0", "z", "9", 1),
    ("single_char_range_found_2", 25009, "0", "z", "A", 2),
]

passed = 0
failed = 0

for name, port, start, end, key, clients in TESTS:
    ok = run_test(name, port, start, end, key, clients)
    if ok:
        passed += 1
    else:
        failed += 1

print(f"\n=== RESULTS: {passed} passed, {failed} failed ===")
sys.exit(0 if failed == 0 else 1)
