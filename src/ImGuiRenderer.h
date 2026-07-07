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
    void newFrame();
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

    bool show_demo_window;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

};


#endif //RIOW_VK_IMGUIRENDERER_H