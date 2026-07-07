//
// Created by Renkai on 01/07/2026.
//

#ifndef RIOW_VK_APPLICATION_H
#define RIOW_VK_APPLICATION_H

#include <volk.h>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <SDL3/SDL_video.h>
#include <shaderc/shaderc.hpp>

#include "imgui.h"


class ImGuiRenderer;
struct VmaAllocator_T;
typedef struct VmaAllocator_T *VmaAllocator;
struct VmaAllocation_T;
typedef struct VmaAllocation_T *VmaAllocation;

struct Pipeline
{
    VkPipelineLayout layout = nullptr;
    VkPipeline handle = nullptr;
};

struct FrameResources
{
    VkCommandPool commandPool = nullptr;
    VkCommandBuffer commandBuffer = nullptr;
    VkSemaphore imageAcquiredSemaphore = nullptr;
};
class Application
{
public:

    void run();
    bool initialize();
    void shutdown();
private:

    bool running = false;
    bool initializeVulkan();
    bool initializeVMA();
    bool createVulkanInstance();
    bool createSurface();
    VkPhysicalDevice findPhysicalDevice();
    bool findGraphicsQueue();
    bool createDevice(VkPhysicalDevice physicalDevice);
    bool createSwapchain(uint32_t width, uint32_t height);
    bool createShaders();
    bool createCommandBuffers();
    VkShaderModule createShaderModule(const std::string & fileName, shaderc_shader_kind kind) const;
    VkPipeline createGraphicsPipeline();
    bool createSyncResources();
    void render();
    void destroySwapchain();
    constexpr static uint32_t VulkanVersion{ VK_API_VERSION_1_4 };
    constexpr static uint32_t MaxFramesInFlight{ 2 };
    constexpr static VkFormat swapchainFormat{ VK_FORMAT_B8G8R8A8_SRGB };
    constexpr static VkFormat depthFormat{ VK_FORMAT_D32_SFLOAT};
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                        VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                        void* pUserData)
    {
        if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        {
            std::cerr << pCallbackData->pMessage << std::endl;
        }
        return VK_FALSE;
    }

    uint32_t width = 1280;
    uint32_t height = 720;
    SDL_Window* window = nullptr;


    // vk core
    VkInstance vulkanInstance = nullptr;
    VkSurfaceKHR surface = nullptr;
    VkDevice device = nullptr;
    VmaAllocator vmaAllocator = nullptr;
    // queue related
    uint32_t gfxQueueFamIdx = UINT32_MAX;
    VkQueue gfxQueue = nullptr;
    // swapchain related
    VkSwapchainKHR swapchain = nullptr;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkSemaphore> renderCompleteSemaphores;
    bool requiredSwapchainRecreated = false;
    uint32_t swapchainWidth = 0;
    uint32_t swapchainHeight = 0;

    VkImage depthImage = nullptr;
    VkImageView depthImageView = nullptr;
    VmaAllocation depthImageAllocation = nullptr;

    // shader resources
    VkShaderModule vertShader = nullptr;
    VkShaderModule fragShader = nullptr;

    // graphics pipeline related
    Pipeline pipeline;

    // frame and synchronization resources
    VkSemaphore timelineSemaphore = nullptr;
    std::array<FrameResources, MaxFramesInFlight> frameResources;

    uint32_t frameCounter = 0;
    uint64_t timelineValue = MaxFramesInFlight - 1; // subtract 1 to ensure wait-for-ID / frame resource index start at 0 during render, avoids if (frameId < MaxFramesInFlight) check

    ImGuiRenderer* imGuiRenderer = nullptr;


};




#endif //RIOW_VK_APPLICATION_H
