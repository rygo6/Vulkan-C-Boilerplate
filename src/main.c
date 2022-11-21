#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

typedef struct FabricState {
    int screenWidth;
    int screenHeight;

    bool enableValidationLayers;

    GLFWwindow *pWindow;

    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;

    VkPhysicalDevice physicalDevice;
    VkDevice device;

    VkQueue queue;
    uint32_t graphicsQueueFamilyIndex;

    VkSwapchainKHR swapChain;
    uint32_t swapChainImageCount;
    VkImage *pSwapChainImages;
    VkImageView *pSwapChainImageViews;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    VkFramebuffer *pSwapChainFramebuffers;

    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;

} FabricAppState;

const uint32_t validationLayersCount = 1;
const char* validationLayers[] = {
        "VK_LAYER_KHRONOS_validation"
};

const uint32_t requiredExtensionCount = 1;
const char* requiredExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

void initWindow(FabricAppState* pState) {
    printf( "%s - initializing fabric window!\n", __FUNCTION__ );

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    pState->pWindow = glfwCreateWindow(pState->screenWidth, pState->screenHeight, "Fabric", NULL, NULL);
    if (pState->pWindow == NULL) {
        printf( "%s - unable to initialize GLFW Window!\n", __FUNCTION__ );
    }
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != NULL) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != NULL) {
        func(instance, debugMessenger, pAllocator);
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        printf( "%s - validation layer: %s\n", __FUNCTION__, pCallbackData->pMessage);
    }

    return VK_FALSE;
}

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo) {
    pCreateInfo->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    pCreateInfo->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    pCreateInfo->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    pCreateInfo->pfnUserCallback = debugCallback;
}

