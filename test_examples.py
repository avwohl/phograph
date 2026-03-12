#!/usr/bin/env python3
"""Extract example JSONs from ExampleCatalog.swift and write them as files,
then compile and run the C++ test harness against each."""

import re
import os
import json
import subprocess
import sys

CATALOG_PATH = "Phograph/Model/ExampleCatalog.swift"
TEST_DIR = "test_output"
HARNESS = "test_output/test_harness"

def extract_examples():
    """Extract example names and JSON from ExampleCatalog.swift"""
    with open(CATALOG_PATH) as f:
        content = f.read()

    examples = []
    # Find all ExampleEntry blocks
    pattern = r'ExampleEntry\(\s*name:\s*"([^"]+)".*?projectJSON:\s*"""(.*?)"""'
    for m in re.finditer(pattern, content, re.DOTALL):
        name = m.group(1)
        json_str = m.group(2).strip()
        # Remove leading whitespace from each line (Swift multiline string indentation)
        lines = json_str.split('\n')
        # Find minimum indentation
        min_indent = float('inf')
        for line in lines:
            stripped = line.lstrip()
            if stripped:
                min_indent = min(min_indent, len(line) - len(stripped))
        if min_indent == float('inf'):
            min_indent = 0
        cleaned = '\n'.join(line[min_indent:] if len(line) >= min_indent else line for line in lines)
        examples.append((name, cleaned))

    return examples

def write_example_files(examples):
    os.makedirs(TEST_DIR, exist_ok=True)
    paths = []
    for name, json_str in examples:
        fname = re.sub(r'[^a-zA-Z0-9]', '_', name).lower() + ".json"
        path = os.path.join(TEST_DIR, fname)
        with open(path, 'w') as f:
            f.write(json_str)
        paths.append((name, path))
    return paths

def compile_harness():
    """Compile the C++ test harness."""
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

def run_examples(paths):
    """Run each example through the harness."""
    passed = 0
    failed = 0
    errors = []

    for name, path in paths:
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
    print("Extracting examples from ExampleCatalog.swift...")
    examples = extract_examples()
    print(f"Found {len(examples)} examples")

    paths = write_example_files(examples)

    if not compile_harness():
        sys.exit(1)

    print(f"\nRunning {len(paths)} examples:\n")
    errors = run_examples(paths)

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
