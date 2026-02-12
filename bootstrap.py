import os
import json
import subprocess
import logging
import sys
from config import *

logging.basicConfig(level=logging.INFO, format='[%(asctime)s][%(levelname)s] %(message)s [%(filename)s:%(lineno)s]')

CMakeConfig.BUILD_DIR = os.path.abspath(CMakeConfig.BUILD_DIR)
CMAKE_PARAMS = {key: value for key, value in CMakeConfig.__dict__.items() if not key.startswith("__")}
cmake_params = []

if CMakeConfig.BUILD_TYPE == "Debug":
    cmake_params.append("-DCMAKE_BUILD_TYPE=Debug ")
else:
    cmake_params.append("-DCMAKE_BUILD_TYPE=Release ")

for k, v in CMAKE_PARAMS.items():
    logging.info(f"{k}: {v}")
    if isinstance(v, str):
        cmake_params.append(f"-D{k}:STRING={v} ")
    elif isinstance(v, int):
        cmake_params.append(f"-D{k}={v} ")
    else:
        raise Exception(f"{k}:{v} value type is not supported")

if not os.path.exists(CMakeConfig.BUILD_DIR):
    os.makedirs(CMakeConfig.BUILD_DIR)
    logging.info(f"Create build dir: {CMakeConfig.BUILD_DIR}")
    
p = subprocess.Popen("cmake .. -G \"Visual Studio 17 2022\" " + "".join(cmake_params), cwd=CMakeConfig.BUILD_DIR, shell=True)

p.wait()

if p.returncode == 0:
    logging.info("CMake configure success!")
else:
    logging.info("CMake configure failed!")
    
# TODO(wye): Add include directory, DEFINES to VSCode .c_cpp_properties.json
vscode_cpp_json_file = os.path.join(os.getcwd(), ".vscode", "c_cpp_properties.json")
if not os.path.exists(vscode_cpp_json_file):
    logging.info(f"Please create {vscode_cpp_json_file} manually! F1 -> C/C++ Edit Configurations\n Execute bootstrap.py again.")
else:
    cpp_json = None
    with open(vscode_cpp_json_file, "r") as f:
        cpp_json = json.load(f)
    if cpp_json is not None:
        for k, v in CMAKE_PARAMS.items():
            if isinstance(v, str):
                if k.endswith("_INCLUDE_PATH") and len(v) > 0 and os.path.exists(v):
                    if v not in cpp_json["configurations"][0]["includePath"]:
                        cpp_json["configurations"][0]["includePath"].append(v)
            if isinstance(v, int) and v == 1:
                logging.info(f"Add define: {k}")
                if k not in cpp_json["configurations"][0]["defines"]:
                    cpp_json["configurations"][0]["defines"].append(k)

        with open(vscode_cpp_json_file, "w") as f:
            json.dump(cpp_json, f, indent=4)
        logging.info(f"Update {vscode_cpp_json_file} success!")
    else:
        logging.info(f"Update {vscode_cpp_json_file} failed, please create {vscode_cpp_json_file} manually! F1 -> C/C++ Edit Configurations\n Execute bootstrap.py again.")

# TODO(wye): Execute vscode cmake-tools build
logging.info("Please execute vscode cmake-tools build manually!")