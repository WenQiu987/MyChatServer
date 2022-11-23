# 要求系统的最小CMake版本
cmake_minimum_required(VERSION 3.0)
# 设置工程名字，不是可执行文件名字
project(main)

# 配置编译选项 可以写上-g等编译选项，-g即是生成可调试的
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)

# 配置头文件搜索路径
# include_directories()
# 配置库文件搜索路径
# link_directories()

# 生成库文件，用于我们制作动态库和静态库
# add_library()

# 设置需要编译的源文件列表
set(SRC_LIST ./muduo_server.cpp)
# 设置可执行文件最终存储路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin) # 存在项目的根目录下的bin文件夹里
# 生成可执行文件，括号内：可执行文件名 需要的.cpp文件
add_executable(server ${SRC_LIST})
#目标可执行文件需要链接的库文件，要把可执行文件名也写上去，然后不需要-l了
target_link_libraries(server muduo_net muduo_base pthread)