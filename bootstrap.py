import os
import subprocess
from config import *

CMakeConfig.BUILD_DIR = os.path.abspath(CMakeConfig.BUILD_DIR)
CMAKE_PARAMS = {key: value for key, value in CMakeConfig.__dict__.items() if not key.startswith("__")}
cmake_params = []

for k, v in CMAKE_PARAMS.items():
    print(f"{k}: {v}")
    cmake_params.append(f"-D{k}:STRING={v} ")

if not os.path.exists(CMakeConfig.BUILD_DIR):
    os.makedirs(CMakeConfig.BUILD_DIR)
    print(f"Create build dir: {CMakeConfig.BUILD_DIR}")
    
p = subprocess.Popen("cmake .. -G \"Visual Studio 17 2022\" " + "".join(cmake_params), cwd=CMakeConfig.BUILD_DIR, shell=True)

p.wait()

if p.returncode == 0:
    print("CMake configure success!")
else:
    print("CMake configure failed!")