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
)

add_subdirectory(external/VulkanMemoryAllocator)

target_link_libraries(RIOW_Vk  PRIVATE
        Vulkan::Vulkan
        Vulkan::volk
        Vulkan::shaderc_combined
        SDL3::SDL3
        GPUOpen::VulkanMemoryAllocator
)


