# XXX_CMAKE_PACKAGE_PATH is use for cmake find_package(setting CMAKE_PREFIX_PATH)
# if XXX_CMAKE_PACKAGE_PATH is not set, cmake will search XXX_INCLUDE_PATH and XXX_LIB_PATH
class CMakeConfig:
    BUILD_DIR = "build"
    BUILD_TYPE = "Debug" # Release

    ENABLE_BOOST_LOG = 1
    
    # GLAD
    # https://glad.dav1d.de/    
    # Profile=Core
    
    # GLFW
    # https://www.glfw.org/download.html
    GLFW_VERSION = "3.4"
    GLFW_CMAKE_PACKAGE_PATH = r''
    GLFW_INCLUDE_PATH = r'E:/workspace/glfw-3.4.bin.WIN64/include'
    GLFW_LIB_PATH = r'E:/workspace/glfw-3.4.bin.WIN64/lib-vc2022/'
    
    # BOOST
    # https://www.boost.org/users/history/version_1_88_0.html
    # bootstrap.bat
    # b2.exe toolset=msvc-14.3 install --prefix="E:\workspace\boost_vs2022" link=static
    # https://blog.csdn.net/nanke_yh/article/details/124346308
    BOOST_VERSION = "1.88.0"
    BOOST_CMAKE_PACKAGE_PATH = r'E:\workspace\boost_vs2022'
    BOOST_INCLUDE_PATH = r''
    BOOST_LIB_PATH = r''

    # GTEST
    # https://github.com/google/googletest/releases
    # cmake directly 
    GTEST_VERSION = "1.17.0"
    GTEST_CMAKE_PACKAGE_PATH = r''
    GTEST_INCLUDE_PATH = r'E:/workspace/googletest-release-1.13.0/googletest/include'
    GTEST_LIB_PATH = r'E:/workspace/googletest-release-1.13.0/googlemock/lib/x64/Debug'
    
    # POCO
    # https://github.com/pocoproject/poco/tree/poco-1.14.2-release
    # mkdir cmake-build
    # cd cmake-build
    # cmake .. -DCMAKE_INSTALL_PREFIX=XXX
    # cmake --build . --config Release --target install
    POCO_VERSION = "1.14.2"
    POCO_CMAKE_PACKAGE_PATH = r'E:\workspace\poco_vs2022'
    POCO_INCLUDE_PATH = r''
    POCO_LIB_PATH = r''