bool checkValidationLayerSupport() {
    uint32_t availableLayerCount;
    vkEnumerateInstanceLayerProperties(&availableLayerCount, NULL);

    VkLayerProperties availableLayers[availableLayerCount];
    vkEnumerateInstanceLayerProperties(&availableLayerCount,availableLayers);

    for (int i = 0; i < validationLayersCount; ++i) {
        bool layerFound = false;

        for (int j = 0; j < availableLayerCount; ++j) {
            if (strcmp(validationLayers[i], availableLayers[j].layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

void getRequiredExtensions(FabricAppState *pState, uint32_t* extensionCount, const char** pExtensions) {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    if (pExtensions == NULL){
        *extensionCount = glfwExtensionCount + (pState->enableValidationLayers ? 1 : 0);
        return;
    }

    for (int i = 0; i < glfwExtensionCount; ++i){
        pExtensions[i] = glfwExtensions[i];
    }

    if (pState->enableValidationLayers) {
        pExtensions[glfwExtensionCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    }
}

void createInstance(FabricAppState* pState) {
    if (pState->enableValidationLayers && !checkValidationLayerSupport()) {
        printf( "%s - validation layers requested, but not available!\n", __FUNCTION__ );
    }

    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Fabric Vulkan",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = NULL,
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0,
    };

    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
    };

    uint32_t extensionCount = 0;
    getRequiredExtensions(pState, &extensionCount, NULL);

    const char* extensions[extensionCount];
    getRequiredExtensions(pState, &extensionCount, extensions);

    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames = extensions;
    createInfo.enabledLayerCount = 0;

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
    if (pState->enableValidationLayers) {
        createInfo.enabledLayerCount = validationLayersCount;
        createInfo.ppEnabledLayerNames = validationLayers;

        populateDebugMessengerCreateInfo(&debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = NULL;
    }

    if (vkCreateInstance(&createInfo, NULL, &pState->instance) != VK_SUCCESS) {
        printf( "%s - unable to initialize Vulkan!\n", __FUNCTION__ );
    }
}

void setupDebugMessenger(FabricAppState* pState) {
    if (!pState->enableValidationLayers)
        return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(&createInfo);

    if (CreateDebugUtilsMessengerEXT(pState->instance, &createInfo, NULL, &pState->debugMessenger) != VK_SUCCESS) {
        printf( "%s - failed to set up debug messenger!\n", __FUNCTION__ );
    }
}

bool createSurface(FabricAppState* pState) {
    if (glfwCreateWindowSurface(pState->instance, pState->pWindow, NULL, &pState->surface) != VK_SUCCESS) {
        printf( "%s - failed to create window surface!\n", __FUNCTION__ );
        return false;
    }

    return true;
}

bool findQueueFamilies(FabricAppState* pState) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(pState->physicalDevice, &queueFamilyCount, NULL);
    VkQueueFamilyProperties queueFamilies[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(pState->physicalDevice, &queueFamilyCount, (VkQueueFamilyProperties *) &queueFamilies);

    if ( queueFamilyCount == 0 )
    {
        printf( "%s - Failed to get queue properties.\n", __FUNCTION__ );
    }

    // Taking a cue from SteamVR Vulkan example and just assuming queue that supports both graphics and present is the only one we want. Don't entirely know if that's right.
    for (int i = 0; i < queueFamilyCount; ++i) {
        VkBool32 graphicsSupport = queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(pState->physicalDevice, i, pState->surface, &presentSupport);

        if (graphicsSupport && presentSupport) {
            pState->graphicsQueueFamilyIndex = i;
            return true;
        }
    }

    printf( "%s - Failed to find a queue that supports both graphics and present!\n", __FUNCTION__ );
    return false;
}

void pickPhysicalDevice(FabricAppState* pState) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(pState->instance, &deviceCount, NULL);

    if (deviceCount == 0) {
        printf( "%s - failed to find GPUs with Vulkan support!\n", __FUNCTION__ );
    }

    VkPhysicalDevice devices[deviceCount];
    vkEnumeratePhysicalDevices(pState->instance, &deviceCount, devices);

    // Todo Implement Query OpenVR for the physical device to use
    // If no OVR fallback to first one. OVR Vulkan used this logic, its much simpler than vulkan example, is it correct? Seemed to be on my 6950xt
    pState->physicalDevice = devices[0];
}

bool createLogicalDevice(FabricAppState* pState) {
    if (!findQueueFamilies(pState)){
        return false;
    }

    const uint32_t queueFamilyCount = 1;
    VkDeviceQueueCreateInfo queueCreateInfos[queueFamilyCount];
    uint32_t uniqueQueueFamilies[] = {pState->graphicsQueueFamilyIndex };

    float queuePriority = 1.0f;
    for (int i = 0; i < queueFamilyCount; ++i) {
        VkDeviceQueueCreateInfo queueCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = uniqueQueueFamilies[i],
                .queueCount = 1,
                .pQueuePriorities = &queuePriority
        };
        queueCreateInfos[i] = queueCreateInfo;
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};

    VkDeviceCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount = queueFamilyCount,
            .pQueueCreateInfos = queueCreateInfos,
            .pEnabledFeatures = &deviceFeatures,
            .enabledExtensionCount = requiredExtensionCount,
            .ppEnabledExtensionNames = requiredExtensions,
    };

    if (pState->enableValidationLayers) {
        createInfo.enabledLayerCount = validationLayersCount;
        createInfo.ppEnabledLayerNames = validationLayers;
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(pState->physicalDevice, &createInfo, NULL, &pState->device) != VK_SUCCESS) {
        printf( "%s - failed to create logical device!\n", __FUNCTION__ );
        return false;
    }

    vkGetDeviceQueue(pState->device, pState->graphicsQueueFamilyIndex, 0, &pState->queue);

    return true;
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(VkSurfaceFormatKHR *availableFormats, uint32_t formatCount) {
    // Favor sRGB if it's available
    for (int i = 0; i < formatCount; ++i) {
        if (availableFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormats[i];
        }
    }

    // Default to the first one if no sRGB
    return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(const VkPresentModeKHR *availablePresentModes, uint32_t presentModeCount) {
    // This logic taken from OVR Vulkan Example
    // VK_PRESENT_MODE_FIFO_KHR - equivalent of eglSwapInterval(1).  The presentation engine waits for the next vertical blanking period to update
    // the current image. Tearing cannot be observed. This mode must be supported by all implementations.
    VkPresentModeKHR swapChainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (int i = 0; i < presentModeCount; ++i) {
        // Order of preference for no vsync:
        // 1. VK_PRESENT_MODE_IMMEDIATE_KHR - The presentation engine does not wait for a vertical blanking period to update the current image,
        //                                    meaning this mode may result in visible tearing
        // 2. VK_PRESENT_MODE_MAILBOX_KHR - The presentation engine waits for the next vertical blanking period to update the current image. Tearing cannot be observed.
        //                                  An internal single-entry queue is used to hold pending presentation requests.
        // 3. VK_PRESENT_MODE_FIFO_RELAXED_KHR - equivalent of eglSwapInterval(-1).
        if ( availablePresentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR )
        {
            // The presentation engine does not wait for a vertical blanking period to update the
            // current image, meaning this mode may result in visible tearing
            swapChainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            break;
        }
        else if ( availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR )
        {
            swapChainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        }
        else if ( ( swapChainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR ) &&
                  ( availablePresentModes[i] == VK_PRESENT_MODE_FIFO_RELAXED_KHR ) )
        {
            // VK_PRESENT_MODE_FIFO_RELAXED_KHR - equivalent of eglSwapInterval(-1)
            swapChainPresentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
        }
    }

    return swapChainPresentMode;
}

VkExtent2D chooseSwapExtent(FabricAppState* pState, const VkSurfaceCapabilitiesKHR capabilities) {
    // Logic from OVR Vulkan sample. Logic little different from vulkan tutorial
    VkExtent2D extents;
    if ( capabilities.currentExtent.width == -1 )
    {
        // If the surface size is undefined, the size is set to the size of the images requested.
        extents.width = pState->screenWidth;
        extents.height = pState->screenHeight;
    }
    else
    {
        // If the surface size is defined, the swap chain size must match
        extents = capabilities.currentExtent;
    }

    return extents;
}

void createSwapChain(FabricAppState* pState) {
    // Logic from OVR Vulkan example
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pState->physicalDevice, pState->surface, &capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(pState->physicalDevice, pState->surface, &formatCount, NULL);
    VkSurfaceFormatKHR formats[formatCount];
    vkGetPhysicalDeviceSurfaceFormatsKHR(pState->physicalDevice, pState->surface, &formatCount, (VkSurfaceFormatKHR *) &formats);

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(pState->physicalDevice, pState->surface, &presentModeCount, NULL);
    VkPresentModeKHR presentModes[presentModeCount];
    vkGetPhysicalDeviceSurfacePresentModesKHR(pState->physicalDevice, pState->surface, &presentModeCount,(VkPresentModeKHR *) &presentModes);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(formats, formatCount);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(presentModes, presentModeCount);
    VkExtent2D extent = chooseSwapExtent(pState, capabilities);

    // Have a swap queue depth of at least three frames
    pState->swapChainImageCount = capabilities.minImageCount;
    if ( pState->swapChainImageCount < 2 )
    {
        pState->swapChainImageCount = 2;
    }
    if ( ( capabilities.maxImageCount > 0 ) && ( pState->swapChainImageCount > capabilities.maxImageCount ) )
    {
        // Application must settle for fewer images than desired:
        pState->swapChainImageCount = capabilities.maxImageCount;
    }

    VkSurfaceTransformFlagsKHR preTransform;
    if ( capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR )
    {
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    else
    {
        preTransform = capabilities.currentTransform;
    }

    VkImageUsageFlags nImageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if ( ( capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT ) )
    {
        nImageUsageFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    else
    {
        printf( "Vulkan swapchain does not support VK_IMAGE_USAGE_TRANSFER_DST_BIT. Some operations may not be supported.\n" );
    }

    VkSwapchainCreateInfoKHR createInfo = {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = pState->surface,
            .minImageCount = pState->swapChainImageCount,
            .imageFormat = surfaceFormat.format,
            .imageColorSpace = surfaceFormat.colorSpace,
            .imageExtent = extent,
            .imageUsage = nImageUsageFlags,
            .preTransform = preTransform,
            .imageArrayLayers = 1,
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = NULL,
            .presentMode = presentMode,
            .clipped = VK_TRUE
    };
    if ( ( capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR ) != 0 )
    {
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    }
    else if ( ( capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR ) != 0 )
    {
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    }
    else
    {
        printf( "Unexpected value for VkSurfaceCapabilitiesKHR.compositeAlpha: %x\n", capabilities.supportedCompositeAlpha );
    }

    if (vkCreateSwapchainKHR(pState->device, &createInfo, NULL, &pState->swapChain) != VK_SUCCESS) {
        printf( "%s - failed to create swap chain!\n", __FUNCTION__ );
    }

    vkGetSwapchainImagesKHR(pState->device, pState->swapChain, &pState->swapChainImageCount, NULL);
    pState->pSwapChainImages =  malloc(sizeof(VkImage) * pState->swapChainImageCount);
    vkGetSwapchainImagesKHR(pState->device, pState->swapChain, &pState->swapChainImageCount, pState->pSwapChainImages);

    pState->swapChainImageFormat = surfaceFormat.format;
    pState->swapChainExtent = extent;
}

void createImageViews(FabricAppState* pState) {
    pState->pSwapChainImageViews =  malloc(sizeof(VkImageView) * pState->swapChainImageCount);

    for (size_t i = 0; i < pState->swapChainImageCount; i++) {
        VkImageViewCreateInfo createInfo = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = pState->pSwapChainImages[i],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = pState->swapChainImageFormat,
                .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
                .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .subresourceRange.baseMipLevel = 0,
                .subresourceRange.levelCount = 1,
                .subresourceRange.baseArrayLayer = 0,
                .subresourceRange.layerCount = 1,
        };

        if (vkCreateImageView(pState->device, &createInfo, NULL, &pState->pSwapChainImageViews[i]) != VK_SUCCESS) {
            printf( "%s - failed to create image views!\n", __FUNCTION__ );
        }
    }
}

void createRenderPass(FabricAppState* pState) {
    VkAttachmentDescription colorAttachment = {
            .format = pState->swapChainImageFormat,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentReference colorAttachmentRef = {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass = {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachmentRef,
    };

    // OVR example doesn't have this
    VkSubpassDependency dependency = {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = 0,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    VkRenderPassCreateInfo renderPassInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = 1,
            .pAttachments = &colorAttachment,
            .subpassCount = 1,
            .pSubpasses = &subpass,
            .dependencyCount = 1,
            .pDependencies = &dependency,
    };

    if (vkCreateRenderPass(pState->device, &renderPassInfo, NULL, &pState->renderPass) != VK_SUCCESS) {
        printf("%s - failed to create render pass!\n", __FUNCTION__);
    }
}

static char* readBinaryFile(const char* filename, uint32_t *length) {
    FILE* file = fopen(filename, "rb");

    if (file == NULL) {
        printf("%s - file can't be opened! %s\n", __FUNCTION__, filename);
    }

    fseek(file, 0, SEEK_END);
    *length = ftell(file);
    rewind(file);
    char* contents = malloc(1 + *length * sizeof (char));
    size_t readCount = fread( contents, *length, 1, file);
    if (readCount == 0) {
        printf("%s - failed to read file! %s\n", __FUNCTION__, filename);
    }
    fclose(file);

    return contents;
}

VkShaderModule createShaderModule(FabricAppState* pState, const char* code, const uint32_t codeLength) {
    VkShaderModuleCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = codeLength,
            .pCode = (const uint32_t *) code,
    };

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(pState->device, &createInfo, NULL, &shaderModule) != VK_SUCCESS) {
        printf("%s - failed to create shader module! %s\n", __FUNCTION__, code);
    }

    return shaderModule;
}

void createGraphicsPipeline(FabricAppState* pState) {
    uint32_t vertLength;
    const char* vertShaderCode = readBinaryFile("./shaders/vert.spv", &vertLength);
    uint32_t fragLength;
    const char* fragShaderCode = readBinaryFile("./shaders/frag.spv", &fragLength);

    VkShaderModule vertShaderModule = createShaderModule(pState, vertShaderCode, vertLength);
    VkShaderModule fragShaderModule = createShaderModule(pState, fragShaderCode, fragLength);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertShaderModule,
            .pName = "main",
    };

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragShaderModule,
            .pName = "main",
    };

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = 0,
            .vertexAttributeDescriptionCount = 0,
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE,
    };

    VkPipelineViewportStateCreateInfo viewportState = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .scissorCount = 1,
    };

    VkPipelineRasterizationStateCreateInfo rasterizer = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .lineWidth = 1.0f,
            .cullMode = VK_CULL_MODE_BACK_BIT,
            .frontFace = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
    };

    VkPipelineMultisampleStateCreateInfo multisampling = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .sampleShadingEnable = VK_FALSE,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
            .blendEnable = VK_FALSE,
    };

    VkPipelineColorBlendStateCreateInfo colorBlending = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable = VK_FALSE,
            .logicOp = VK_LOGIC_OP_COPY,
            .attachmentCount = 1,
            .pAttachments = &colorBlendAttachment,
            .blendConstants[0] = 0.0f,
            .blendConstants[1] = 0.0f,
            .blendConstants[2] = 0.0f,
            .blendConstants[3] = 0.0f,
    };

    uint32_t dynamicStateCount = 2;
    VkDynamicState dynamicStates[] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = dynamicStateCount,
            .pDynamicStates = dynamicStates,
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 0,
        .pushConstantRangeCount = 0,
    };

    if (vkCreatePipelineLayout(pState->device, &pipelineLayoutInfo, NULL, &pState->pipelineLayout) != VK_SUCCESS) {
        printf("%s - failed to create pipeline layout!\n", __FUNCTION__);
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount = 2,
            .pStages = shaderStages,
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssembly,
            .pViewportState = &viewportState,
            .pRasterizationState = &rasterizer,
            .pMultisampleState = &multisampling,
            .pColorBlendState = &colorBlending,
            .pDynamicState = &dynamicState,
            .layout = pState->pipelineLayout,
            .renderPass = pState->renderPass,
            .subpass = 0,
            .basePipelineHandle = VK_NULL_HANDLE,
    };

    if (vkCreateGraphicsPipelines(pState->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &pState->graphicsPipeline) != VK_SUCCESS) {
        printf("%s - failed to create graphics pipeline!\n", __FUNCTION__);
    }

    vkDestroyShaderModule(pState->device, fragShaderModule, NULL);
    vkDestroyShaderModule(pState->device, vertShaderModule, NULL);
}

