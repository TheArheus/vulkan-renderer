
#include "common.h"
#include "shader.cpp"
#include "mesh_loader.cpp"
#include "volk.c"

static main_state*
CreateSDLWindow(char* Name, uint32_t Width, uint32_t Height)
{
    SDL_Init(SDL_INIT_EVERYTHING);

    main_state* MainState = (main_state*)malloc(sizeof(main_state));

    MainState->Window       = SDL_CreateWindow(Name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, Width, Height, SDL_WINDOW_VULKAN|SDL_WINDOW_SHOWN);
    MainState->Renderer     = SDL_CreateRenderer(MainState->Window, -1, SDL_RENDERER_ACCELERATED);
    MainState->Texture      = SDL_CreateTexture(MainState->Renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, Width, Height);
    MainState->IsRunning    = true;
    MainState->WindowWidth  = Width;
    MainState->WindowHeight = Height;
    MainState->RtxSupported = false;

    return MainState;
}

static void
DestroyWindow(main_state* MainState)
{
    SDL_QuitSubSystem(SDL_INIT_EVERYTHING);
    SDL_DestroyTexture(MainState->Texture);
    SDL_DestroyRenderer(MainState->Renderer);
    SDL_DestroyWindow(MainState->Window);
    SDL_Quit();

    free(MainState);
}

static void
ProcessInput(main_state* MainState)
{
    SDL_Event Event;
    SDL_PollEvent(&Event);

    switch(Event.type)
    {
        case SDL_QUIT:
        {
            MainState->IsRunning = false;
        }break;

        case SDL_KEYDOWN:
        {
            if(Event.key.keysym.sym == SDLK_ESCAPE) MainState->IsRunning = false;
        }break;
    }
}

static void
Update(main_state* MainState)
{
}

static void
Render(main_state* MainState)
{
}

static VkInstance 
CreateInstance()
{
    VkApplicationInfo AppInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    AppInfo.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo InstanceCreateInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    InstanceCreateInfo.pApplicationInfo = &AppInfo;

#if _DEBUG
    const char* DebugLayers[] = 
    {
        //"VK_LAYER_LUNARG_standard_validation"
        "VK_LAYER_KHRONOS_validation"
    };

    InstanceCreateInfo.ppEnabledLayerNames = DebugLayers;
    InstanceCreateInfo.enabledLayerCount = ArraySize(DebugLayers);
#endif

    const char* Extensions[] =
    {
        VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef VK_USE_PLATFORM_WIN32_KHR
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
#if _DEBUG
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
#endif
    };

    InstanceCreateInfo.ppEnabledExtensionNames = Extensions;
    InstanceCreateInfo.enabledExtensionCount = ArraySize(Extensions);

    VkInstance Instance = 0;
    VK_CHECK(vkCreateInstance(&InstanceCreateInfo, 0, &Instance));

    return Instance;
}

VkBool32 DebugReportCallback(VkDebugReportFlagsEXT Flags, VkDebugReportObjectTypeEXT ObjectType, 
                             uint64_t Object, size_t Location, int32_t MessageCode, 
                             const char* pLayerPrefix, const char* pMessage, void* UserData)
{
    const char* Type = 
    ((Flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) 
    ? "Error" 
    : ((Flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) 
        ? "Performance Warning" : 
        ((Flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) ? "Warning" : "Other")));

    char Message[2048];
    sprintf_s(Message, ArraySize(Message), "Error type \"%s\": %s\n", Type, pMessage);
    printf("%s", Message);

#ifdef _WIN32
    OutputDebugStringA(Message);
#endif
    printf("\n");

    if(Flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
    {
        assert("Validation error encourted");
    }
    return VK_FALSE;
}

VkDebugReportCallbackEXT
RegisterDebugCallback(VkInstance Instance)
{
    VkDebugReportCallbackCreateInfoEXT DebugCallbackCreateInfo = {VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT};
    DebugCallbackCreateInfo.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT | 
                                    VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | 
                                    VK_DEBUG_REPORT_ERROR_BIT_EXT;
    DebugCallbackCreateInfo.pfnCallback = DebugReportCallback;

    VkDebugReportCallbackEXT DebugCallback = 0;
    vkCreateDebugReportCallbackEXT(Instance, &DebugCallbackCreateInfo, 0, &DebugCallback);

    return DebugCallback;
}

static VkSurfaceKHR 
CreateSurface(VkInstance Instance, SDL_Window* Window)
{
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    SDL_SysWMinfo WMInfo;
    SDL_VERSION(&WMInfo.version);
    SDL_GetWindowWMInfo(Window, &WMInfo);

    VkWin32SurfaceCreateInfoKHR CreateInfo = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR, 0, 0, WMInfo.info.win.hinstance, WMInfo.info.win.window };
    CreateInfo.hinstance = WMInfo.info.win.hinstance;
    CreateInfo.hwnd = WMInfo.info.win.window;

    VkSurfaceKHR Surface;
    VK_CHECK(vkCreateWin32SurfaceKHR(Instance, &CreateInfo, 0, &Surface));
    return Surface;
#endif
}

