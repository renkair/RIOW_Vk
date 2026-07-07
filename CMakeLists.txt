cmake_minimum_required(VERSION 4.1)
project(RIOW_Vk)

set(CMAKE_CXX_STANDARD 20)

include_directories(external/glm)


if(WIN32)
    message("Platform: Windows")
elseif(UNIX)
    if(APPLE)
        message("Platform:MAC")
    else()
        message("Platform:Linux")
    endif ()
else ()
    message("Platform:Unsupported")
endif ()



find_package(SDL3 REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED COMPONENTS volk shaderc_combined)



if(Vulkan_FOUND)
    message("find Vulkan success: ${Vulkan_INCLUDE_DIRS}")
else ()
    message("Didn't find vulkan!")
endif ()

add_executable(RIOW_Vk src/main.cpp
        src/application.cpp
        src/application.h
        src/utils.cpp
        src/utils.h
        src/ImGuiRenderer.cpp
        src/ImGuiRenderer.h
)

add_subdirectory(external/VulkanMemoryAllocator)

add_library(imgui STATIC
        external/imgui/imgui.cpp
        external/imgui/imgui_draw.cpp
        external/imgui/imgui_widgets.cpp
        external/imgui/imgui_tables.cpp
        external/imgui/imgui_demo.cpp
        external/imgui/backends/imgui_impl_sdl3.cpp
        external/imgui/backends/imgui_impl_vulkan.cpp
)
target_include_directories(imgui PUBLIC
        external/imgui
        external/imgui/backends
)
target_link_libraries(imgui PRIVATE
        SDL3::SDL3
        Vulkan::Vulkan
)

target_link_libraries(RIOW_Vk  PRIVATE
        Vulkan::Vulkan
        Vulkan::volk
        Vulkan::shaderc_combined
        SDL3::SDL3
        GPUOpen::VulkanMemoryAllocator
        imgui
)

