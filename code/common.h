#define _CRT_SECURE_NO_WARNINGS

#define VK_USE_PLATFORM_WIN32_KHR
#include <SDL2/SDL_config.h>

#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <vector>
#include <array>
#include <algorithm>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

//#define VK_NO_PROTOTYPES
//#include <vulkan/vulkan.h>
#include "volk.h"
#include <vulkan/vulkan_core.h>

struct main_state;

#include "mesh_loader.h"
#include "shader.h"

#define VK_CHECK(call)                  \
    do                                  \
    {                                   \
        VkResult Result_ = call;        \
        assert(Result_ == VK_SUCCESS);  \
    } while(0)

#define Min(a, b) ((a > b) ? a : b)
#define Max(a, b) ((a < b) ? a : b)
#define ArraySize(arr) (sizeof(arr) / sizeof(arr[0]))

struct swapchain_t
{
    VkSwapchainKHR Swapchain;

    std::vector<VkImage> Images;
    std::vector<VkImageView> ImageViews;
    std::vector<VkFramebuffer> Framebuffers;
};

struct main_state
{
    SDL_Window* Window;
    SDL_Renderer* Renderer;
    SDL_Texture* Texture;

    uint32_t WindowWidth;
    uint32_t WindowHeight;

    uint32_t IsRunning;
    uint32_t RtxSupported;

    swapchain_t Swapchain;
};

