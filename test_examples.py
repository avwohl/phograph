#!/usr/bin/env python3
"""Load example JSONs from the examples/ directory and run the C++ test harness against each."""

import os
import json
import subprocess
import sys

EXAMPLES_DIR = "examples"
TEST_DIR = "test_output"
HARNESS = "test_output/test_harness"

def load_examples():
    """Load example names and file paths from catalog.json."""
    catalog_path = os.path.join(EXAMPLES_DIR, "catalog.json")
    with open(catalog_path) as f:
        catalog = json.load(f)

    examples = []
    for cat in catalog["categories"]:
        for entry in cat["examples"]:
            path = os.path.join(EXAMPLES_DIR, entry["file"])
            examples.append((entry["name"], path))
    return examples

def compile_harness():
    """Compile the C++ test harness."""
    os.makedirs(TEST_DIR, exist_ok=True)
    src_dir = "phograph_core/src"
    # Collect all .cc files except the Apple platform file and bridge
    cc_files = []
    for f in sorted(os.listdir(src_dir)):
        if f.endswith('.cc') and f != 'pho_bridge.cc':
            cc_files.append(os.path.join(src_dir, f))

    # Also compile plugins (they're .cpp)
    plugin_dir = os.path.join(src_dir, "plugins")
    if os.path.isdir(plugin_dir):
        for f in sorted(os.listdir(plugin_dir)):
            if f.endswith('.cpp'):
                cc_files.append(os.path.join(plugin_dir, f))

    cmd = [
        "c++", "-std=c++17", "-O1",
        "-I", src_dir,
        "-I", os.path.join(src_dir, "plugins"),
        "-o", HARNESS,
        "test_harness.cc",
    ] + cc_files + [
        "-framework", "CoreMIDI",
        "-framework", "AudioToolbox",
        "-framework", "CoreFoundation",
        "-framework", "CoreGraphics",
        "-framework", "ImageIO",
        "-framework", "Security",
    ]

    print("Compiling test harness...")
    print(" ".join(cmd[:8]) + " ...")
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print("COMPILE FAILED:")
        print(result.stderr)
        return False
    print("Compiled OK")
    return True

def run_examples(examples):
    """Run each example through the harness."""
    passed = 0
    failed = 0
    errors = []

    for name, path in examples:
        result = subprocess.run(
            [HARNESS, path],
            capture_output=True, text=True,
            timeout=10
        )
        status = result.stdout.strip().split('\n')[-1] if result.stdout.strip() else "NO OUTPUT"

        if result.returncode == 0 and "PASS" in status:
            print(f"  PASS  {name}")
            passed += 1
        else:
            print(f"  FAIL  {name}")
            if result.stdout.strip():
                for line in result.stdout.strip().split('\n'):
                    print(f"        {line}")
            if result.stderr.strip():
                for line in result.stderr.strip().split('\n')[:5]:
                    print(f"        stderr: {line}")
            failed += 1
            errors.append((name, result.stdout, result.stderr))

    print(f"\n{passed} passed, {failed} failed out of {passed + failed}")
    return errors

def main():
    print("Loading examples from examples/catalog.json...")
    examples = load_examples()
    print(f"Found {len(examples)} examples")

    if not compile_harness():
        sys.exit(1)

    print(f"\nRunning {len(examples)} examples:\n")
    errors = run_examples(examples)

    if errors:
        print("\nFailed examples detail:")
        for name, stdout, stderr in errors:
            print(f"\n--- {name} ---")
            print(stdout[:500])
            if stderr:
                print(f"stderr: {stderr[:300]}")
        sys.exit(1)

if __name__ == "__main__":
    main()
