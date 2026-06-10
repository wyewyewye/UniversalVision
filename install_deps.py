#!/usr/bin/env python3
"""
Auto-install script for third-party dependencies.
Reads config.py and installs all dependencies into third_party/ directory.
After installation, updates config.py paths to point to the new locations.

Usage:
    python install_deps.py              # Install all dependencies
    python install_deps.py --lib glfw   # Install specific library only
    python install_deps.py --skip-build # Skip cmake/b2 build steps (download only)
"""

import os
import sys
import urllib.request
import urllib.error
import zipfile
import tarfile
import shutil
import subprocess
import re
import argparse
import time
from pathlib import Path

# ============================================================
# Parse config.py to get library versions and settings
# ============================================================

def parse_config(config_path="config.py"):
    """Parse CMakeConfig class from config.py and extract library info."""
    config = {}
    config_path = Path(config_path)
    if not config_path.exists():
        print(f"[ERROR] config.py not found at {config_path.resolve()}")
        sys.exit(1)

    with open(config_path, "r", encoding="utf-8") as f:
        content = f.read()

    # Extract class CMakeConfig block
    match = re.search(r"class\s+CMakeConfig\s*:", content)
    if not match:
        print("[ERROR] CMakeConfig class not found in config.py")
        sys.exit(1)

    # Extract all assignments like KEY = VALUE
    pattern = re.compile(r"^\s+(\w+)\s*=\s*(.+?)$", re.MULTILINE)
    for m in pattern.finditer(content):
        key = m.group(1)
        value = m.group(2).strip()
        # Remove surrounding quotes for strings
        if value.startswith("r'") and value.endswith("'"):
            value = value[2:-1]
        elif value.startswith("'") and value.endswith("'"):
            value = value[1:-1]
        elif value.startswith('r"') and value.endswith('"'):
            value = value[2:-1]
        elif value.startswith('"') and value.endswith('"'):
            value = value[1:-1]
        config[key] = value

    return config


# ============================================================
# Utility functions
# ============================================================

THIRD_PARTY_DIR = Path(__file__).parent / "third_party"
DOWNLOAD_DIR = THIRD_PARTY_DIR / "_downloads"


def ensure_dir(path):
    path.mkdir(parents=True, exist_ok=True)


def download_file(url, dest_path, desc=""):
    """Download a file with progress indication."""
    if dest_path.exists():
        print(f"  [SKIP] {desc} already downloaded: {dest_path.name}")
        return dest_path

    print(f"  [DOWNLOAD] {desc} from {url}")
    ensure_dir(dest_path.parent)

    max_retries = 3
    for attempt in range(1, max_retries + 1):
        try:
            req = urllib.request.Request(url, headers={
                "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36"
            })
            with urllib.request.urlopen(req, timeout=120) as response:
                total = int(response.headers.get("Content-Length", 0))
                downloaded = 0
                chunk_size = 8192
                with open(dest_path, "wb") as f:
                    while True:
                        chunk = response.read(chunk_size)
                        if not chunk:
                            break
                        f.write(chunk)
                        downloaded += len(chunk)
                        if total > 0:
                            pct = downloaded * 100 // total
                            print(f"\r    Progress: {pct}% ({downloaded // 1024 // 1024}MB / {total // 1024 // 1024}MB)", end="")
                        else:
                            print(f"\r    Downloaded: {downloaded // 1024 // 1024}MB", end="")
                print()
            return dest_path
        except Exception as e:
            print(f"    Attempt {attempt}/{max_retries} failed: {e}")
            if attempt < max_retries:
                wait = 3 * attempt
                print(f"    Retrying in {wait}s...")
                time.sleep(wait)
            else:
                if dest_path.exists():
                    dest_path.unlink()
                raise
    return dest_path


def extract_zip(zip_path, extract_dir, desc=""):
    """Extract a zip file."""
    print(f"  [EXTRACT] {desc} -> {extract_dir}")
    ensure_dir(extract_dir)
    with zipfile.ZipFile(zip_path, "r") as zf:
        zf.extractall(extract_dir)
    print(f"    Done")


def extract_tar_gz(tar_path, extract_dir, desc=""):
    """Extract a tar.gz file."""
    print(f"  [EXTRACT] {desc} -> {extract_dir}")
    ensure_dir(extract_dir)
    with tarfile.open(tar_path, "r:gz") as tf:
        tf.extractall(extract_dir)
    print(f"    Done")


