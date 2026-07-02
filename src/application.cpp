//
// Created by Renkai on 01/07/2026.
//
#define VK_NO_PROTOTYPES
#include "application.h"

#define VOLK_IMPLEMENTATION
#include <volk.h>
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"


#include <iostream>
#include <ostream>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>



void Application::run()
{
    running = true;
    while (running)
    {
        SDL_Event event{0};
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                running = false;
                break;
            case SDL_EVENT_WINDOW_RESIZED:
                width = event.window.data1;
                height = event.window.data2;
                break;
            }
        }
        //render();
    }
}

bool Application::initialize()
{
    if (SDL_InitSubSystem(SDL_INIT_VIDEO))
    {
        window = SDL_CreateWindow("Ray Tracer",
            width, height,
            SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
        if (!window)
        {
            std::cout<<"Error creating window"<<std::endl;
            return false;
        }
        // if (!initializeVulkan())
        // {
        //     return false;
        // }

    }
    else
    {
        std::cout<<"unable to initialize SDL3"<<std::endl;
        return false;
    }
    return true;
}

bool Application::initializeVulkan()
{
    if (!createVulkanInstance())
    {
        std::cout<<"Error creating vulkan instance"<<std::endl;
        return false;
    }
    if (!createSurface())
    {
        std::cout<<"Error creating surface"<<std::endl;
        return false;
    }
    if (physicalDevice = findPhysicalDevice(); !physicalDevice)
    {
        std::cout<<"Error finding physical device"<<std::endl;
        return false;
    }
    if (!findGraphicsQueue())
    {
        std::cout<<"Error finding grapihcs queue"<<std::endl;
        return false;
    }
    if (!createDevice(physicalDevice))
    {
        std::cout<<"Error creating device"<<std::endl;
        return false;
    }
    if (!initializeVMA())
    {
        std::cout<<"Error initializing vma"<<std::endl;
        return false;
    }
    return true;
}

bool Application::createVulkanInstance()
{
    //initialize volk and load vk function pointers
    if(volkInitialize() != VK_SUCCESS)
    {
        std::cout<<"Error initializing volk"<<std::endl;
        return false;
    }

    // create the vulkan application instance
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Ray Tracer",
        .apiVersion = VulkanVersion,
    };

    uint32_t instanceExtensionCount = 0;
    const char *const *extensions = SDL_Vulkan_GetInstanceExtensions(&instanceExtensionCount);
    std::vector<const char *> requiredExtensions
    {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
    };
    requiredExtensions.resize(requiredExtensions.size() + instanceExtensionCount);
    for (uint32_t i = requiredExtensions.size(); i < requiredExtensions.size() + instanceExtensionCount; ++i)
    {
        requiredExtensions[i] = extensions[i];
    }

    // we also need to enable the validation layer for error checking  and reporting
    std::vector<const char *> requiredLayers
    {
        "VK_LAYERS_KHRONOS_validation"
    };

    VkDebugUtilsMessengerCreateInfoEXT debugInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debugCallback
    };

    VkInstanceCreateInfo instanceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = &debugInfo,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
        .ppEnabledLayerNames = requiredLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
        .ppEnabledExtensionNames = requiredExtensions.data(),
    };

    if (vkCreateInstance(&instanceCreateInfo, nullptr, &vulkanInstance) != VK_SUCCESS)
    {
        return false;
    }
    volkLoadInstance(vulkanInstance);
    return true;
}

bool Application::createSurface()
{
    if (!SDL_Vulkan_CreateSurface(window, vulkanInstance, nullptr, &surface))
    {
        return false;
    }
    return true;
}

VkPhysicalDevice Application::findPhysicalDevice()
{
    // enumerate all physical device
    uint32_t physicalDeviceCount = 0;
    vkEnumeratePhysicalDevices(vulkanInstance, &physicalDeviceCount, nullptr);
    std::vector<VkPhysicalDevice> physicalDevices;
    physicalDevices.resize(physicalDeviceCount);
    vkEnumeratePhysicalDevices(vulkanInstance, &physicalDeviceCount, physicalDevices.data());

    VkPhysicalDevice physicalDevice = nullptr;

    if (physicalDeviceCount)
    {
        physicalDevice = physicalDevices[0];
        // look through list and see if a dGPU exits
        for (auto& pDevice : physicalDevices)
        {
            VkPhysicalDeviceProperties physicalDeviceProperties;
            vkGetPhysicalDeviceProperties(pDevice, &physicalDeviceProperties);
            if (physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                physicalDevice = pDevice;
                break;
            }
        }
    }
    return physicalDevice;
}

