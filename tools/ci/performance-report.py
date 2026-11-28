"""Publish the benchmark output from the existing CTest execution, without rerunning tests."""

import argparse
import os
import pathlib
import shutil
import xml.etree.ElementTree as ET


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", required=True, type=pathlib.Path)
    parser.add_argument("--configuration", required=True)
    args = parser.parse_args()
    log = args.build / "Testing" / "Temporary" / "LastTest.log"
    junit = args.build / "performance-results.xml"
    report = [
        "# Performance validation",
        "",
        f"Configuration: {args.configuration}; runner: "
        f"{os.environ.get('RUNNER_OS', 'local')} {os.environ.get('RUNNER_ARCH', '')}; "
        f"image: {os.environ.get('ImageVersion', 'unknown')}.",
        "",
    ]
    if junit.is_file():
        root = ET.parse(junit).getroot()
        cases = root.findall(".//testcase")
        failures = sum(case.find("failure") is not None or case.find("error") is not None for case in cases)
        skipped = sum(case.find("skipped") is not None for case in cases)
        report += [
            f"Test cases: {len(cases)}; failures: {failures}; skipped: {skipped}.",
            "",
            "Case duration includes setup and is not the measured workload latency.",
            "",
        ]
    else:
        report += ["No JUnit output was produced.", ""]
    if log.is_file():
        shutil.copyfile(log, args.build / "performance.log")
        lines = [
            line.strip() for line in log.read_text(encoding="utf-8", errors="replace").splitlines()
            if line.strip().startswith(("[Chunk", "[Directional", "[Biome world performance]"))
        ]
        if lines:
            report += ["Measured workload output (milliseconds as reported by each test):", "", "```text", *lines, "```", ""]
        else:
            report += ["No measurement lines were found; inspect performance.log.", ""]
    else:
        report += ["No CTest log was produced.", ""]
    output = "\n".join(report) + "\n"
    (args.build / "performance-report.md").write_text(output, encoding="utf-8")
    summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary:
        with open(summary, "a", encoding="utf-8") as file:
            file.write(output)
    print(output)


if __name__ == "__main__":
    main()
