#!/usr/bin/env python3

# Copyright (C) 2026 Oscar Pernia <oscarperniamoreno@gmail.com>
#
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, see
# <https://www.gnu.org/licenses/>.

import argparse
import os
import subprocess
import sys

from pathlib import Path
from subprocess import PIPE


def eprint(*args, file=None, **kwargs):
    print(*args, file=sys.stderr, **kwargs)


def run_win32(exe_abspath, mock_abslibs):
    # Mock libs are unused, IAT hooking is performed on runtime, still, users must place the DLLs
    # files where the linker can find it on runtime.
    _ = mock_abslibs
    return subprocess.run(
        [exe_abspath],
        stdin=PIPE,
        stdout=PIPE,
        stderr=PIPE,
    ).returncode


def run_macos(exe_abspath, mock_abslibs):
    return subprocess.run(
        [exe_abspath],
        env={"DYLD_INSERT_LIBRARIES": os.pathsep.join(mock_abslibs)},
        stdin=PIPE,
        stdout=PIPE,
        stderr=PIPE,
    ).returncode


def run_linux(exe_abspath: str, mock_abslibs: list[str]):
    return subprocess.run(
        [exe_abspath],
        env={"LD_PRELOAD": os.pathsep.join(mock_abslibs)},
        stdin=PIPE,
        stdout=PIPE,
        stderr=PIPE,
    ).returncode


def main():
    parser = argparse.ArgumentParser(
        description="Runs test programs with mocked implementations",
    )

    parser.add_argument("executable", help="test executable to be run")
    parser.add_argument(
        "--mock-library",
        "-m",
        action="append",
        nargs="+",
        required=True,
        help="mock shared object containing mocked implementations, you can add more than one and it will appended",
    )

    parsed = parser.parse_args()

    exe_abspath = str(Path(parsed.executable).resolve().absolute())
    mock_abslibs = [str(Path(x).resolve().absolute()) for x in parsed.mock_library]

    if sys.platform == "linux":
        code = run_linux(exe_abspath, mock_abslibs)
    elif sys.platform == "darwin":
        code = run_macos(exe_abspath, mock_abslibs)
    elif sys.platform == "win32":
        code = run_win32(exe_abspath, mock_abslibs)
    else:
        eprint(
            f"'${sys.platform}' platform unsupported, file an issue with the name of your platform at https://gitlab.gnome.org/otaxhu/glib-mock/-/issues",
        )
        code = 1

    sys.exit(code)


if __name__ == "__main__":
    main()
