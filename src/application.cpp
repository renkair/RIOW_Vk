//
// Created by Renkai on 01/07/2026.
//
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

#include "ImGuiRenderer.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"
#include "utils.h"


void Application::run()
{
    running = true;
    while (running)
    {
        SDL_Event event{0};
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
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

        // Start the Dear ImGui frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        ImGui::ShowDemoWindow(&show_demo_window);

        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            //ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        render();
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
        if (!initializeVulkan())
        {
            return false;
        }
        imGuiRenderer = new ImGuiRenderer(vulkanInstance, physicalDevice, device, gfxQueueFamIdx, gfxQueue, window);
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

    if (!createSwapchain(width, height))
    {
        std::cout<<"Error creating swapchain"<<std::endl;
        return false;
    }

    if (!createShaders())
    {
        std::cout<<"Error creating shaders"<<std::endl;
        return false;
    }
    if (pipeline.handle = createGraphicsPipeline(); !pipeline.handle)
    {
        std::cout<<"Error creating graphics pipeline"<<std::endl;
        return false;
    }
    if (!createSyncResources())
    {
        std::cout<<"Error creating sync resources"<<std::endl;
        return false;
    }
    if (!createCommandBuffers())
    {
        std::cout<<"Error creating command buffers"<<std::endl;
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
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
    };
    for (uint32_t i = 0; i < instanceExtensionCount; ++i)
    {
        requiredExtensions.push_back(extensions[i]);
    }

    // we also need to enable the validation layer for error checking  and reporting
    std::vector<const char *> requiredLayers
    {
        "VK_LAYER_KHRONOS_validation"
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
        .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR, /// this is for MAC
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
        "VK_KHR_portability_subset" // this is for MAC
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
    volkLoadDevice(device);
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
    // track swapchain seperate from window size
    swapchainWidth = width;
    swapchainHeight = height;

    // ensure we request  an appropriate number of images
    VkSurfaceCapabilitiesKHR surfaceCapabilities{};
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities) != VK_SUCCESS)
    {
        std::cerr << "failed to get surface capabilities" << std::endl;
        return false;
    }
    uint32_t requestedImageCount = std::max(2u, surfaceCapabilities.minImageCount);
    if (surfaceCapabilities.maxImageCount > 0)
    {
        requestedImageCount = std::min(requestedImageCount, surfaceCapabilities.maxImageCount);
    }

    VkSwapchainCreateInfoKHR swapchainCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = requestedImageCount,
        .imageFormat = swapchainFormat,
        .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = {.width = swapchainWidth, .height = swapchainHeight},
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = surfaceCapabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR
    };

    if (vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain) != VK_SUCCESS)
    {
        std::cerr << "failed to create swapchain" << std::endl;
        return false;
    }


    // ask for swapchain image
    uint32_t imageCount = 0;
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
    swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());
    swapchainImageViews.resize(imageCount);

    //create the imageview
    for (size_t i = 0; i < swapchainImageViews.size(); i++)
    {
        VkImageViewCreateInfo imageViewCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = swapchainImages[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = swapchainFormat,
            .subresourceRange
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        if (vkCreateImageView(device, &imageViewCreateInfo, nullptr, &swapchainImageViews[i]) != VK_SUCCESS)
        {
            std::cerr << "failed to create image view" << std::endl;
            return false;
        }

    }

    //semaphores used to signal render completion
    renderCompleteSemaphores.resize(swapchainImageViews.size());
    for (VkSemaphore &semaphore : renderCompleteSemaphores)
    {
        VkSemaphoreCreateInfo semaphoreCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
        };
        if (vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphore) != VK_SUCCESS)
        {
            std::cerr << "failed to create semaphore" << std::endl;
            return false;
        }
    }

    // create depth image
    VkImageCreateInfo depthCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = depthFormat,
        .extent = {.width = swapchainWidth, .height = swapchainHeight, .depth = 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    VmaAllocationCreateInfo allocCreateInfo = {
        .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO
    };
    if (vmaCreateImage(vmaAllocator, &depthCreateInfo, &allocCreateInfo, &depthImage,
        &depthImageAllocation, nullptr) != VK_SUCCESS)
    {
        std::cerr << "failed to create depth image" << std::endl;
        return false;
    }

    VkImageViewCreateInfo depthImageViewCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = depthImage,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = depthFormat,
        .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, .levelCount = 1, .layerCount = 1}
    };

    if (vkCreateImageView(device, &depthImageViewCreateInfo, nullptr, &depthImageView) != VK_SUCCESS)
    {
        std::cerr << "failed to create depth image view" << std::endl;
        return false;
    }
    return true;
}