VkFormat
GetPhysicalDeviceSurfaceFormat(VkPhysicalDevice Device, VkSurfaceKHR Surface)
{
    uint32_t FormatSize = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(Device, Surface, &FormatSize, 0));
    
    std::vector<VkSurfaceFormatKHR> SurfaceFormats(FormatSize);
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(Device, Surface, &FormatSize, SurfaceFormats.data()));
    assert(FormatSize < ArraySize(SurfaceFormats));

    if(FormatSize == 1 && SurfaceFormats[0].format == VK_FORMAT_UNDEFINED)
        return VK_FORMAT_R8G8B8A8_UNORM;

    for(uint32_t FormatIndex = 0;
        FormatIndex < FormatSize;
        ++FormatIndex)
    {
        if(SurfaceFormats[FormatIndex].format == VK_FORMAT_R8G8B8A8_UNORM || SurfaceFormats[FormatIndex].format == VK_FORMAT_B8G8R8A8_UNORM)
            return SurfaceFormats[FormatIndex].format;
    }

    return SurfaceFormats[0].format;
}

uint32_t
GetGraphicsQueueFamily(VkPhysicalDevice PhysicalDevice)
{
    uint32_t QueuePropertiesCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueuePropertiesCount, 0);

    std::vector<VkQueueFamilyProperties> QueueProperties(QueuePropertiesCount);
    vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueuePropertiesCount, QueueProperties.data());
    //assert(QueuePropertiesCount < QueueProperties.size());

    for(uint32_t QueueIndex = 0;
        QueueIndex < QueuePropertiesCount;
        ++QueueIndex)
    {
        if(QueueProperties[QueueIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT) return QueueIndex;
    }

    return VK_QUEUE_FAMILY_IGNORED;
}

static VkDevice
CreateDevice(main_state* MainState, VkInstance Instance, VkPhysicalDevice PhysicalDevice, uint32_t* FamilyIndex)
{
    float QueuePriorities[] = {1.0f};

    VkDeviceQueueCreateInfo QueueCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    QueueCreateInfo.queueFamilyIndex = *FamilyIndex;
    QueueCreateInfo.queueCount = 1;
    QueueCreateInfo.pQueuePriorities = QueuePriorities;

    std::vector<const char*> Extensions = 
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
        VK_KHR_16BIT_STORAGE_EXTENSION_NAME,
        VK_KHR_8BIT_STORAGE_EXTENSION_NAME,
    };

    if(MainState->RtxSupported) Extensions.push_back(VK_NV_MESH_SHADER_EXTENSION_NAME);

    VkPhysicalDeviceFeatures2 Features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};

    VkPhysicalDevice8BitStorageFeatures StorageFeatures8Bit = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES};
    StorageFeatures8Bit.storageBuffer8BitAccess = true;
    StorageFeatures8Bit.uniformAndStorageBuffer8BitAccess = true;

    VkPhysicalDevice16BitStorageFeatures StorageFeatures16Bit = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES};
    StorageFeatures16Bit.storageBuffer16BitAccess = true;

    VkPhysicalDeviceMeshShaderFeaturesNV FeaturesMesh = {};
    if(MainState->RtxSupported)
    {
        FeaturesMesh = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV};
        FeaturesMesh.meshShader = true;
    }

    VkDeviceCreateInfo PhysicalDeviceCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    PhysicalDeviceCreateInfo.queueCreateInfoCount = 1;
    PhysicalDeviceCreateInfo.pQueueCreateInfos = &QueueCreateInfo;
    PhysicalDeviceCreateInfo.ppEnabledExtensionNames = Extensions.data();
    PhysicalDeviceCreateInfo.enabledExtensionCount = (uint32_t)Extensions.size();

    PhysicalDeviceCreateInfo.pNext = &Features;
    Features.pNext = &StorageFeatures8Bit;
    StorageFeatures8Bit.pNext = &StorageFeatures16Bit;

    if(MainState->RtxSupported)
    {
        StorageFeatures16Bit.pNext = &FeaturesMesh;
    }

    VkDevice Device = 0;
    VK_CHECK(vkCreateDevice(PhysicalDevice, &PhysicalDeviceCreateInfo, 0, &Device));

    return Device;
}

