cmake_minimum_required(VERSION 4.1)
project(RIOW_Vk)

set(CMAKE_CXX_STANDARD 20)

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
find_package(glfw3 REQUIRED)
#Vulkan
find_package(Vulkan REQUIRED)

if(Vulkan_FOUND)
    message("find Vulkan success: ${Vulkan_INCLUDE_DIRS}")
else ()
    message("Didn't find vulkan!")
endif ()

add_executable(RIOW_Vk main.cpp)
target_link_libraries(RIOW_Vk  PRIVATE
        glfw
        Vulkan::Vulkan
)


