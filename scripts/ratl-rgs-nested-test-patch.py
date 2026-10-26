#!/usr/bin/env python3
"""Strictly test-only RATL P1-D recursive deletion patch for isolated checkout.

Never modifies pinned RATL source, vendored OpenATL, or installed SDK.
Retain LGPL/GPL license boundaries; publish original/patch test results separately.
"""
from pathlib import Path
import subprocess
import sys

PIN = "2e2efa182d7c6ee9037454d23780991fb8543084"

OLD = """        while (RegEnumKeyExW(parentKey, 0, szBuffer, &dwSize, NULL, NULL, NULL, &time) == ERROR_SUCCESS)
        {
            lRes = RegDeleteTreeX(childKey, szBuffer);
            if (lRes != ERROR_SUCCESS)
                return lRes;
            dwSize = sizeof(szBuffer) / sizeof(szBuffer[0]);
        }
        RegCloseKey(childKey);
        return RegDeleteKeyW(parentKey, subKeyName);"""
NEW = """        // Enumerate the opened child, not the parent. The old code
        // repeatedly selected the parent's first key and failed to
        // traverse nested descendants (or incorrectly returned early).
        while (RegEnumKeyExW(childKey, 0, szBuffer, &dwSize, NULL, NULL, NULL, &time) == ERROR_SUCCESS)
        {
            lRes = RegDeleteTreeX(childKey, szBuffer);
            if (lRes != ERROR_SUCCESS)
            {
                RegCloseKey(childKey); // no handle leak on recursive failure
                return lRes;
            }
            dwSize = sizeof(szBuffer) / sizeof(szBuffer[0]);
        }
        RegCloseKey(childKey);
        return RegDeleteKeyW(parentKey, subKeyName);"""


def main():
    if len(sys.argv) != 2:
        raise SystemExit("Usage: ratl-rgs-nested-test-patch.py TEMP_RATL_CHECKOUT")
    root=Path(sys.argv[1]).resolve(strict=True)
    sha=subprocess.check_output(
        ["git", "-C", str(root), "rev-parse", "HEAD"],text=True).strip()
    if sha != PIN:
        raise SystemExit(f"Refuse non-pinned RATL checkout: {sha}")
    header=root/"include"/"atl"/"statreg.h"
    contents=header.read_text(encoding="utf-8")
    if contents.count(OLD) != 1:
        raise SystemExit("Refuse unknown or already patched RATL RegDeleteTreeX")
    header.write_text(contents.replace(OLD,NEW),encoding="utf-8")
    print("Test-only RATL statreg nested delete parent->child traversal corrected")


if __name__=="__main__":
    main()
