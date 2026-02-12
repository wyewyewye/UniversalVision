# XXX_CMAKE_PACKAGE_PATH is use for cmake find_package(setting CMAKE_PREFIX_PATH)
# XXX_INCLUDE_PATH is use for vscode c_cpp_properties.json
# if XXX_CMAKE_PACKAGE_PATH is not set, cmake will search XXX_INCLUDE_PATH and XXX_LIB_PATH
class CMakeConfig:
    BUILD_DIR = "build"
    BUILD_TYPE = "Debug" # Release
    DEBUG = 1 if BUILD_TYPE == "Debug" else 0

    ENABLE_TEST = 1
    ENABLE_BOOST_LOG = 1
    
    # GLAD
    # https://glad.dav1d.de/    
    # Profile=Core
    
    # GLFW
    # https://www.glfw.org/download.html
    GLFW_VERSION = "3.4"
    GLFW_CMAKE_PACKAGE_PATH = r''
    GLFW_INCLUDE_PATH = r'D:\workspace\cpp_project\glfw-3.4.bin.WIN64\include'
    GLFW_LIB_PATH = r'D:\workspace\cpp_project\glfw-3.4.bin.WIN64\lib-vc2022'
    
    # BOOST
    # https://www.boost.org/users/history/version_1_88_0.html
    # bootstrap.bat
    # b2.exe toolset=msvc-14.3 install --prefix="D:\workspace\cpp_project\boost_vs2022" link=static
    # https://blog.csdn.net/nanke_yh/article/details/124346308
    BOOST_VERSION = "1.88.0"
    BOOST_CMAKE_PACKAGE_PATH = r'D:\workspace\cpp_project\boost_vs2022'
    BOOST_INCLUDE_PATH = r'D:\workspace\cpp_project\boost_vs2022\include\boost-1_88'
    BOOST_LIB_PATH = r''

    # GTEST
    # https://github.com/google/googletest/releases
    # cmake .. -DCMAKE_INSTALL_PREFIX="D:\workspace\cpp_project\gtest_vs2022"
    # cmake --build . --config Debug --target install
    GTEST_VERSION = "1.17.0"
    GTEST_CMAKE_PACKAGE_PATH = r'D:\workspace\cpp_project\gtest_vs2022'
    GTEST_INCLUDE_PATH = r'D:\workspace\cpp_project\gtest_vs2022\include'
    GTEST_LIB_PATH = r''
    
    # POCO
    # https://github.com/pocoproject/poco/tree/poco-1.14.2-release
    # mkdir cmake-build
    # cd cmake-build
    # cmake .. -DCMAKE_INSTALL_PREFIX="D:\workspace\cpp_project\poco_vs2022"
    # cmake --build . --config Debug --target install
    POCO_VERSION = "1.14.2"
    POCO_CMAKE_PACKAGE_PATH = r'D:\workspace\cpp_project\poco_vs2022'
    POCO_INCLUDE_PATH = r'D:\workspace\cpp_project\poco_vs2022\include'
    POCO_LIB_PATH = r''
    
    # FFMPEG
    # https://www.ffmpeg.org/releases/ffmpeg-4.4.6.tar.gz   
    # 直接使用编译好的库或者使用msys2编译
    FFMPEG_VERSION = "4.4.6"
    FFMPEG_CMAKE_PACKAGE_PATH = r'D:\workspace\cpp_project\ffmpeg_vs2022'
    FFMPEG_INCLUDE_PATH = r''
    FFMPEG_LIB_PATH = r''