static VkSwapchainKHR
CreateSwapchain(VkPhysicalDevice PhysicalDevice, VkDevice Device, VkSurfaceKHR Surface, main_state* MainState, uint32_t* FamilyIndex, VkFormat Format)
{
    VkSurfaceCapabilitiesKHR SurfaceCaps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, Surface, &SurfaceCaps);

    VkCompositeAlphaFlagBitsKHR SurfaceComposite = 
        (SurfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) 
        ? VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR : 
        (SurfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR) 
        ? VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR : 
        (SurfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR) 
        ? VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR
        : VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    

    VkSwapchainCreateInfoKHR SwapchainCreateInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    SwapchainCreateInfo.surface               = Surface;
    SwapchainCreateInfo.minImageCount         = Max(2u, SurfaceCaps.minImageCount);
    SwapchainCreateInfo.imageFormat           = Format;
    SwapchainCreateInfo.imageColorSpace       = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    SwapchainCreateInfo.imageExtent.width     = MainState->WindowWidth;
    SwapchainCreateInfo.imageExtent.height    = MainState->WindowHeight;
    SwapchainCreateInfo.imageArrayLayers      = 1;
    SwapchainCreateInfo.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    SwapchainCreateInfo.queueFamilyIndexCount = 1;
    SwapchainCreateInfo.pQueueFamilyIndices   = FamilyIndex;
    SwapchainCreateInfo.preTransform          = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    SwapchainCreateInfo.compositeAlpha        = SurfaceComposite;
    SwapchainCreateInfo.presentMode           = VK_PRESENT_MODE_FIFO_KHR;

    VkSwapchainKHR Swapchain = 0;
    VK_CHECK(vkCreateSwapchainKHR(Device, &SwapchainCreateInfo, 0, &Swapchain));

    return Swapchain;
}

static VkSemaphore
CreateSemaphore(VkDevice Device)
{
    VkSemaphoreCreateInfo SemaphoreCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

    VkSemaphore Semaphore = 0;
    VK_CHECK(vkCreateSemaphore(Device, &SemaphoreCreateInfo, 0, &Semaphore));

    return Semaphore;
}

static VkCommandPool
CreateCommandPool(VkDevice Device, uint32_t FamilyIndex)
{
    VkCommandPoolCreateInfo CommandPoolCreateInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    CommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    CommandPoolCreateInfo.queueFamilyIndex = FamilyIndex;

    VkCommandPool CommandPool = 0;
    VK_CHECK(vkCreateCommandPool(Device, &CommandPoolCreateInfo, 0, &CommandPool));

    return CommandPool;
}

bool
SupportsPresentation(VkPhysicalDevice PhysicalDevice, uint32_t FamilyIndex)
{
    bool Result = true;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    Result = vkGetPhysicalDeviceWin32PresentationSupportKHR(PhysicalDevice, FamilyIndex);
#endif
    return Result;
}

static VkPhysicalDevice
PickPhysicalDevice(VkPhysicalDevice* PhysicalDevices, uint32_t PhysicalDeviceCount)
{
    VkPhysicalDevice DiscreteDevice = 0;
    VkPhysicalDevice FallbackDevice = 0;
    for(uint32_t DeviceIndex = 0;
        DeviceIndex < PhysicalDeviceCount;
        ++DeviceIndex)
    {
        VkPhysicalDeviceProperties Property = {};
        vkGetPhysicalDeviceProperties(PhysicalDevices[DeviceIndex], &Property);

        printf("GPU%d: %s\n", DeviceIndex, Property.deviceName);

        uint32_t FamilyIndex = GetGraphicsQueueFamily(PhysicalDevices[DeviceIndex]);
        if(FamilyIndex == VK_QUEUE_FAMILY_IGNORED) continue;
        bool Supports = SupportsPresentation(PhysicalDevices[DeviceIndex], FamilyIndex);
        if(!Supports) continue;

        if(!DiscreteDevice && Property.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            DiscreteDevice = PhysicalDevices[DeviceIndex];
        }

        if(!FallbackDevice)
        {
            FallbackDevice = PhysicalDevices[DeviceIndex];
        }
    }

#if 1
    VkPhysicalDevice Result = DiscreteDevice ? DiscreteDevice : FallbackDevice;

    return Result;
#else
    if(PhysicalDeviceCount > 0)
    {
        VkPhysicalDeviceProperties Property = {};
        vkGetPhysicalDeviceProperties(PhysicalDevices[0], &Property);
        printf("Picking device fallback GPU: %s\n", Property.deviceName);
        return PhysicalDevices[0];
    }

    printf("No physical devices available\n");
    return 0;
#endif
}

VkImageView
CreateImageView(VkDevice Device, VkImage Image, VkFormat Format)
{
    VkImageSubresourceRange Range = {};
    Range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    Range.levelCount = 1;
    Range.layerCount = 1;

    VkImageViewCreateInfo ImageViewCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    ImageViewCreateInfo.image            = Image;
    ImageViewCreateInfo.viewType         = VK_IMAGE_VIEW_TYPE_2D;
    ImageViewCreateInfo.format           = Format;
    ImageViewCreateInfo.subresourceRange = Range;

    VkImageView ImageView = 0;
    VK_CHECK(vkCreateImageView(Device, &ImageViewCreateInfo, 0, &ImageView));

    return ImageView;
}

VkRenderPass
CreateRenderPass(VkDevice Device, VkFormat Format)
{
    VkAttachmentDescription Attachments[1] = {};
    Attachments[0].format        = Format;
    Attachments[0].samples       = VK_SAMPLE_COUNT_1_BIT;
    Attachments[0].loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR;
    Attachments[0].storeOp       = VK_ATTACHMENT_STORE_OP_STORE;
    Attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    Attachments[0].stencilStoreOp= VK_ATTACHMENT_STORE_OP_DONT_CARE;
    Attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    Attachments[0].finalLayout   = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference ColorAttachments = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkSubpassDescription Subpass = {};
    Subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    Subpass.colorAttachmentCount = 1;
    Subpass.pColorAttachments    = &ColorAttachments;

    VkRenderPassCreateInfo RenderPassCreateInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    RenderPassCreateInfo.attachmentCount = ArraySize(Attachments);
    RenderPassCreateInfo.pAttachments    = Attachments;
    RenderPassCreateInfo.subpassCount    = 1;
    RenderPassCreateInfo.pSubpasses      = &Subpass;

    VkRenderPass RenderPass = 0;
    VK_CHECK(vkCreateRenderPass(Device, &RenderPassCreateInfo, 0, &RenderPass));

    return RenderPass;
}

