project(Fabric)

set(TARGET_NAME fabric_ovr_vulkan)
cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 17)

file(GLOB SRC_FILES
        src/*.cpp
        src/*.h
        )

add_executable(${TARGET_NAME}
        ${SRC_FILES}
        )

target_link_directories(${TARGET_NAME} PUBLIC
        thirdparty/sdl2-2.0.3/bin/win64
        thirdparty/sdl2-2.0.3/bin/win64
        thirdparty/vulkan-1.0.49.0/lib/win64
        thirdparty/openvr/lib/win64
)
target_link_libraries(${TARGET_NAME}
        SDL2.lib
        SDL2main.lib
        vulkan-1.lib
        openvr_api.lib
        )

target_include_directories(${TARGET_NAME} PUBLIC
        thirdparty/sdl2-2.0.3/include
        thirdparty/vulkan-1.0.49.0/include
        thirdparty/openvr/headers
)