void createFramebuffers(FabricAppState* pState) {
    pState->pSwapChainFramebuffers = malloc(sizeof(VkFramebuffer) * pState->swapChainImageCount);

    for (size_t i = 0; i < pState->swapChainImageCount; i++) {
        VkImageView attachments[] = {
                pState->pSwapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo = {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = pState->renderPass,
                .attachmentCount = 1,
                .pAttachments = attachments,
                .width = pState->swapChainExtent.width,
                .height = pState->swapChainExtent.height,
                .layers = 1,
        };

        if (vkCreateFramebuffer(pState->device, &framebufferInfo, NULL, &pState->pSwapChainFramebuffers[i]) != VK_SUCCESS) {
            printf("%s - failed to create framebuffer!\n", __FUNCTION__);
        }
    }
}

void createCommandPool(FabricAppState* pState) {
    VkCommandPoolCreateInfo poolInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = pState->graphicsQueueFamilyIndex,
    };

    if (vkCreateCommandPool(pState->device, &poolInfo, NULL, &pState->commandPool) != VK_SUCCESS) {
        printf("%s - failed to create command pool!\n", __FUNCTION__);
    }
}

void createCommandBuffer(FabricAppState* pState) {
    VkCommandBufferAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = pState->commandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
    };

    if (vkAllocateCommandBuffers(pState->device, &allocInfo, &pState->commandBuffer) != VK_SUCCESS) {
        printf("%s - failed to allocate command buffers!\n", __FUNCTION__);
    }
}