VkFramebuffer
CreateFramebuffer(main_state* MainState, VkDevice Device, VkRenderPass RenderPass, VkImageView ImageView)
{
    VkFramebufferCreateInfo FramebufferCreateInfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    FramebufferCreateInfo.renderPass      = RenderPass;
    FramebufferCreateInfo.attachmentCount = 1;
    FramebufferCreateInfo.pAttachments    = &ImageView;
    FramebufferCreateInfo.width           = MainState->WindowWidth;
    FramebufferCreateInfo.height          = MainState->WindowHeight;
    FramebufferCreateInfo.layers          = 1;

    VkFramebuffer Framebuffer = 0;
    VK_CHECK(vkCreateFramebuffer(Device, &FramebufferCreateInfo, 0, &Framebuffer));

    return Framebuffer;
}

VkImageMemoryBarrier 
CreateImageBarrier(VkImage Image, 
                   VkAccessFlags SourceAccessMask, VkAccessFlags DestAccessMask,
                   VkImageLayout OldLayout, VkImageLayout NewLayout)
{
    VkImageMemoryBarrier ImageBarrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    ImageBarrier.srcAccessMask = SourceAccessMask;
    ImageBarrier.dstAccessMask = DestAccessMask;
    ImageBarrier.oldLayout = OldLayout;
    ImageBarrier.newLayout = NewLayout;
    ImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    ImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    ImageBarrier.image = Image;
    ImageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ImageBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    ImageBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    return ImageBarrier;
}

VkBufferMemoryBarrier
CreateMemoryBarrier(VkBuffer Buffer, VkAccessFlags scrAccessMask, VkAccessFlags dstAccessMask)
{
    VkBufferMemoryBarrier MemoryBarrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};

    MemoryBarrier.srcAccessMask = scrAccessMask;
    MemoryBarrier.dstAccessMask = dstAccessMask;
    MemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    MemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    MemoryBarrier.buffer = Buffer;
    MemoryBarrier.offset = 0;
    MemoryBarrier.size   = VK_WHOLE_SIZE;

    return MemoryBarrier;
}

struct buffer
{
    VkBuffer Buffer;
    VkDeviceMemory Memory; 
    void* Data;
    size_t Size;
};

uint32_t
SelectMemoryType(const VkPhysicalDeviceMemoryProperties& MemProperty, uint32_t MemoryBytes, VkMemoryPropertyFlags Flags)
{
    for(uint32_t PropIndex = 0;
        PropIndex < MemProperty.memoryTypeCount;
        ++PropIndex)
    {
        if((MemoryBytes & (1 << PropIndex)) != 0 && (MemProperty.memoryTypes[PropIndex].propertyFlags & Flags) == Flags)
        {
            return PropIndex;
        }
    }

    assert("No compatible memory type found");
    return ~0u;
}

void UploadBuffer(VkDevice Device, VkCommandPool CommandPool, 
                  VkCommandBuffer CommandBuffer, VkQueue Queue, 
                  const buffer& Buffer, const buffer& Scratch, 
                  const void* Data, size_t Size)
{
    assert(Scratch.Data);
    assert(Scratch.Size >= Size);
    memcpy(Scratch.Data, Data, Size);

    VK_CHECK(vkResetCommandPool(Device, CommandPool, 0));

    VkCommandBufferBeginInfo BeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(CommandBuffer, &BeginInfo));

    VkBufferCopy Region = {0, 0, (VkDeviceSize)Size};
    vkCmdCopyBuffer(CommandBuffer, Scratch.Buffer, Buffer.Buffer, 1, &Region);

    VkBufferMemoryBarrier CopyBarrier = CreateMemoryBarrier(Buffer.Buffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT);
    vkCmdPipelineBarrier(CommandBuffer, 
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
                         VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, &CopyBarrier, 0, 0);

    VK_CHECK(vkEndCommandBuffer(CommandBuffer));

    VkSubmitInfo SubmitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers = &CommandBuffer;

    VK_CHECK(vkQueueSubmit(Queue, 1, &SubmitInfo, VK_NULL_HANDLE));

    VK_CHECK(vkDeviceWaitIdle(Device));
}