def get_single_top_dir(dir_path):
    """If the directory contains a single top-level directory, return its path."""
    entries = list(dir_path.iterdir())
    if len(entries) == 1 and entries[0].is_dir():
        return entries[0]
    return None


def run_command(cmd, cwd=None, desc=""):
    """Run a shell command and stream output."""
    print(f"  [RUN] {desc}")
    print(f"    Command: {' '.join(cmd)}")
    process = subprocess.Popen(
        cmd,
        cwd=cwd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        shell=True,
        text=True,
        bufsize=1,
    )
    for line in process.stdout:
        line = line.rstrip()
        if line:
            print(f"    | {line}")
    process.wait()
    if process.returncode != 0:
        raise RuntimeError(f"Command failed with exit code {process.returncode}: {' '.join(cmd)}")
    return process.returncode


def update_config_path(config_path, key, new_value):
    """Update a path value in config.py for a given key."""
    config_path = Path(config_path)
    with open(config_path, "r", encoding="utf-8") as f:
        content = f.read()

    # Escape backslashes for regex
    old_value = None
    pattern = re.compile(rf"^(\s+{re.escape(key)}\s*=\s*).+$", re.MULTILINE)
    m = pattern.search(content)
    if m:
        old_line = m.group(0)
        old_value = old_line.split("=", 1)[1].strip()

    # Normalize path separators for comparison
    def norm(p):
        return os.path.normpath(p).lower()

    if old_value is not None:
        old_clean = old_value.strip().strip("'\"").strip("r'").strip('r"')
        if norm(old_clean) == norm(new_value):
            print(f"  [CONFIG] {key} already set to {new_value}, skipping")
            return False

    # Replace the value
    escaped_new = new_value.replace("\\", "\\\\")
    new_line = f'\\1r\'{escaped_new}\''
    content, count = pattern.subn(new_line, content)
    if count == 0:
        print(f"  [WARN] Could not find {key} in config.py")
        return False

    with open(config_path, "w", encoding="utf-8") as f:
        f.write(content)
    print(f"  [CONFIG] Updated {key} -> {new_value}")
    return True


# ============================================================
# Library installers
# ============================================================

def install_glfw(config, config_path):
    """Install GLFW pre-built binaries."""
    version = config.get("GLFW_VERSION", "3.4")
    lib_name = f"glfw-{version}.bin.WIN64"
    install_dir = THIRD_PARTY_DIR / lib_name
    include_dir = install_dir / "include"
    lib_dir = install_dir / "lib-vc2022"

    print(f"\n{'='*60}")
    print(f"GLFW {version}")
    print(f"{'='*60}")

    if install_dir.exists() and include_dir.exists() and lib_dir.exists():
        print(f"  [SKIP] Already installed at {install_dir}")
    else:
        zip_name = f"glfw-{version}.bin.WIN64.zip"
        zip_url = f"https://github.com/glfw/glfw/releases/download/{version}/{zip_name}"
        zip_path = DOWNLOAD_DIR / zip_name

        download_file(zip_url, zip_path, f"GLFW {version}")
        extract_zip(zip_path, THIRD_PARTY_DIR, f"GLFW {version}")

        # Verify extraction
        extracted = THIRD_PARTY_DIR / lib_name
        if not extracted.exists():
            # Maybe extracted into a subdirectory
            subdir = get_single_top_dir(THIRD_PARTY_DIR / zip_name.replace(".zip", ""))
            if subdir:
                shutil.move(str(subdir), str(extracted))
            else:
                print(f"  [ERROR] GLFW extraction failed, expected {extracted}")
                return False

        if not include_dir.exists():
            print(f"  [WARN] GLFW include dir not found at {include_dir}")
        if not lib_dir.exists():
            print(f"  [WARN] GLFW lib dir not found at {lib_dir}")

    # Update config.py paths
    update_config_path(config_path, "GLFW_CMAKE_PACKAGE_PATH", "")
    update_config_path(config_path, "GLFW_INCLUDE_PATH", str(include_dir.resolve()))
    update_config_path(config_path, "GLFW_LIB_PATH", str(lib_dir.resolve()))
    return True