bool Application::createShaders()
{
    // create the shader modules that we'll need for the graphics pipline
    if (vertShader = createShaderModule("shader.vert", shaderc_vertex_shader); !vertShader)
    {
        std::cerr << "failed to create shader" << std::endl;
        return false;
    }
    if (fragShader = createShaderModule("shader.frag", shaderc_fragment_shader); !fragShader)
    {
        std::cerr << "failed to create shader" << std::endl;
        return false;
    }
    return true;
}

bool Application::createCommandBuffers()
{
    for (FrameResources &res : frameResources)
    {
        // we'll give each frame its own pool, faster cmd buffer reset in this way
        VkCommandPoolCreateInfo commandPoolCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = gfxQueueFamIdx,
        };
        if (vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &res.commandPool) != VK_SUCCESS)
        {
            std::cerr << "failed to create command pool" << std::endl;
            return false;
        }
        // create the command buffer for this frame
        VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = res.commandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        if (vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &res.commandBuffer) != VK_SUCCESS)
        {
            std::cerr << "failed to allocate command buffers" << std::endl;
            return false;
        }
    }
    return true;
}

VkShaderModule Application::createShaderModule(const std::string & fileName, shaderc_shader_kind kind) const
{
    // read shader file from disk
    const std::string shaderPath = "src/shaders/"+fileName;
    const std::string src = readTextFile(shaderPath);
    if (src.empty())
    {
        std::cerr << "failed to read shader file" << std::endl;
        return nullptr;
    }

    // complie the shader to SPIR-V
    std::cout << "compiling shader" << shaderPath << std::endl;
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_4);
    options.SetTargetSpirv(shaderc_spirv_version_1_6);
    options.SetOptimizationLevel(shaderc_optimization_level_performance);
    shaderc::CompilationResult result = compiler.CompileGlslToSpv(src, kind, fileName.c_str(), options);

    if (result.GetCompilationStatus()!=shaderc_compilation_status_success)
    {
        std::cerr << "compilation failed" << std::endl;
        return nullptr;
    }

    const size_t shaderSize = (result.cend() - result.begin())*sizeof(uint32_t);
    // pass spir-v to vulkan and create shader-moudle
    VkShaderModuleCreateInfo shaderModuleCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = shaderSize,
        .pCode = result.cbegin()
    };

    VkShaderModule shaderModule = nullptr;
    if (vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        std::cerr << "failed to create shader module" << std::endl;
        return nullptr;
    }
    return shaderModule;
}

VkPipeline Application::createGraphicsPipeline()
{
    // need to define a pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 0,
        .pPushConstantRanges = 0
    };
    if (vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipeline.layout) != VK_SUCCESS)
    {
        std::cerr << "failed to create pipeline layout" << std::endl;
        return nullptr;
    }

    //configure the shader stage struct
    const char * entryPoint = "main";
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertShader,
            .pName = entryPoint
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragShader,
            .pName = entryPoint
        }
    };

    // vertex pulling, don't define vertex input details
    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    };

    // input assembly, we will be drawing triangle lists
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    };

    //depth/stencil configuration
    VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .stencilTestEnable = VK_FALSE,
    };

    // dynamic rendering allows to set this up...dynamically
    // we still need this struct though
    VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = nullptr,
        .scissorCount = 1,
        .pScissors = nullptr
    };

    // rasterizer settings
    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .lineWidth = 1.0f,
    };
    // no multiple sampling
    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    //Alpha-blending (disabled for now), still need attachment info and write mask
    VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };
    VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachmentState,
    };

    // enable dynamic state
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    // structure required for dynamic rendering
    VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &swapchainFormat,
        .depthAttachmentFormat = depthFormat,
    };

    // create the graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &pipelineRenderingCreateInfo,   /// to enable dynamic rendering
        .stageCount = static_cast<uint32_t>(shaderStages.size()),
        .pStages = shaderStages.data(),
        .pVertexInputState = &vertexInputStateCreateInfo,
        .pInputAssemblyState = &inputAssemblyStateCreateInfo,
        .pViewportState = &viewportStateCreateInfo,
        .pRasterizationState = &rasterizationStateCreateInfo,
        .pMultisampleState = &multisampleStateCreateInfo,
        .pDepthStencilState = &depthStencilStateCreateInfo,
        .pColorBlendState = &colorBlendStateCreateInfo,
        .pDynamicState = &dynamicStateCreateInfo,
        .layout = pipeline.layout,
        .renderPass = VK_NULL_HANDLE,
    };

    VkPipeline newPipeline;
    if (vkCreateGraphicsPipelines(device, nullptr, 1, &pipelineCreateInfo, nullptr, &newPipeline) != VK_SUCCESS)
    {
        std::cerr << "Failed to create pipeline" << std::endl;
        return nullptr;
    }
    return newPipeline;

}