void createSyncObjects(FabricAppState* pState) {
    VkSemaphoreCreateInfo semaphoreInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };

    VkFenceCreateInfo fenceInfo = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    if (vkCreateSemaphore(pState->device, &semaphoreInfo, NULL, &pState->imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(pState->device, &semaphoreInfo, NULL, &pState->renderFinishedSemaphore) != VK_SUCCESS ||
        vkCreateFence(pState->device, &fenceInfo, NULL, &pState->inFlightFence) != VK_SUCCESS) {
        printf("%s - failed to create synchronization objects for a frame!\n", __FUNCTION__);
    }
}

void recordCommandBuffer(FabricAppState* pState, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo = {
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
    };

    if (vkBeginCommandBuffer(pState->commandBuffer, &beginInfo) != VK_SUCCESS) {
        printf("%s - failed to begin recording command buffer!\n", __FUNCTION__);
    }

    VkRenderPassBeginInfo renderPassInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = pState->renderPass,
            .framebuffer = pState->pSwapChainFramebuffers[imageIndex],
            .renderArea.offset = {0, 0},
            .renderArea.extent = pState->swapChainExtent,
    };

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(pState->commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(pState->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pState->graphicsPipeline);

    VkViewport viewport = {
            .x = 0.0f,
            .y = 0.0f,
            .width = (float) pState->swapChainExtent.width,
            .height = (float) pState->swapChainExtent.height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
    };
    vkCmdSetViewport(pState->commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = {
            .offset = {0, 0},
            .extent = pState->swapChainExtent,
    };
    vkCmdSetScissor(pState->commandBuffer, 0, 1, &scissor);

    vkCmdDraw(pState->commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(pState->commandBuffer);

    if (vkEndCommandBuffer(pState->commandBuffer) != VK_SUCCESS) {
        printf("%s - failed to record command buffer!\n", __FUNCTION__);
    }
}

void drawFrame(FabricAppState* pState) {
    vkWaitForFences(pState->device, 1, &pState->inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(pState->device, 1, &pState->inFlightFence);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(pState->device, pState->swapChain, UINT64_MAX, pState->imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    vkResetCommandBuffer(pState->commandBuffer, /*VkCommandBufferResetFlagBits*/ 0);
    recordCommandBuffer(pState, imageIndex);

    VkSubmitInfo submitInfo = {
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO
    };

    VkSemaphore waitSemaphores[] = {pState->imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &pState->commandBuffer;

    VkSemaphore signalSemaphores[] = {pState->renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(pState->queue, 1, &submitInfo, pState->inFlightFence) != VK_SUCCESS) {
        printf("%s - failed to submit draw command buffer!\n", __FUNCTION__);
    }

    VkPresentInfoKHR presentInfo = {
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR
    };

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {pState->swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(pState->queue, &presentInfo);
}

void initVulkan(FabricAppState* pState) {
    printf( "%s - initializing vulkan!\n", __FUNCTION__ );
    createInstance(pState);
    setupDebugMessenger(pState);
    createSurface(pState);
    pickPhysicalDevice(pState);
    createLogicalDevice(pState);
    createSwapChain(pState);
    createImageViews(pState);
    createRenderPass(pState);
    createGraphicsPipeline(pState);
    createFramebuffers(pState);
    createCommandPool(pState);
    createCommandBuffer(pState);
    createSyncObjects(pState);
}

void mainLoop(FabricAppState* pState) {
    printf( "%s - fabric mainloop starting!\n", __FUNCTION__ );

    while (!glfwWindowShouldClose(pState->pWindow)) {
        glfwPollEvents();
        drawFrame(pState);
    }

    vkDeviceWaitIdle(pState->device);
}

void cleanup(FabricAppState* pState) {
    printf("%s - cleaning up fabric!\n", __FUNCTION__);

    vkDestroySemaphore(pState->device, pState->renderFinishedSemaphore, NULL);
    vkDestroySemaphore(pState->device, pState->imageAvailableSemaphore, NULL);
    vkDestroyFence(pState->device, pState->inFlightFence, NULL);

    vkDestroyCommandPool(pState->device, pState->commandPool, NULL);

    for (int i = 0; i < pState->swapChainImageCount; ++i) {
        vkDestroyFramebuffer(pState->device, pState->pSwapChainFramebuffers[i], NULL);
    }

    vkDestroyPipeline(pState->device, pState->graphicsPipeline, NULL);
    vkDestroyPipelineLayout(pState->device, pState->pipelineLayout, NULL);
    vkDestroyRenderPass(pState->device, pState->renderPass, NULL);

    for (int i = 0; i < pState->swapChainImageCount; ++i) {
        vkDestroyImageView(pState->device, pState->pSwapChainImageViews[i], NULL);
    }

    vkDestroySwapchainKHR(pState->device, pState->swapChain, NULL);
    vkDestroyDevice(pState->device, NULL);

    if (pState->enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(pState->instance, pState->debugMessenger, NULL);
    }

    vkDestroySurfaceKHR(pState->instance, pState->surface, NULL);
    vkDestroyInstance(pState->instance, NULL);

    glfwDestroyWindow(pState->pWindow);

    glfwTerminate();
}


int main(int argc, char *argv[])
{
    printf( "%s - Starting up Fabric!\n", __FUNCTION__ );

    FabricAppState *pState;
    pState = malloc(sizeof(*pState));
    memset(pState, 0, sizeof( *pState ) );
    pState->screenWidth = 800;
    pState->screenHeight = 600;
    pState->enableValidationLayers = true;

    initWindow(pState);
    initVulkan(pState);
        
    mainLoop(pState);
    cleanup(pState);

    return 0;
}