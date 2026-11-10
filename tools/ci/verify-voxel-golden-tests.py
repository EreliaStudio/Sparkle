#!/usr/bin/env python3
"""Fail CI when a voxel golden-image test is missing or silently skipped."""

import sys
import xml.etree.ElementTree as ET


EXPECTED = {
    "VoxelRendererGoldenTest": 5,
    "VoxelShapeGoldenTest": 16,
    "VoxelArrangementGoldenTest": 16,
    "VoxelCharacterGoldenTest": 5,
    "VoxelRoomGoldenTest": 12,
}


def main() -> int:
    cases = ET.parse(sys.argv[1]).getroot().findall(".//testcase")
    errors = []
    for name, expected_count in EXPECTED.items():
        selected = [case for case in cases if name in case.get("name", "")]
        if len(selected) != expected_count:
            errors.append(f"{name}: expected {expected_count} cases, got {len(selected)}")
        for case in selected:
            if case.get("status") != "run" or case.find("skipped") is not None:
                errors.append(f"Golden was not executed: {case.get('name')}")
            if case.find("failure") is not None or case.find("error") is not None:
                errors.append(f"Golden failed: {case.get('name')}")
    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1
    print(f"Verified {sum(EXPECTED.values())} executed voxel golden comparisons.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
