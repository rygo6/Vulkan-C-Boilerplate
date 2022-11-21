project(Fabric)

set(TARGET_NAME fabric_ovr_vulkan)
cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 17)

set(VULKAN_SDK_PATH "C:/VulkanSDK/1.3.231.1")
set(GLFW_SDK_PATH "C:/Developer/glfw-3.3.8.bin.WIN64")

#set(VULKAN_SDK_PATH "/mnt/c/VulkanSDK/1.3.231.1")
#set(GLFW_SDK_PATH "/mnt/c/Developer/glfw-3.3.8.bin.WIN64")

file(GLOB SRC_FILES
        src/*.c
        src/*.h
        )

add_executable(${TARGET_NAME}
        ${SRC_FILES}
)

target_link_directories(${TARGET_NAME} PUBLIC
        lib
        "${VULKAN_SDK_PATH}/Lib"
        "${GLFW_SDK_PATH}/lib-mingw-w64"
)
target_link_libraries(${TARGET_NAME}
        glfw3
        gdi32
        vulkan-1
        )

target_include_directories(${TARGET_NAME} PUBLIC
        include
        "${VULKAN_SDK_PATH}/Include"
        "${GLFW_SDK_PATH}/include"
)