bool Application::findGraphicsQueue()
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties2> queueFamProps(queueFamilyCount,
        {.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2});
    vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &queueFamilyCount, queueFamProps.data());

    for (int currentFamIdx = 0; currentFamIdx < queueFamProps.size(); ++currentFamIdx)
    {
        //ensure it has presentation support
        VkBool32 hasPresentationSuport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, currentFamIdx, surface, &hasPresentationSuport);

        const auto &props = queueFamProps[currentFamIdx];
        //ensure this is a GRAPHICS queue with presentation support
        if (props.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT && hasPresentationSuport)
        {
            gfxQueueFamIdx = currentFamIdx;
            return true;
        }
    }
    return false;
}

bool Application::createDevice(VkPhysicalDevice physicalDevice)
{
    // query supported feature
    VkPhysicalDeviceVulkan14Features supportedFeatures14 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES, .pNext = nullptr
    };
    VkPhysicalDeviceVulkan13Features supportedFeatures13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES, .pNext = &supportedFeatures14
    };
    VkPhysicalDeviceVulkan12Features supportedFeatures12 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES, .pNext = &supportedFeatures13
    };
    VkPhysicalDeviceFeatures2 supportedFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = &supportedFeatures12
    };
    vkGetPhysicalDeviceFeatures2(physicalDevice, &supportedFeatures);

    // check if what we need is supported
    if (!supportedFeatures13.dynamicRendering || !supportedFeatures13.synchronization2 || !supportedFeatures12.timelineSemaphore)
    {
        std::cerr << "physical device doesn't meet the feature requirements" << std::endl;
        return false;
    }

    // produce a seperate features struct chain for device creation
    VkPhysicalDeviceVulkan14Features features14 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES,
        .pNext = nullptr,
    };

    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = &features14,
        .synchronization2 = VK_TRUE,
        .dynamicRendering = VK_TRUE,
    };
    VkPhysicalDeviceVulkan12Features features12 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .pNext = &features13,
        .timelineSemaphore = VK_TRUE,
    };
    VkPhysicalDeviceFeatures2 features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &features12,
    };

    // request the queue we will use
    std::vector<float> queuePriorities = {1.0f};
    VkDeviceQueueCreateInfo gfxQueueInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = gfxQueueFamIdx,
        .queueCount = 1,
        .pQueuePriorities = queuePriorities.data()
    };

    // device specific extensions
    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        "VK_KHR_portability_subset"
    };

    VkDeviceCreateInfo deviceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &features,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &gfxQueueInfo,
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data(),
        .pEnabledFeatures = nullptr,
    };

    if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS)
    {
        std::cerr << "failed to create logical device" << std::endl;
        return false;
    }

    //grab the VKQueue object finally
    vkGetDeviceQueue(device, gfxQueueFamIdx, 0, &gfxQueue);
    if (!gfxQueue)
    {
        std::cerr << "failed to get graphics queue family" << std::endl;
        return false;
    }

    return true;
}

bool Application::initializeVMA()
{
    VmaVulkanFunctions vmaFuncInfo;
    VmaAllocatorCreateInfo vmaAllocatorCreateInfo
    {
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = physicalDevice,
        .device = device,
        .pVulkanFunctions = &vmaFuncInfo,
        .instance = vulkanInstance,
        .vulkanApiVersion = VulkanVersion
    };

    // vma can import directly from volk
    vmaImportVulkanFunctionsFromVolk(&vmaAllocatorCreateInfo, &vmaFuncInfo);

    if (vmaCreateAllocator(&vmaAllocatorCreateInfo, &vmaAllocator) != VK_SUCCESS)
    {
        std::cerr << "failed to create allocator" << std::endl;
        return false;
    }
    return true;
}

bool Application::createSwapchain(uint32_t width, uint32_t height)
{

}

void Application::shutdown()
{
    // other shutdown
    //VMA
    if (vmaAllocator)
    {
        vmaDestroyAllocator(vmaAllocator);
    }
    // cleanup vulkan
    if (surface)
    {
        vkDestroySurfaceKHR(vulkanInstance, surface, nullptr);
    }
    if (device)
    {
        vkDestroyDevice(device, nullptr);
    }
    if (vulkanInstance)
    {
        vkDestroyInstance(vulkanInstance, nullptr);
    }

    volkFinalize();
    //clean up SDL
    if (window)
    {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
}