def install_boost(config, config_path):
    """Install Boost from source (build with b2)."""
    version = config.get("BOOST_VERSION", "1.88.0")
    # Convert version to underscore format: 1.88.0 -> 1_88_0
    version_us = version.replace(".", "_")
    install_dir = THIRD_PARTY_DIR / f"boost_vs2022"
    include_dir = install_dir / "include" / f"boost-{version_us}"
    lib_dir = install_dir / "lib"

    print(f"\n{'='*60}")
    print(f"BOOST {version}")
    print(f"{'='*60}")

    if install_dir.exists() and include_dir.exists():
        print(f"  [SKIP] Already installed at {install_dir}")
    else:
        src_dir = THIRD_PARTY_DIR / f"boost_{version_us}"
        src_zip = DOWNLOAD_DIR / f"boost_{version_us}.zip"

        # Download Boost source
        zip_url = f"https://archives.boost.io/release/{version}/source/boost_{version_us}.zip"
        download_file(zip_url, src_zip, f"Boost {version}")

        # Extract
        if not src_dir.exists():
            extract_zip(src_zip, THIRD_PARTY_DIR, f"Boost {version}")
            extracted = THIRD_PARTY_DIR / f"boost_{version_us}"
            if extracted.exists() and extracted != src_dir:
                pass  # Already in the right place
        else:
            print(f"  [SKIP] Boost source already extracted at {src_dir}")

        # Build with b2
        bootstrap_exe = src_dir / "bootstrap.bat"
        b2_exe = src_dir / "b2.exe"

        if not b2_exe.exists():
            if not bootstrap_exe.exists():
                print(f"  [ERROR] bootstrap.bat not found at {bootstrap_exe}")
                return False
            run_command(
                f'cmd /c "{bootstrap_exe}"',
                cwd=str(src_dir),
                desc="Boost bootstrap.bat",
            )

        if not b2_exe.exists():
            print(f"  [ERROR] b2.exe not found after bootstrap")
            return False

        # Build and install
        # Using link=static as per config.py comment
        run_command(
            f'cmd /c "{b2_exe}" toolset=msvc-14.3 install --prefix="{install_dir}" link=static',
            cwd=str(src_dir),
            desc=f"Boost b2 install -> {install_dir}",
        )

        if not include_dir.exists():
            print(f"  [WARN] Boost include dir not found at {include_dir}")
        if not lib_dir.exists():
            print(f"  [WARN] Boost lib dir not found at {lib_dir}")

    # Update config.py paths
    update_config_path(config_path, "BOOST_CMAKE_PACKAGE_PATH", str(install_dir.resolve()))
    update_config_path(config_path, "BOOST_INCLUDE_PATH", str(include_dir.resolve()))
    update_config_path(config_path, "BOOST_LIB_PATH", str(lib_dir.resolve()))
    return True


def install_gtest(config, config_path):
    """Install Google Test from source (build with cmake)."""
    version = config.get("GTEST_VERSION", "1.17.0")
    install_dir = THIRD_PARTY_DIR / f"gtest_vs2022"
    include_dir = install_dir / "include"
    lib_dir = install_dir / "lib"

    print(f"\n{'='*60}")
    print(f"GTEST {version}")
    print(f"{'='*60}")

    if install_dir.exists() and include_dir.exists():
        print(f"  [SKIP] Already installed at {install_dir}")
    else:
        src_dir = THIRD_PARTY_DIR / f"googletest-{version}"
        src_zip = DOWNLOAD_DIR / f"googletest-{version}.zip"

        zip_url = f"https://github.com/google/googletest/archive/refs/tags/v{version}.zip"
        download_file(zip_url, src_zip, f"Google Test {version}")

        if not src_dir.exists():
            extract_zip(src_zip, THIRD_PARTY_DIR, f"Google Test {version}")
            # GitHub archives have a different directory name
            extracted = THIRD_PARTY_DIR / f"googletest-{version}"
            if extracted.exists() and extracted != src_dir:
                pass  # Already in the right place

        if not src_dir.exists():
            print(f"  [ERROR] GTest source not found at {src_dir}")
            return False

        # CMake build
        build_dir = src_dir / "build"
        ensure_dir(build_dir)

        run_command(
            f'cmake .. -DCMAKE_INSTALL_PREFIX="{install_dir}"',
            cwd=str(build_dir),
            desc="GTest cmake configure",
        )
        run_command(
            f'cmake --build . --config Debug --target install',
            cwd=str(build_dir),
            desc="GTest cmake build & install (Debug)",
        )
        run_command(
            f'cmake --build . --config Release --target install',
            cwd=str(build_dir),
            desc="GTest cmake build & install (Release)",
        )

        if not include_dir.exists():
            print(f"  [WARN] GTest include dir not found at {include_dir}")

    update_config_path(config_path, "GTEST_CMAKE_PACKAGE_PATH", str(install_dir.resolve()))
    update_config_path(config_path, "GTEST_INCLUDE_PATH", str(include_dir.resolve()))
    update_config_path(config_path, "GTEST_LIB_PATH", str(lib_dir.resolve()))
    return True