void
CreateBuffer(buffer& Result, VkDevice Device, VkPhysicalDeviceMemoryProperties* MemProperty, size_t Size, VkBufferUsageFlags Flags, VkMemoryPropertyFlags MemoryFlags)
{
    VkBufferCreateInfo BufferCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    BufferCreateInfo.size = Size;
    BufferCreateInfo.usage = Flags;

    VkBuffer Buffer = 0;
    VK_CHECK(vkCreateBuffer(Device, &BufferCreateInfo, 0, &Buffer));

    VkMemoryRequirements MemoryR;
    vkGetBufferMemoryRequirements(Device, Buffer, &MemoryR);

    uint32_t MemoryType = SelectMemoryType(*MemProperty, MemoryR.memoryTypeBits, MemoryFlags);
    assert(MemoryType != ~0u);

    VkMemoryAllocateInfo AllocateInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    AllocateInfo.allocationSize = MemoryR.size;
    AllocateInfo.memoryTypeIndex = MemoryType;

    VkDeviceMemory Memory = 0;
    VK_CHECK(vkAllocateMemory(Device, &AllocateInfo, 0, &Memory));

    VK_CHECK(vkBindBufferMemory(Device, Buffer, Memory, 0));

    void* Data = 0;
    if(MemoryFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        VK_CHECK(vkMapMemory(Device, Memory, 0, Size, 0, &Data));
    }

    Result.Buffer = Buffer;
    Result.Memory = Memory;
    Result.Data = Data;
    Result.Size = Size;
}

void 
DestroyBuffer(const buffer& Buffer, VkDevice Device)
{
    vkFreeMemory(Device, Buffer.Memory, 0);
    vkDestroyBuffer(Device, Buffer.Buffer, 0);
}