bool Application::createSyncResources()
{
    VkSemaphoreTypeCreateInfo semaphoreTypeInfo
    {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
        .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
        .initialValue = timelineValue
    };

    VkSemaphoreCreateInfo semaphoreCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = &semaphoreTypeInfo,
    };

    if (vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &timelineSemaphore) != VK_SUCCESS)
    {
        std::cerr << "Failed to create semaphore" << std::endl;
        return false;
    }
    // per-frame image-require semaphores
    for (FrameResources &res : frameResources)
    {
        // create binary semaphores
        VkSemaphoreCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };
        if (vkCreateSemaphore(device, &createInfo, nullptr, &res.imageAcquiredSemaphore) != VK_SUCCESS)
        {
            std::cerr << "Failed to create pre-frame acquire semaphore" << std::endl;
            return false;
        }
    }

    return true;

}

void Application::render()
{
    // first check if our swapchain is still valid
    if(requiredSwapchainRecreated)
    {
        vkDeviceWaitIdle(device);
        destroySwapchain();
        createSwapchain(width, height);
        requiredSwapchainRecreated = false;
    }
    const uint32_t frameResIndex = frameCounter++ % MaxFramesInFlight;
    // wait for frame using this frame's resources to complete
    uint64_t frameId = ++timelineValue; // this is our frame "ID", and what we're using to signal the end of this frame later
    uint64_t waitForId = frameId - MaxFramesInFlight; // frame N and frame N - MaxInFlight share resources (3 - 2 = 1 -- frame 3 and 1 share resources)

    VkSemaphoreWaitInfo semaphoreWaitInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
        .semaphoreCount = 1,
        .pSemaphores = &timelineSemaphore,
        .pValues = &waitForId
    };
    vkWaitSemaphores(device, &semaphoreWaitInfo, UINT64_MAX);

    // now its safe to start recording commands
    FrameResources &res = frameResources[frameResIndex];
    vkResetCommandPool(device, res.commandPool, 0);

    // get resource for this frame
    VkSemaphore imageAcquiredSemaphore = frameResources[frameResIndex].imageAcquiredSemaphore;
    uint32_t imageIndex = 0;
    VkResult acquireResult = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAcquiredSemaphore, nullptr, &imageIndex);
    // handle resize and out-of-date images, may need swapchain recreate
    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        requiredSwapchainRecreated = true;
        return;
    }
    else if (acquireResult == VK_SUBOPTIMAL_KHR)
    {
        // can render this frame, recreate next around
        requiredSwapchainRecreated = true;
    }
    // begin recording commands
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vkBeginCommandBuffer(res.commandBuffer, &beginInfo);

    // transition the color and depth images
    std::vector<VkImageMemoryBarrier2> layoutBarriers
    {
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = 0,
            .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
            .image = swapchainImages[imageIndex],
            .subresourceRange
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        },
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
            .srcAccessMask = 0,
            .dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            .dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .image = depthImage,
            .subresourceRange
            {
                .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        }
    };
    VkDependencyInfo depInfo = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = static_cast<uint32_t>(layoutBarriers.size()),
        .pImageMemoryBarriers = layoutBarriers.data(),
    };
    vkCmdPipelineBarrier2(res.commandBuffer, &depInfo);

    // setup the attachments (color and depth) and begin rendering (dynamic)
    VkRenderingAttachmentInfo colorAttachmentInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = swapchainImageViews[imageIndex],
        .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, // clear the image
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE, // keep data for presentation
        .clearValue = {.color = {0.01f, 0.01f, 0.01f, 1}}
    };

    VkRenderingAttachmentInfo depthAttachmentInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = depthImageView,
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, // clear the depth data
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE, // don't care after rendering
        .clearValue = {.depthStencil = {1.0f, 0}}
    };

    VkRenderingInfo renderingInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea
        {
            .offset{.x = 0, .y = 0},
            .extent{.width = swapchainWidth, .height = swapchainHeight},
        },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentInfo,
        .pDepthAttachment = &depthAttachmentInfo,
    };

    // begin dynamic rendering
    vkCmdBeginRendering(res.commandBuffer, &renderingInfo);
    {
        // set the viewport and scissor state
        VkViewport viewport
        {
            .x = 0, .y = 0,
            .width = static_cast<float>(swapchainWidth), .height = static_cast<float>(swapchainHeight),
        };
        vkCmdSetViewport(res.commandBuffer, 0, 1, &viewport);

        VkRect2D scissor
        {
            .offset{.x = 0, .y = 0},
            .extent{.width = swapchainWidth, .height = swapchainHeight},
        };
        vkCmdSetScissor(res.commandBuffer, 0, 1, &scissor);

        //draw out triangle
        vkCmdBindPipeline(res.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.handle);
        vkCmdDraw(res.commandBuffer, 3, 1, 0, 0);

        //////IMGUI
        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), res.commandBuffer, VK_NULL_HANDLE);
        ////IMGUI
    }


    //end dynamic rendering
    vkCmdEndRendering(res.commandBuffer);


    // transition the image from color attachment to presentation so we can show it
    VkImageMemoryBarrier2 presentLayoutBarrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_NONE, // cache is flushed, layout is transitioned
        .dstAccessMask = 0,
        .oldLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .image = swapchainImages[imageIndex],
        .subresourceRange
        {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    VkDependencyInfo presentDepInfo = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &presentLayoutBarrier
    };
    vkCmdPipelineBarrier2(res.commandBuffer, &presentDepInfo);
    vkEndCommandBuffer(res.commandBuffer);


    // ensure swapchain image is actually available  to start color output
    VkSemaphoreSubmitInfo imageAcquireWaitInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = imageAcquiredSemaphore,
        .stageMask =  VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
    };

    // signal that the image  can be presented
    std::vector<VkSemaphoreSubmitInfo> semaphoreSignals
    {
            { // render work completion signal
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .semaphore = renderCompleteSemaphores[imageIndex],
                .stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT
            },
            { // entire frame is completed (timeline)
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .semaphore = timelineSemaphore,
                .value = frameId, // we're signalling our current frame ID is complete
                .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT
            }
    };

    VkCommandBufferSubmitInfo commandBufferSubmitInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer = res.commandBuffer,
    };
    VkSubmitInfo2 submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .waitSemaphoreInfoCount = 1,
        .pWaitSemaphoreInfos = &imageAcquireWaitInfo,  // ensure the image is ready
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &commandBufferSubmitInfo,
        .signalSemaphoreInfoCount = static_cast<uint32_t>(semaphoreSignals.size()),
        .pSignalSemaphoreInfos = semaphoreSignals.data(),
    };


    vkQueueSubmit2(gfxQueue, 1, &submitInfo, VK_NULL_HANDLE);

    // present the image
    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &renderCompleteSemaphores[imageIndex], // render work completed semaphore
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = &imageIndex,
        .pResults = nullptr,
    };




    vkQueuePresentKHR(gfxQueue, &presentInfo);
}

