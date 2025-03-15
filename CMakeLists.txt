cmake_minimum_required(VERSION 3.10)
project(jbl_heartbeat VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加可执行文件
add_executable(jbl_heartbeat WIN32
    main.cpp
    resource.rc
)

# 添加Windows库
target_link_libraries(jbl_heartbeat PRIVATE
    winmm
    comctl32
    user32
    shell32
    ole32
    advapi32
)

# 复制图标文件到构建目录
configure_file(${CMAKE_SOURCE_DIR}/alert_running.ico ${CMAKE_BINARY_DIR}/alert_running.ico COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/alert_paused.ico ${CMAKE_BINARY_DIR}/alert_paused.ico COPYONLY)

# 安装目标
install(TARGETS jbl_heartbeat DESTINATION bin)
install(FILES
    ${CMAKE_SOURCE_DIR}/alert_running.ico
    ${CMAKE_SOURCE_DIR}/alert_paused.ico
    DESTINATION bin
)

# 添加打包支持
include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "jbl_heartbeat")
set(CPACK_PACKAGE_EXECUTABLES "jbl_heartbeat" "JBL Heartbeat")
include(CPack)