def install_poco(config, config_path):
    """Install POCO from source (build with cmake)."""
    version = config.get("POCO_VERSION", "1.14.2")
    install_dir = THIRD_PARTY_DIR / f"poco_vs2022"
    include_dir = install_dir / "include"
    lib_dir = install_dir / "lib"

    print(f"\n{'='*60}")
    print(f"POCO {version}")
    print(f"{'='*60}")

    if install_dir.exists() and include_dir.exists():
        print(f"  [SKIP] Already installed at {install_dir}")
    else:
        # GitHub tag format: poco-{version}-release
        tag = f"poco-{version}-release"
        src_dir = THIRD_PARTY_DIR / f"poco-{tag}"
        src_zip = DOWNLOAD_DIR / f"{tag}.zip"

        zip_url = f"https://github.com/pocoproject/poco/archive/refs/tags/{tag}.zip"
        download_file(zip_url, src_zip, f"POCO {version}")

        if not src_dir.exists():
            extract_zip(src_zip, THIRD_PARTY_DIR, f"POCO {version}")
            # Check extracted directory name
            extracted = THIRD_PARTY_DIR / f"poco-{tag}"
            if extracted.exists() and extracted != src_dir:
                pass

        if not src_dir.exists():
            print(f"  [ERROR] POCO source not found at {src_dir}")
            return False

        # CMake build
        build_dir = src_dir / "cmake-build"
        ensure_dir(build_dir)

        run_command(
            f'cmake .. -DCMAKE_INSTALL_PREFIX="{install_dir}"',
            cwd=str(build_dir),
            desc="POCO cmake configure",
        )
        run_command(
            f'cmake --build . --config Debug --target install',
            cwd=str(build_dir),
            desc="POCO cmake build & install (Debug)",
        )
        run_command(
            f'cmake --build . --config Release --target install',
            cwd=str(build_dir),
            desc="POCO cmake build & install (Release)",
        )

        if not include_dir.exists():
            print(f"  [WARN] POCO include dir not found at {include_dir}")

    update_config_path(config_path, "POCO_CMAKE_PACKAGE_PATH", str(install_dir.resolve()))
    update_config_path(config_path, "POCO_INCLUDE_PATH", str(include_dir.resolve()))
    update_config_path(config_path, "POCO_LIB_PATH", str(lib_dir.resolve()))
    return True


