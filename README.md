# Vulkan-C-Boilerplate

Boilerplate for vulkan hello triangle in C.

This code is derived from https://vulkan-tutorial.com/ with some alterations and simplification taken from the [SteamVR OVR Vulkan Sample](https://github.com/ValveSoftware/openvr/tree/master/samples/hellovr_vulkan), then rewritten in C. 

Currently compiles under mingw gcc on Windows.

You have to run compile.bat in shaders directory to compile the shaders. Then you might need to copy the shader folder next to the exe if it doesn't build the exe to root.

This wants GLFW and Vulkan headers, so ensure the SDK to those are installed and the paths in CMakeLists.txt are correct.