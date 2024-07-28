import os
import subprocess
import argparse
import platform
import shutil

BUILD_DIR_NAME = "build"
PROJECT_NAME = "voxels"
EXECUTABLE_NAME = "voxels"
COMPILE_DB_NAME = "compile_commands.json"
BUILD_MODES = ["Debug", "Release", "RelWithDebInfo"]


def cmake_generate(mode):
    print(f"Running CMake in {mode} mode")
    cmake_command = [
        "cmake",
        "-G",
        "Ninja",
        '-DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld"',
        '-DCMAKE_SHARED_LINKER_FLAGS="-fuse-ld=lld"',
        f"-DCMAKE_BUILD_TYPE={mode}",
        "../..",
    ]
    cmake_process = subprocess.Popen(cmake_command)
    cmake_process.wait()

    if cmake_process.returncode != 0:
        print("CMake configuration failed. Exiting...")
        exit(1)


def find_executable_path(directory, executable_name):
    for root, _, files in os.walk(directory):
        if executable_name in files:
            return os.path.join(root, executable_name)
    return None


def run(build_dir):
    executable_name = EXECUTABLE_NAME
    if platform.system() == "Windows":
        executable_name += ".exe"

    executable_path = find_executable_path(build_dir, executable_name)
    if not executable_path:
        print("Error finding executable, exiting...")
        exit(1)

    subprocess.run(executable_path, shell=True)


def cmake_build(mode):
    print(f"Building in {mode} mode")
    if platform.system() == "Windows":
        parallel_flag = "/m:"
    else:
        parallel_flag = "-j"
    cmake_build_command = [
        "cmake",
        "--build",
        ".",
        "--",
        f"{parallel_flag} {str(os.cpu_count()-2)}",
    ]
    build_process = subprocess.Popen(cmake_build_command)
    build_process.wait()

    if build_process.returncode != 0:
        print("Build failed. Exiting...")
        exit(1)


def clean():
    for mode in BUILD_MODES:
        dir_name = f"build-{mode.lower()}"
        if os.path.exists(dir_name):
            shutil.rmtree(dir_name)
    os.remove(COMPILE_DB_NAME)


def main():
    parser = argparse.ArgumentParser(description=f"{PROJECT_NAME} build tools")
    parser.add_argument(
        "-r", "--run", help="Build and run the program", action="store_true"
    )
    parser.add_argument("-b", "--build", help="Build the program", action="store_true")
    parser.add_argument(
        "--mode",
        help="CMake build type (default Debug)",
        type=str,
        choices=BUILD_MODES,
        metavar="BUILD_MODE",
        default="Debug",
    )
    parser.add_argument("-c", help="Configure", action="store_true")
    parser.add_argument("--clean", help="Remove all build files", action="store_true")

    args = parser.parse_args()
    if not (args.c or args.clean or args.mode or args.run):
        parser.print_usage()
        return

    if args.clean:
        clean()

    build_dir = None

    if args.c:
        build_dir = f"{BUILD_DIR_NAME}/{args.mode}"
        os.makedirs(build_dir, exist_ok=True)
        os.chdir(build_dir)
        cmake_generate(args.mode)
        os.chdir("../..")

    if args.build or args.run:
        build_dir = f"{BUILD_DIR_NAME}/{args.mode}"
        os.chdir(build_dir)
        cmake_build(args.mode)
        # symlink
        os.chdir("../..")
        if os.path.exists(COMPILE_DB_NAME) or os.path.islink(COMPILE_DB_NAME):
            os.remove(COMPILE_DB_NAME)
        os.symlink(os.path.join(build_dir, COMPILE_DB_NAME), COMPILE_DB_NAME)

    if args.run:
        print("Running", build_dir)
        run(build_dir)


if __name__ == "__main__":
    main()
