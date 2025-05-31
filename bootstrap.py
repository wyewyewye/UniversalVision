import os
import subprocess
from config import *

CMakeConfig.BUILD_DIR = os.path.abspath(CMakeConfig.BUILD_DIR)
CMAKE_PARAMS = {key: value for key, value in CMakeConfig.__dict__.items() if not key.startswith("__")}
cmake_params = []

if CMakeConfig.BUILD_TYPE == "Debug":
    cmake_params.append("-DCMAKE_BUILD_TYPE=Debug ")
else:
    cmake_params.append("-DCMAKE_BUILD_TYPE=Release ")

for k, v in CMAKE_PARAMS.items():
    print(f"{k}: {v}")
    if isinstance(v, str):
        cmake_params.append(f"-D{k}:STRING={v} ")
    elif isinstance(v, int):
        cmake_params.append(f"-D{k}={v} ")
    else:
        raise Exception(f"{k}:{v} value type is not supported")

if not os.path.exists(CMakeConfig.BUILD_DIR):
    os.makedirs(CMakeConfig.BUILD_DIR)
    print(f"Create build dir: {CMakeConfig.BUILD_DIR}")
    
p = subprocess.Popen("cmake .. -G \"Visual Studio 17 2022\" " + "".join(cmake_params), cwd=CMakeConfig.BUILD_DIR, shell=True)

p.wait()

if p.returncode == 0:
    print("CMake configure success!")
else:
    print("CMake configure failed!")