VkQueryPool
CreateQueryPool(VkDevice Device, uint32_t Size)
{
    VkQueryPoolCreateInfo QueryPoolCreateInfo = {VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
    QueryPoolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
    QueryPoolCreateInfo.queryCount = Size;

    VkQueryPool QueryPool = 0;
    vkCreateQueryPool(Device, &QueryPoolCreateInfo, 0, &QueryPool);

    return QueryPool;
}

int main(int argc, char** argv)
{
    main_state* MainState = CreateSDLWindow("", 1280, 720);

    VK_CHECK(volkInitialize());

    VkInstance Instance = CreateInstance();
    assert(Instance);

    volkLoadInstance(Instance);

    bool PushDescriptorsSupported = false;

    //VkDebugReportCallbackEXT DebugCallback = RegisterDebugCallback(Instance);

    VkPhysicalDevice PhysicalDevices[16];
    uint32_t PhysicalDeviceCount = ArraySize(PhysicalDevices);
    VK_CHECK(vkEnumeratePhysicalDevices(Instance, &PhysicalDeviceCount, PhysicalDevices));

    VkPhysicalDevice PhysicalDevice = PickPhysicalDevice(PhysicalDevices, PhysicalDeviceCount);
    assert(PhysicalDevice);

    uint32_t ExtensionCount = 0;
    VK_CHECK(vkEnumerateDeviceExtensionProperties(PhysicalDevice, 0, &ExtensionCount, 0));

    std::vector<VkExtensionProperties> PhysicalDeviceProperties(ExtensionCount);
    VK_CHECK(vkEnumerateDeviceExtensionProperties(PhysicalDevice, 0, &ExtensionCount, PhysicalDeviceProperties.data()));

    for(auto& Ext : PhysicalDeviceProperties)
    {
        if(strcmp(Ext.extensionName, "VK_NV_mesh_shader") == 0) MainState->RtxSupported = true;
        if(strcmp(Ext.extensionName, "VK_KHR_push_descriptor") == 0) PushDescriptorsSupported = true;
    }

    VkPhysicalDeviceProperties Props = {};
    vkGetPhysicalDeviceProperties(PhysicalDevice, &Props);
    assert(Props.limits.timestampComputeAndGraphics);

    uint32_t FamilyIndex = GetGraphicsQueueFamily(PhysicalDevice);
    assert(FamilyIndex != VK_QUEUE_FAMILY_IGNORED);

    VkDevice Device = CreateDevice(MainState, Instance, PhysicalDevice, &FamilyIndex);
    assert(Device);

    shader MeshletMS = {};
    if(MainState->RtxSupported)
    {
        assert(LoadShader(MeshletMS, Device, "..\\shaders\\meshlet.mesh.spv"));
    }

    shader MeshVert = {};
    assert(LoadShader(MeshVert, Device, "..\\shaders\\mesh.vert.spv"));

    shader MeshFrag = {};
    assert(LoadShader(MeshFrag, Device, "..\\shaders\\mesh.frag.spv"));

    VkSurfaceKHR Surface = CreateSurface(Instance, MainState->Window);
    assert(Surface);

    VkBool32 PresentSupported = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, FamilyIndex, Surface, &PresentSupported));
    assert(PresentSupported);

    VkFormat WindowFormat = GetPhysicalDeviceSurfaceFormat(PhysicalDevice, Surface);
    assert(WindowFormat);

    VkSwapchainKHR Swapchain = CreateSwapchain(PhysicalDevice, Device, Surface, MainState, &FamilyIndex, WindowFormat);
    assert(Swapchain);

    VkSemaphore AcquireSemaphore = CreateSemaphore(Device);
    assert(AcquireSemaphore);
    VkSemaphore ReleaseSemaphore = CreateSemaphore(Device);
    assert(ReleaseSemaphore);

    VkQueue Queue = 0;
    vkGetDeviceQueue(Device, FamilyIndex, 0, &Queue);

    uint32_t SwapchainImagesCount = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(Device, Swapchain, &SwapchainImagesCount, 0));

    VkImage SwapchainImages[16] = {};
    VK_CHECK(vkGetSwapchainImagesKHR(Device, Swapchain, &SwapchainImagesCount, SwapchainImages));

    VkRenderPass RenderPass = CreateRenderPass(Device, WindowFormat);
    assert(RenderPass);

    VkPipelineLayout PipelineLayout = CreatePipelineLayout(Device, MeshVert, MeshFrag);
    assert(PipelineLayout);

    VkDescriptorUpdateTemplate MeshUpdateTemplate = CreateUpdateTemplate(Device, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, MeshVert, MeshFrag);
    assert(MeshUpdateTemplate);

    VkPipelineLayout PipelineLayoutRTX = 0;
    VkDescriptorUpdateTemplate MeshUpdateTemplateRTX = 0;
    if(MainState->RtxSupported)
    {
        PipelineLayoutRTX = CreatePipelineLayout(Device, MeshletMS, MeshFrag);
        assert(PipelineLayoutRTX);

        MeshUpdateTemplateRTX = CreateUpdateTemplate(Device, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayoutRTX, MeshletMS, MeshFrag);
        assert(MeshUpdateTemplateRTX);
    }

    VkPipelineCache PipelineCache = 0;
    VkPipeline MeshPipeline = CreateGraphicsPipeline(Device, RenderPass, PipelineCache, MeshVert, MeshFrag, PipelineLayout);
    assert(MeshPipeline);

    VkPipeline MeshPipelineRTX = 0;
    if(MainState->RtxSupported)
    {
        MeshPipelineRTX = CreateGraphicsPipeline(Device, RenderPass, PipelineCache, MeshletMS, MeshFrag, PipelineLayoutRTX);
        assert(MeshPipelineRTX);
    }

    std::vector<VkImageView> SwapchainImageViews(SwapchainImagesCount);
    for(uint32_t ImageViewIndex = 0;
        ImageViewIndex < SwapchainImagesCount;
        ++ImageViewIndex)
    {
        SwapchainImageViews[ImageViewIndex] = CreateImageView(Device, SwapchainImages[ImageViewIndex], WindowFormat);
        assert(SwapchainImageViews[ImageViewIndex]);
    }

    std::vector<VkFramebuffer> Framebuffers(SwapchainImagesCount);
    for(uint32_t FramebufferIndex = 0;
        FramebufferIndex < SwapchainImagesCount;
        ++FramebufferIndex)
    {
        Framebuffers[FramebufferIndex] = CreateFramebuffer(MainState, Device, RenderPass, SwapchainImageViews[FramebufferIndex]);
        assert(Framebuffers[FramebufferIndex]);
    }

    VkCommandPool CommandPool = CreateCommandPool(Device, FamilyIndex);
    assert(CommandPool);

    VkCommandBufferAllocateInfo CommandBufferAllocateInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    CommandBufferAllocateInfo.commandPool = CommandPool;
    CommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    CommandBufferAllocateInfo.commandBufferCount = 1;

    VkCommandBuffer CommandBuffer = 0;
    vkAllocateCommandBuffers(Device, &CommandBufferAllocateInfo, &CommandBuffer);

    VkPhysicalDeviceMemoryProperties MemProperty;
    vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &MemProperty);

    VkQueryPool QueryPool = CreateQueryPool(Device, 128);
    assert(QueryPool);

    LoadMesh("../data/kitten.obj");

    buffer ScratchBuffer = {};
    uint32_t ScratchBufferSize = 128 * 1024 * 1024;
    CreateBuffer(ScratchBuffer, Device, &MemProperty, ScratchBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    buffer VertexBuffer = {};
    uint32_t VertexBufferSize = 128 * 1024 * 1024;
    CreateBuffer(VertexBuffer, Device, &MemProperty, VertexBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    buffer IndexBuffer  = {};
    uint32_t IndexBufferSize = 128 * 1024 * 1024;
    CreateBuffer(IndexBuffer,  Device, &MemProperty, IndexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    UploadBuffer(Device, CommandPool, CommandBuffer, Queue, VertexBuffer, ScratchBuffer, Meshes[0].Mesh.data(), Meshes[0].Mesh.size() * sizeof(vertex));
    UploadBuffer(Device, CommandPool, CommandBuffer, Queue, IndexBuffer, ScratchBuffer, Meshes[0].ConvertedVertexIndices.data(), Meshes[0].ConvertedVertexIndices.size() * sizeof(uint32_t));

    buffer MeshletBuffer  = {};
    if(MainState->RtxSupported)
    {
        uint32_t MeshletBufferSize = 128 * 1024 * 1024;
        CreateBuffer(MeshletBuffer,  Device, &MemProperty, MeshletBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        UploadBuffer(Device, CommandPool, CommandBuffer, Queue, MeshletBuffer, ScratchBuffer, Meshes[0].Meshlets.data(), Meshes[0].Meshlets.size() * sizeof(meshlet_t));
    }

    while(MainState->IsRunning)
    {
        uint64_t FrameBegin = SDL_GetPerformanceCounter();
        ProcessInput(MainState);
        Update(MainState);
        Render(MainState);

        uint32_t ImageIndex = 0;
        vkAcquireNextImageKHR(Device, Swapchain, ~0ull, AcquireSemaphore, VK_NULL_HANDLE, &ImageIndex);

        VK_CHECK(vkResetCommandPool(Device, CommandPool, 0));

        VkCommandBufferBeginInfo BeginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_CHECK(vkBeginCommandBuffer(CommandBuffer, &BeginInfo));

        vkCmdResetQueryPool(CommandBuffer, QueryPool, 0, 128);
        vkCmdWriteTimestamp(CommandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, QueryPool, 0);

        VkImageMemoryBarrier RenderBeginMemoryBarrier = CreateImageBarrier(SwapchainImages[ImageIndex], 0, 
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 
                VK_IMAGE_LAYOUT_UNDEFINED, 
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        vkCmdPipelineBarrier(CommandBuffer, 
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
                             VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &RenderBeginMemoryBarrier);
        

        VkClearColorValue Color = {0, 0, 0, 1};
        VkClearValue ClearColor = {Color};

        VkRenderPassBeginInfo RenderPassBeginInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        RenderPassBeginInfo.renderPass = RenderPass;
        RenderPassBeginInfo.framebuffer = Framebuffers[ImageIndex];
        RenderPassBeginInfo.renderArea.extent.width  = MainState->WindowWidth;
        RenderPassBeginInfo.renderArea.extent.height = MainState->WindowHeight;
        RenderPassBeginInfo.clearValueCount = 1;
        RenderPassBeginInfo.pClearValues = &ClearColor;

        vkCmdBeginRenderPass(CommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport Viewport = {0, (float)MainState->WindowHeight, (float)MainState->WindowWidth, -(float)MainState->WindowHeight, 0, 1};
        VkRect2D Scissor = {{0, 0}, {MainState->WindowWidth, MainState->WindowHeight}};

        vkCmdSetViewport(CommandBuffer, 0, 1, &Viewport);
        vkCmdSetScissor(CommandBuffer, 0, 1, &Scissor);

        VkDescriptorBufferInfo BufferInfo = {};
        BufferInfo.buffer = VertexBuffer.Buffer;
        BufferInfo.offset = 0;
        BufferInfo.range = VertexBuffer.Size;

#if 0
        if(MainState->RtxSupported)
        {
            vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MeshPipelineRTX);

            descriptor_info Descriptors[] = {VertexBuffer.Buffer, MeshletBuffer.Buffer};
            vkCmdPushDescriptorSetWithTemplateKHR(CommandBuffer, MeshUpdateTemplateRTX, PipelineLayoutRTX, 0, Descriptors);

            vkCmdDrawMeshTasksNV(CommandBuffer, (uint32_t)Meshes[0].Meshlets.size(), 0);
        }
        else
        {
            vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MeshPipeline);

            descriptor_info Descriptors[] = {VertexBuffer.Buffer};
            vkCmdPushDescriptorSetWithTemplateKHR(CommandBuffer, MeshUpdateTemplate, PipelineLayout, 0, Descriptors);

            vkCmdBindIndexBuffer(CommandBuffer, IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(CommandBuffer, (uint32_t)Meshes[0].ConvertedVertexIndices.size(), 1, 0, 0, 0);
        }
#else
        if(MainState->RtxSupported)
        {
            vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MeshPipelineRTX);

            VkDescriptorBufferInfo MeshletBufferInfo = {};
            MeshletBufferInfo.buffer = MeshletBuffer.Buffer;
            MeshletBufferInfo.offset = 0;
            MeshletBufferInfo.range = MeshletBuffer.Size;

            VkWriteDescriptorSet Descriptors[2] = {};
            Descriptors[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            Descriptors[0].dstBinding = 0;
            Descriptors[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            Descriptors[0].descriptorCount = 1;
            Descriptors[0].pBufferInfo = &BufferInfo;

            Descriptors[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            Descriptors[1].dstBinding = 1;
            Descriptors[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            Descriptors[1].descriptorCount = 1;
            Descriptors[1].pBufferInfo = &MeshletBufferInfo;
            vkCmdPushDescriptorSetKHR(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayoutRTX, 0, ArraySize(Descriptors), Descriptors);

            vkCmdDrawMeshTasksNV(CommandBuffer, (uint32_t)Meshes[0].Meshlets.size(), 0);
        }
        else
        {
            vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MeshPipeline);

            VkWriteDescriptorSet Descriptors[1] = {};
            Descriptors[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            Descriptors[0].dstBinding = 0;
            Descriptors[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            Descriptors[0].descriptorCount = 1;
            Descriptors[0].pBufferInfo = &BufferInfo;
            vkCmdPushDescriptorSetKHR(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 0, ArraySize(Descriptors), Descriptors);

            vkCmdBindIndexBuffer(CommandBuffer, IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(CommandBuffer, (uint32_t)Meshes[0].ConvertedVertexIndices.size(), 1, 0, 0, 0);
        }
#endif

        vkCmdEndRenderPass(CommandBuffer);

        vkCmdWriteTimestamp(CommandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, QueryPool, 1);

        VkImageMemoryBarrier RenderEndMemoryBarrier = CreateImageBarrier(SwapchainImages[ImageIndex], 
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 
                0, 
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        vkCmdPipelineBarrier(CommandBuffer, 
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 
                             VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &RenderEndMemoryBarrier);

        VK_CHECK(vkEndCommandBuffer(CommandBuffer));

        VkPipelineStageFlags SubmitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo SubmitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
        SubmitInfo.waitSemaphoreCount = 1;
        SubmitInfo.pWaitSemaphores = &AcquireSemaphore;
        SubmitInfo.pWaitDstStageMask = &SubmitStageMask;
        SubmitInfo.commandBufferCount = 1;
        SubmitInfo.pCommandBuffers = &CommandBuffer;
        SubmitInfo.signalSemaphoreCount = 1;
        SubmitInfo.pSignalSemaphores = &ReleaseSemaphore;

        VK_CHECK(vkQueueSubmit(Queue, 1, &SubmitInfo, VK_NULL_HANDLE));

        VkPresentInfoKHR PresentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        PresentInfo.swapchainCount = 1;
        PresentInfo.pSwapchains = &Swapchain;
        PresentInfo.pImageIndices = &ImageIndex;
        PresentInfo.waitSemaphoreCount = 1;
        PresentInfo.pWaitSemaphores = &ReleaseSemaphore;

        VK_CHECK(vkQueuePresentKHR(Queue, &PresentInfo));

        VK_CHECK(vkDeviceWaitIdle(Device));

        uint64_t QueryResults[2];
        vkGetQueryPoolResults(Device, QueryPool, 0, ArraySize(QueryResults), sizeof(QueryResults), QueryResults, sizeof(QueryResults[0]), VK_QUERY_RESULT_64_BIT);

        double FrameBeginGPU = (double)(QueryResults[0]) * Props.limits.timestampPeriod * 1e-6;
        double FrameEndGPU   = (double)(QueryResults[1]) * Props.limits.timestampPeriod * 1e-6;

        uint64_t FrameEnd = SDL_GetPerformanceCounter();
        double cpu_ms = (double)(FrameEnd - FrameBegin) * 1000.0f / (double)SDL_GetPerformanceFrequency();
        double gpu_ms = (FrameEndGPU - FrameBeginGPU);

        char Title[256];
        sprintf(Title, "cpu: %.1f ms; gpu: %.1f ms\n", cpu_ms, gpu_ms);
        SDL_SetWindowTitle(MainState->Window, Title);
    }

    VK_CHECK(vkDeviceWaitIdle(Device));


    DestroyBuffer(ScratchBuffer, Device);
    DestroyBuffer(IndexBuffer, Device);
    DestroyBuffer(VertexBuffer, Device);
    if(MainState->RtxSupported)
    {
        DestroyBuffer(MeshletBuffer, Device);
    }

    vkDestroyQueryPool(Device, QueryPool, 0);

    vkDestroyCommandPool(Device, CommandPool, 0);

    for(uint32_t FrameIndex = 0;
        FrameIndex < SwapchainImagesCount;
        ++FrameIndex)
    {
        vkDestroyFramebuffer(Device, Framebuffers[FrameIndex], 0);
    }

    for(uint32_t FrameIndex = 0;
        FrameIndex < SwapchainImagesCount;
        ++FrameIndex)
    {
        vkDestroyImageView(Device, SwapchainImageViews[FrameIndex], 0);
    }

    vkDestroyPipeline(Device, MeshPipeline, 0);
    vkDestroyPipelineLayout(Device, PipelineLayout, 0);
    vkDestroyDescriptorUpdateTemplate(Device, MeshUpdateTemplate, 0);

    if(MainState->RtxSupported)
    {
        vkDestroyPipeline(Device, MeshPipelineRTX, 0);
        vkDestroyPipelineLayout(Device, PipelineLayoutRTX, 0);
        vkDestroyDescriptorUpdateTemplate(Device, MeshUpdateTemplateRTX, 0);

    }

    vkDestroyShaderModule(Device, MeshFrag.Module, 0);
    vkDestroyShaderModule(Device, MeshVert.Module, 0);

    vkDestroyRenderPass(Device, RenderPass, 0);
    
    vkDestroySemaphore(Device, AcquireSemaphore, 0);
    vkDestroySemaphore(Device, ReleaseSemaphore, 0);

    vkDestroySwapchainKHR(Device, Swapchain, 0);
    vkDestroySurfaceKHR(Instance, Surface, 0);

    vkDestroyDevice(Device, 0);

    //vkDestroyDebugReportCallbackEXT(Instance, DebugCallback, 0);

    vkDestroyInstance(Instance, 0);

    DestroyWindow(MainState);
    return 0;
}