void Application::shutdown()
{
    // wait in case resource are in use
    vkDeviceWaitIdle(device);

    // frame/sync object cleanup
    if (timelineSemaphore)
    {
        vkDestroySemaphore(device, timelineSemaphore, nullptr);
    }

    for (auto &res : frameResources)
    {
        vkDestroySemaphore(device, res.imageAcquiredSemaphore, nullptr);
        vkDestroyCommandPool(device, res.commandPool, nullptr); // destorys buffers implicitly
    }

    // destroy pipeline layout
    if (pipeline.layout)
    {
        vkDestroyPipelineLayout(device, pipeline.layout, nullptr);
    }
    // clean up shader
    if (vertShader)
    {
        vkDestroyShaderModule(device, vertShader, nullptr);
    }
    if (fragShader)
    {
        vkDestroyShaderModule(device, fragShader, nullptr);
    }

    // clean up swapchain
    destroySwapchain();

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

void Application::destroySwapchain()
{
    for (VkImageView swapchainImageView : swapchainImageViews)
    {
        vkDestroyImageView(device, swapchainImageView, nullptr);
    }
    swapchainImageViews.clear();

    // destroy render-complete semephores
    for (VkSemaphore renderCompleteSemaphore : renderCompleteSemaphores)
    {
        vkDestroySemaphore(device, renderCompleteSemaphore, nullptr);
    }
    renderCompleteSemaphores.clear();

    if (swapchain)
    {
        vkDestroySwapchainKHR(device, swapchain, nullptr);
        swapchain = VK_NULL_HANDLE;
    }
    // destroy the depth buffer along with the swapchain

    if (depthImageView)
    {
        vkDestroyImageView(device, depthImageView, nullptr);
        vmaDestroyImage(vmaAllocator, depthImage, depthImageAllocation);
        depthImageView = VK_NULL_HANDLE;
    }
}
