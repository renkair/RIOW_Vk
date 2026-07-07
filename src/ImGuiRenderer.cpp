//
// Created by Renkai on 07/07/2026.
//

#include "ImGuiRenderer.h"

#include <cstdlib>

#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"
#include "imgui.h"


ImGuiRenderer::ImGuiRenderer(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device,
    uint32_t gfxFamilyIdx, VkQueue gfxQueue, SDL_Window* window) : instance(instance), physicalDevice(physicalDevice), device(device), gfxFamilyIdx(gfxFamilyIdx), gfxQueue(gfxQueue), window(window)
{
    init();
}

ImGuiRenderer::~ImGuiRenderer()
{
    shutdown();
}

static void check_vk_result(VkResult err)
{
    if (err == VK_SUCCESS)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        std::abort();
}

void ImGuiRenderer::init()
{
    createDescriptorPools();
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    //io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForVulkan(window);



    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = instance;
    init_info.PhysicalDevice = physicalDevice;
    init_info.Device = device;
    init_info.QueueFamily = gfxFamilyIdx;
    init_info.Queue = gfxQueue;
    //init_info.PipelineCache = YOUR_PIPELINE_CACHE;
    init_info.DescriptorPool = descriptorPool;
    init_info.MinImageCount = 2;
    init_info.ImageCount = 2;
    //init_info.Allocator = YOUR_ALLOCATOR;
    //init_info.PipelineInfoMain.RenderPass = wd->RenderPass;

    // - About dynamic rendering:
    //   - When using dynamic rendering, set UseDynamicRendering=true + fill PipelineInfoMain.PipelineRenderingCreateInfo structure.
    init_info.UseDynamicRendering = true;
    VkFormat colorFormat = VK_FORMAT_B8G8R8A8_SRGB;
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &colorFormat,
        .depthAttachmentFormat = VK_FORMAT_D32_SFLOAT
    };

    init_info.PipelineInfoMain.Subpass = 0;
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    //init_info.CheckVkResultFn = check_vk_result;


    ImGui_ImplVulkan_Init(&init_info);
}

void ImGuiRenderer::shutdown()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiRenderer::createDescriptorPools()
{
    // Create Descriptor Pool
    // If you wish to load e.g. additional textures you may need to alter pools sizes and maxSets.
    {
        VkDescriptorPoolSize pool_sizes[] =
        {
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, IMGUI_IMPL_VULKAN_MINIMUM_SAMPLED_IMAGE_POOL_SIZE },
            { VK_DESCRIPTOR_TYPE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_SAMPLER_POOL_SIZE },
        };
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 0;
        for (VkDescriptorPoolSize& pool_size : pool_sizes)
            pool_info.maxSets += pool_size.descriptorCount;
        pool_info.poolSizeCount = (uint32_t)IM_COUNTOF(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        auto err = vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptorPool);
        check_vk_result(err);
    }
}