def install_ffmpeg(config, config_path):
    """Install FFMPEG pre-built binaries."""
    version = config.get("FFMPEG_VERSION", "4.4.6")
    install_dir = THIRD_PARTY_DIR / f"ffmpeg_vs2022"
    include_dir = install_dir / "include"
    lib_dir = install_dir / "lib"

    print(f"\n{'='*60}")
    print(f"FFMPEG {version}")
    print(f"{'='*60}")

    if install_dir.exists() and include_dir.exists():
        print(f"  [SKIP] Already installed at {install_dir}")
    else:
        # Use BtbN FFmpeg builds (provides dev headers and libs)
        # For FFmpeg 4.4, use the autobuild from around that time
        zip_name = f"ffmpeg-n{version}-latest-win64-gpl-{version}.zip"
        zip_url = f"https://github.com/BtbN/FFmpeg-Builds/releases/download/autobuild-2021-12-01-12-22/{zip_name}"
        zip_path = DOWNLOAD_DIR / zip_name

        try:
            download_file(zip_url, zip_path, f"FFMPEG {version}")
        except Exception as e:
            print(f"  [WARN] Primary URL failed: {e}")
            # Fallback: try a more recent build
            print(f"  Trying alternative URL...")
            zip_name2 = "ffmpeg-master-latest-win64-gpl.zip"
            zip_url2 = f"https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/{zip_name2}"
            zip_path2 = DOWNLOAD_DIR / zip_name2
            download_file(zip_url2, zip_path2, f"FFMPEG (latest)")
            zip_path = zip_path2

        extract_zip(zip_path, THIRD_PARTY_DIR, f"FFMPEG")

        # Find the extracted directory
        extracted_items = list(THIRD_PARTY_DIR.glob("ffmpeg-*"))
        if not extracted_items:
            extracted_items = list(THIRD_PARTY_DIR.glob("ffmpeg*"))
        if extracted_items:
            extracted_dir = extracted_items[0]
            # Create our standard directory structure
            ensure_dir(install_dir)
            # Copy include and lib if they exist
            src_include = extracted_dir / "include"
            src_lib = extracted_dir / "lib"
            if src_include.exists():
                if include_dir.exists():
                    shutil.rmtree(include_dir)
                shutil.copytree(src_include, include_dir)
            if src_lib.exists():
                if lib_dir.exists():
                    shutil.rmtree(lib_dir)
                shutil.copytree(src_lib, lib_dir)
            print(f"  Installed FFMPEG to {install_dir}")
        else:
            print(f"  [WARN] Could not find extracted FFMPEG directory")

        if not include_dir.exists():
            print(f"  [WARN] FFMPEG include dir not found at {include_dir}")
        if not lib_dir.exists():
            print(f"  [WARN] FFMPEG lib dir not found at {lib_dir}")

    update_config_path(config_path, "FFMPEG_CMAKE_PACKAGE_PATH", str(install_dir.resolve()))
    update_config_path(config_path, "FFMPEG_INCLUDE_PATH", str(include_dir.resolve()))
    update_config_path(config_path, "FFMPEG_LIB_PATH", str(lib_dir.resolve()))
    return True


# ============================================================
# Main
# ============================================================

def main():
    parser = argparse.ArgumentParser(description="Install third-party dependencies for UniversalVision")
    parser.add_argument("--lib", choices=["glfw", "boost", "gtest", "poco", "ffmpeg", "all"],
                        default="all", help="Specific library to install (default: all)")
    parser.add_argument("--skip-build", action="store_true",
                        help="Skip build steps (cmake/b2), download only")
    parser.add_argument("--config", default="config.py",
                        help="Path to config.py (default: config.py)")
    args = parser.parse_args()

    config_path = Path(args.config).resolve()
    if not config_path.exists():
        print(f"[ERROR] {config_path} not found")
        sys.exit(1)

    print(f"Reading configuration from {config_path}")
    config = parse_config(str(config_path))

    # Create directories
    ensure_dir(THIRD_PARTY_DIR)
    ensure_dir(DOWNLOAD_DIR)

    print(f"Third-party directory: {THIRD_PARTY_DIR.resolve()}")
    print(f"Download cache: {DOWNLOAD_DIR.resolve()}")

    # Define install order
    libs = []
    if args.lib == "all":
        libs = [
            ("glfw", install_glfw),
            ("gtest", install_gtest),
            ("poco", install_poco),
            ("ffmpeg", install_ffmpeg),
            ("boost", install_boost),  # Boost last (longest build time)
        ]
    else:
        lib_map = {
            "glfw": ("glfw", install_glfw),
            "boost": ("boost", install_boost),
            "gtest": ("gtest", install_gtest),
            "poco": ("poco", install_poco),
            "ffmpeg": ("ffmpeg", install_ffmpeg),
        }
        libs = [lib_map[args.lib]]

    success = True
    for name, installer in libs:
        try:
            if not installer(config, str(config_path)):
                print(f"[FAIL] {name} installation failed")
                success = False
        except Exception as e:
            print(f"[FAIL] {name} installation failed: {e}")
            success = False

    print(f"\n{'='*60}")
    if success:
        print("All dependencies installed successfully!")
    else:
        print("Some dependencies failed to install. Check the logs above.")
    print(f"{'='*60}")

    return 0 if success else 1


if __name__ == "__main__":
    sys.exit(main())
