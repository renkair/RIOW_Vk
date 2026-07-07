//
// Created by Renkai on 07/07/2026.
//

#ifndef RIOW_VK_IMGUIRENDERER_H
#define RIOW_VK_IMGUIRENDERER_H

#include <volk.h>

#include <SDL3/SDL_video.h>

#include "imgui.h"

class ImGuiRenderer
{
public:
    ImGuiRenderer(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, uint32_t gfxFamilyIdx, VkQueue gfxQueue, SDL_Window* window);
    ~ImGuiRenderer();
    void render();
private:
    void init();
    void shutdown();
    void createDescriptorPools();
    //context
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    uint32_t gfxFamilyIdx;
    VkQueue gfxQueue;

    SDL_Window* window = nullptr;


    VkDescriptorPool descriptorPool;

};


#endif //RIOW_VK_IMGUIRENDERER_H