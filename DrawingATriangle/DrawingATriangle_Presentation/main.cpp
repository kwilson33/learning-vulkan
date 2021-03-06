// Including GLFW will included GLFW definitions (duh) and also load the Vulkan header from the LunarG SDK, which provides the functions, structures and enumerations.
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>     // These 2 headers are for reporting and propagating errors.
#include <stdexcept>
#include <vector>       // Enables use of  std::vector
#include <sstream>      // Enables creating std::stringstream error messages for exceptions
#include <optional>     // A wrapper that contains no value until you assign something it.
#include <set>          // Allows creation of sets, ie of all unique queue families.
#include <cstdlib>      // Provides the EXIT_SUCCESS and EXIT_FAILURE macros
#include <cstdint>      // Necessary for UINT32_MAX
#include <algorithm>    // Allows use of min and max functions

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

// Add two configuration variables to specify the layers to enable...
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};
// ... and whether to enable them.
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

// Define the required physical device extensions.
const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// This struct will hold queue families (almost all Vulkan commands are submitted to queues)
struct QueueFamilyIndices {
    // Need to use std::optional, because any int value could be a valid queue family, leaving no value to show an invalid family. So, using std::optional lets us check if there was any value assigned.
    std::optional<uint32_t> graphicsFamily;

    // It's possible that the queue families supporting drawign commands and the ones supporting presentation don't overlap. Must take this into account by checking both families.
    std::optional<uint32_t> presentationFamily;

    // Check if there was any value assigned to the queue families
    bool isComplete() {
        return graphicsFamily.has_value() && presentationFamily.has_value();
    }
};

// This struct contains the 3 kind of properties needed to be checked to see if a swap chain is compatible with the window surface. All of the properties are structs/ list of structs.
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities; 
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentationModes;

    // Swap chain is adequate if there is atleast one image format available and atleast one presentation mode available.
    bool isAdequate() {
        return !formats.empty() && !presentationModes.empty();
    }
};


// The program itself is wrapped into a class where we'll store the Vulkan objects as private class members and add funcs to initiate each of them, which will be called from the initVulkan func.
class HelloTriangleApplication {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    // Store reference to a window.
    GLFWwindow* window;
    // Store the Vulkan instance.
    VkInstance instance;
    // Tell Vulkan about the callback function. Even this needs to be created and destroyed.
    VkDebugUtilsMessengerEXT debugMessenger;
    // Add a surface class member to help Vulkan interface with the window system
    VkSurfaceKHR surface;
    // GPU pr other physical device that is picked is stored in this handle
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    // Logical device handle (interfaces with the physical device)
    VkDevice device;
    // Queues are automatically created with the logical device, but we need handles to interface with them.
    VkQueue graphicsQueue;
    VkQueue presentationQueue;

    // Handles for the swap chain, images stored in the swap chain, image views for images in the swap chain, the image format, and the swap extent (resolution) of the images
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;


    // ~~~~~~~~~~~~~~~ Initialization, Main loop, & Cleanup ~~~~~~~~~~~~~~~~~~~
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Initialize GLFW and create a window.
    void initWindow() {
        // initializes the GLFW library
        glfwInit();

        // Because GLFW was originally designed to create an OpenGL context, need to tell it not to.
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        // Handling windows takes special care that we'll do later, so disable it for now.
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        // Create the actual window (width, height, title, optional monitor, last param only relevant to OpenGL)
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    // Calls funcs to initiate Vulkan objects.
    void initVulkan() {
        // Very first thing to init Vulkan library is by creating an instance.
        createInstance();
        std::cout << "\n{########## Vulkan instance created. ##########}\n";

        // Then, get the validation layers callback working by setting up the debug messenger
        setupDebugMessenger();
        std::cout << "\n{########## Debug messenger setup. ##########}\n";

        // Create a surface for Vulkan to interface with the window system.
        createSurface();
        std::cout << "\n{########## VkSurfaceKHR object created. ##########}\n";

        // Pick a GPU that supports the features we need
        pickPhysicalDevice();
        std::cout << "\n{########## Physical device picked. ##########}\n";

        // Once the physical device is chosen, need to use a logical device to interface with it.
        createLogicalDevice();
        std::cout << "\n{########## Logical device created. ##########}\n";

        // Once the logical device is created to interface with a physical device, and after we've confirmed a swap chain is available (during isDeviceSuitable()), create a swap chain with the best possible settings (surface format, presentation mode, and swap extent)
        createSwapChain();
        std::cout << "\n{########## Swap chain created. ##########}\n";

        // Once the swap chain is created, create image views for the images stored within.
        createImageViews();
        std::cout << "\n{########## Image views created. ##########}\n";
    }


    // Iterates until the window is closed.
    void mainLoop() {
        // loops and checks for events like pressing the Close/X button.
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    // Deallocate resources. In C++ it's possible to perform automatic resource management like using RAII, but in this tutorial, it will be explicitly done.
    void cleanup() {
        // Destroy the VkImageView objects used for the VkImage objects within the swap chain.
        for (auto imageView : swapChainImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }

        // Destroy the swap chain before you destroy the logical device (since the swap chain is used by the logical device).
        vkDestroySwapchainKHR(device, swapChain, nullptr);

        // Destroy the logical device which interacts with the chosen physical device. 
        vkDestroyDevice(device, nullptr);

        // Destroy the VkDebugUtilsMessengerEXT object
        if (enableValidationLayers) {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }
         
        // Destroy the surface created in createSurface()
        vkDestroySurfaceKHR(instance, surface, nullptr);

        // VkInstance should be destroyed right before program exits, ignore the optional callback param.
        vkDestroyInstance(instance, nullptr);

        // Once window is closed, must destroy it.
        glfwDestroyWindow(window);

        // Terminate glfw itself
        glfwTerminate();
    }
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~



    // ~~~~~~~~~~~~~~~~ Validation Layers & Debug Messenger ~~~~~~~~~~~~~~~~~~~
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // Fill in a struct with details about the debug messenger and its callback .
    void setupDebugMessenger() {
        if (!enableValidationLayers) return;

        // Create struct and populate
        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        // Now, pass this struct to the vkCreateDebugUtilsMessengerEXT func to create the VkDebugUtilsMessengerEXT object. This function is not automatically loaded and must look up address ourselves, so I've created a proxy function to take care of that.
        // Since the debug messenger is specific to our Vulkan instance and its layers, it needs to be explicitly loaded.
        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("ERROR! Failed to setup the debug messenger!");
        }

    }

    // The vkCreateDebugUtilsMessengerEXT function isn't loaded by default, and this function looks up the address of it ourselves. This function is needed to setup the debug messenger, for validation layer purposes.
    static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
        // Get the function address using vkGetInstanceProcAddr
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        }
        else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }


    // Similarly, the vkCreateDebugUtilsMessengerEXT function needs to be explicitly loaded like vkCreateDebugUtilsMessengerEXT
    static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
        // Get the function address using vkGetInstanceProcAddr
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, pAllocator);
        }
    }


    // Extract population of the debug messenger into separate function.
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        // The message severity field lets you specify all the severity types you would like your callback to be called for
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        // The message type field lets you filter which types of messages your callback is notified about. 
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        // Specifies the pointer to the callback function
        createInfo.pfnUserCallback = debugCallback;
        // Optionally pass a pointer to the callback function via the pUserData parameter.
        createInfo.pUserData = nullptr;
    }


    // Checks if the requested layers are available. Use in createInstance().
    bool checkValidationLayerSupport() {
        uint32_t layerCount;
        // Get how many layers there are and then ...
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        // ... init an array with that size and then ...
        std::vector<VkLayerProperties> availableLayers(layerCount);
        /// ... fill up that array with the validation layers
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        // List all the available layers
        std::cout << "Available validation layers:\n~~~~~~~~~~~~~~~~~~~~~~~~\n";
        for (const auto& layer : availableLayers) {
            std::cout << '\t' << layer.layerName << '\n';
        }

        // Check if the layers defined for validationLayers variable exist in the available layers
        for (const char* layerName : validationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }
            // If a layer isn't found, print to console. Error will be thrown in createInstance()
            if (!layerFound) {
                std::cout << "ERROR! Missing " << layerName << " layer\n";

            }
            else {
                std::cout << layerName << " found!\n";
            }
        }
        std::cout << "Validation layer requirements fulfilled!" << "\n";
        return true;
    }

    // Add a static member function with PFN_vkDebugUtilsMessengerCallbackEXT prototype. The 'VKAPI_ATTR' and 'VKAPI_CALL' ensure the function has the right signature for Vulkan to call it. https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Validation_layers Returns a boolean that indicates if the Vulkan call that triggered the validation layer message should be aborted.
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {

        // Print out the callback message
        std::cerr << "validation layer: " << pCallbackData->pMessage << "\n";

        return VK_FALSE;
    }
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~



    // ~~~~~~~~~~~~~~~~~~~~~~~~~~ Vulkan Instance ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // The instance is connection b/w your app and the Vulkan library.
    void createInstance() {

        // First, check if the requested validation layers are available.
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error("ERROR! Validation layers requested, but not available!");
        }

        // Then, fill in VkApplicationInfo struct. Data is technically optional, but may provide some useful information to the driver in order to optimize our specific app.
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pNext = nullptr;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;


        // Retrieve a list of supported extensions before creating an instance. The function vkEnumerateInstanceExtensionProperties takes a ptr to a variable that stores the # of extensions and an array of VkExtensionProperties to store details of extensions. The first parameter is for filtering by a specific validation layer, which we'll ignore for now.
        uint32_t extensionCount = 0;
        // First, get # of extensions by leaving ptr to extension array empty
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        // Then, allocate an array using the retrieved size
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        // Finally, get the extension details with another call, this time passing in created array
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

        // List all the available extensions
        std::cout << "\nAvailable Vulkan extensions:\n~~~~~~~~~~~~~~~~~~~~~~~~\n";
        for (const auto& extension : availableExtensions) {
            std::cout << '\t' << extension.extensionName << '\n';
        }

        // This next struct is NOT optional and tells the Vulkan driver which global extensions and validation layers we want to use.
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        // Include validation layers if they are enabled
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else {
            createInfo.enabledLayerCount = 0;
        }

        // Get list of required GLFW extensions (and check if they are available), and add debug messenger extension if validation layers are enabled.
        auto requiredExtensions = getAndCheckRequiredExtensions(availableExtensions);

        // add # extensions, and the extension names, to the VkInstanceCreateInfo struct
        createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
        createInfo.ppEnabledExtensionNames = requiredExtensions.data();


        // Create an additional debug messenger so that we can debug any issues in the vkCreateInstance and vkDestroyInstance calls. Have to do this because the vkCreateDebugUtilsMessengerEXT call requires a valid instance to have been created and vkDestroyDebugUtilsMessengerEXT must be called before the instance is destroyed.
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;

        // Add validation layer info the struct.
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
            // Populate the debug messenger meant specifically for testing vkCreateInstance and vkDestroyInstance by passing it as pNext to the createInfo struct. This is the only way to test those methods with a debug messenger for a Vulkan instance.
            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        }
        else {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        // We've now specified everything Vulkan needs to create an instance
        // The general pattern for object creation function paramers is
        // 1) Pointer to struct with creation info
        // 2) Pointer to custom allocator callbacks, always nullptr for this tutorial
        // 3) Pointer to the variable that store the handle to the new object
        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }

    }

    // Returns the list of extensions based on whether validation layers are enabled or not. The GLFW extensions are always required, but the debug messenger extension is conditionally added.
    std::vector<const char*> getAndCheckRequiredExtensions(std::vector<VkExtensionProperties> availableExtensions) {
        // GLFW has built-in func that returns the extensions it needs for Vulkan to interface with the window system.
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        // Use the built-in func to get # extensions.
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        // Print out the glfw extensions
        std::cout << "\nRequired GLFW extensions:\n~~~~~~~~~~~~~~~~~~~~~~~~\n";
        for (uint32_t i = 0; i < glfwExtensionCount; i++) {
            std::cout << "\t" << glfwExtensions[i] << "\n";
        }

        // Check if the GLFW extensions are included in the available extensions
        checkRequiredExtensionsPresent(availableExtensions, glfwExtensions, glfwExtensionCount);

        // Create a char* vector filled with the glfwExtensions array. Start from beginning of glfwExtensions, and go until end of the array.
        std::vector<const char*> requiredExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        // To set up a callback in the program to handle messages and associated details, have to setup a debug messenger with a callback using the VK_EXT_debug_utils extension (add using macro below)
        if (enableValidationLayers) {
            requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        return requiredExtensions;

    }

    // Checks if required extensions (ie, GLFW extensions) are available
    void checkRequiredExtensionsPresent(std::vector<VkExtensionProperties> availableExt, const char** requiredExt, int requiredExtCount) {
        // Loop through the required extensions and...
        for (int i = 0; i < requiredExtCount; ++i) {
            bool extension_found = false;
            // loop through the available extensions to see if there's a match
            for (const auto& extension : availableExt) {
                if (strcmp(requiredExt[i], extension.extensionName) == 0) {
                    extension_found = true;
                }
            }
            if (!extension_found) {
                std::stringstream errorMessage;
                std::cout << "ERROR! Missing " << requiredExt[i] << "\n";
                throw std::runtime_error(errorMessage.str().c_str());
            }
            else {
                std::cout << requiredExt[i] << " extension found!\n";
            }
        }
        std::cout << "\nExtension requirements fulfilled!" << "\n";
    }
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~



    // ~~~~~~~~~~~~~~~~~~ Surface, Swap Chain, and Image Views ~~~~~~~~~~~~~~~~
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // Creates a VkSurfaceKHR (based on system details like Windows vs Linux) for Vulkan to interface with the window system
    void createSurface() {
        // glfwCreateWindow surface takes care of platform specific instantiations of a VkSurfaceKHR surface. For example, on Windows this call will create a VkWin32SurfaceCreateInfoKHR  struct, fill it in with platform specific details, and then call vkCreateWin32SurfaceKHR(). On Linux, different methods will be used.
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("ERROR! Failed to create window surface!");
        }

    }
    
    
    // This function will populate the swap chain struct
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
        SwapChainSupportDetails details;
        
        // Start with the basic surface capabilities. All of the support querying functions have device and surface as the first 2 params, because they are the core components of the swap chain.
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        // Next, query the supported surface formats. First must query the # of formats ...
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
        if (formatCount != 0) {
            // ... and then fill in from there.
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        // Finally, query the supported presentation modes in the same way.
        uint32_t presentationModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationModeCount, nullptr);
        if (presentationModeCount != 0) {
            // ... and then fill in from there.
            details.presentationModes.resize(presentationModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationModeCount, details.presentationModes.data());
        }

        return details;
    }

    // Even when a swap chain is deemed adequate, there are other settings to determine to get the best possible swap chain. One of these settings is the surface format (color depth) This method gets the best possible surface format.
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        // Each VkSurfaceFormatKHR entry contains a format and colorSpace member. We want the colorspace to support SRGB if possible, and for the format to be VK_FORMAT_B8G8R8A8_SRGB (32 bits per pixel)
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {

                return availableFormat;
            }
        }

        // If those conditions aren't met, just settle for the first format that is specified
        return availableFormats[0];
    }

    // Presentation mode is arguably the most important setting for the swap chain, because it represents the actual conditions for showing images to the screen.
    VkPresentModeKHR chooseSwapPresentationMode(const std::vector<VkPresentModeKHR> &availablePresentationModes) {
        // 4 possible presentation modes.

        // VK_PRESENT_MODE_IMMEDIATE_KHR - images submitted by app are transferred right away. Can result in tearing.

        // VK_PRESENT_MODE_FIFO_KHR - Similar to VSYNC in modern games. Display takes an image from front of queue when the display is refreshed. and program inserts rendered images to back of queue.

        // VK_PRESENT_MODE_FIFO_RELAXED_KHR - If the app is late and queue is empty at the last vertical blank (moment the display is refreshed), the image is transferred right away when it finally arrives insread of waiting until the next vertical blank. Can result in tearing.

        // VK_PRESENT_MODE_MAILBOX_KHR - Instead of blocking the app when the queue is full, the images that are already in the queue will be replaced with newer ones.

        // VK_PRESENT_MODE_MAILBOX_KHR can be used to implement triple buffering, which allows you to avoid tearing and has less latency issues than standard verticle sync that uses double buffering. This mode renders new images that are as up-to-date as possible right until the vertical blank.
        for (const auto& availablePresentationMode : availablePresentationModes) {
            if (availablePresentationMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentationMode;
            }
        }


        // This mode is guaranteed to be available, if the desired one is not.
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    // The swap extent is the resolution of the swap chain images and it's almost always equal to the resolution of the window we're drawing to in pixels.
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) {
        
        // Vulkan tells us to match the resolution of the window by setting the width and height in the currentExtent member of VkSurfaceCapabilitiesKHR. But, some window managers allow us to differ here and this is indicated by setting the width and height to UINT32_MAX. If this happens, we we'll pick the resolution that best matches the window within the minImageExtent and maxImageExtend bounds.

        // Setting the width and height to UINT32_MAX notifies Vulkan to not use the default currentExtent. If either not set to UINT32_MAX, just return  what's already in currentExtent.
        if (capabilities.currentExtent.width != UINT32_MAX) {
            return capabilities.currentExtent;
        }

        // GLFW uses two units when measuring sizes, pixels and screen coords. For example, the WIDTH and HEIGHT specified in the hader are in screen coords. But Vulkan works with pixels, so the swap chain extent must be specified in pixels too. For certain displays, these units don't match up and we will have to use glfwGetFramebufferSize() to get the resolution of the window in pixels.
        else {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            // Pick the resolution that best matches the window within the minImageExtent and maxImageExtent bounds.

            // max( min width,   min(max width, actual width) )
            actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
            // max( min height,   min(max height, actual height) )
            actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

            // If not using the default currentExtent, return this with updated width and height
            return actualExtent;
        }
    }



    // Next, create the swap chain for rendered images to be sent to and for presenting images to the window system using all the best settings found above.
    void createSwapChain() {
        // Fill in the swap chain struct with capabilities, surface formats, and presentation modes.
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

        // Once the swap chain struct is created, choose the best surface format, the best presentation mode, and the correct swap extent.
        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        std::cout << "\nSurface Format: " << surfaceFormat.format << ", Color Space: " << surfaceFormat.colorSpace << "\n";
        VkPresentModeKHR presentationMode = chooseSwapPresentationMode(swapChainSupport.presentationModes);
        std::cout << "Presentation Mode: " << presentationMode << "\n";
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);
        std::cout << "Swap Extent Width: " << extent.width << ", Swap Extent Height: " << extent.height << "\n\n";

        // Recommended to request atleast one more image than minimum, to decrease likelihood we will have to wait on the driver to complete internal operations before acquiring another image to render to.
        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        // Also make sure we're not exceeding the max # of images. Max of 0 means there is no maximum.
        if (swapChainSupport.capabilities.maxImageArrayLayers > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        // Like other Vulkan objects, creating the swap chain object requires filling in a large struct.
        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        // imageArrayLayers specifies amount of layers for each image to consist of. THis is always 1 unless you are developing a stereoscopic 3D app.
        createInfo.imageArrayLayers = 1;
        // imageUsage specifies what kind of operations we'll use the images in the swap chain for. In this tutorial we'll render directly to them, meaning they're used at color attachment. Other options include render to a separate image to perform post-processing.
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        // Next, we need to specify how to handle swap chain images that will be used across multiple queue families.
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentationFamily.value() };
        
        // If the queue families aren't the same, gotta do this. In this case we'll be drawing on the images in the swap chain from the graphics queue, and then submitting them on the presentation queue.
        if (indices.graphicsFamily != indices.presentationFamily) {
            // This sharing mode means images can be used across multiple queue families without explicity ownership transfers. This mode requires you to specify in advance b/w which queue families ownership will be shared using queueFamilyIndexCount and pQueueFamilyIndices params.
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        // Otherwise, there is one queue family that does drawing and presentation.
        } else {
            // This sharing mode means an image is owned by one queue family at a time and ownership must explictly be transferred using it in another queue family. Offers best performance.
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;       // Optional for VK_SHARING_MODE_EXCLUSIVE
            createInfo.pQueueFamilyIndices = nullptr;   // Optional for VK_SHARING_MODE_EXCLUSIVE
        }

        // To specify you don't want any transformation done to all images, specify the current transform.
        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        // Specifies if the alpha channel should be used for blending with other windows in the window system. You'll almost always want to ignore the alpha channel by setting it to opaque.
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        // Set the presentation mode (chosen earlier).
        createInfo.presentMode = presentationMode;
        // If true, means we don't care about color of pixels that are obscured. Offers best results.
        createInfo.clipped = VK_TRUE;

        // With Vulkan, it's possible your swap chain becomes invalid or unoptimized while app is still running for example if window is resized. In this case, the swap chain needs to be recreated from scratch and a reference to an old one needs to be specified. Ignore for now.
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        // FINALLY, create the swap chain by providing the device, the creation info, optional custom allocators, and pointer to store the handle in.
        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
            throw std::runtime_error("ERROR! Failed to create swap chain!");
        }

        // Now that the swap chain is created, we need to retrieve the handles for the VkImages stored within. First get the # images. We specified earlier the minimum # of images, and what is returned here may be larger than that. Then ...
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
        // ... resize the handle for the swap chain images with that size and lastly ...
        swapChainImages.resize(imageCount);
        // ... retrieve the handles with another call to vkGetSwapchainImagesKHR
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
        std::cout << "Number of swap chain images: " << imageCount << "\n";

        // Store the values of the current swap chain surface format and extent in the member variables.
        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }


    // Finally, create VkImageViews for interfacing with the VkImage objs within the swap chain
    void createImageViews() {
        // Resize to the # of swap chain VkImage objects
        swapChainImageViews.resize(swapChainImages.size());

        // Iterate over the swap chain images
        for (size_t i = 0; i < swapChainImages.size(); i++) {
           // Create a VkImageView object for each VkImage
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapChainImages[i];

            // The viewType and format fields specify how the image should be interpreted, for example 1D textures, 2D textures, 3D textures, and cube maps, and in which format (color format and colorspace)
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = swapChainImageFormat;

            // The components field allows you to 'swizzle' color channels around. For example, you can map all of the channels to the red channel for a monochrome texture. We'll stick to the default mapping.
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            // The subresourceRange field describes what the image's purpose is and which part should be accessed. Our images will be used as color targets w/o any mipmapping levels or multiple layers
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            // Finally, create the VkImageViews with the creation info
            if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("ERROR! Failed to create image views!");
            }
        }
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~



    // ~~~~~~~~~~~~~~~~~~ Physical and Logical Devices ~~~~~~~~~~~~~~~~~~~~~~~
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // Look for and select a GPU in the system that supports the features we need.
    void pickPhysicalDevice() {
        uint32_t deviceCount = 0;
        // Query the # of devices and then ...
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        // ... check if we should allocate an array and then ...
        if (deviceCount == 0) {
            throw std::runtime_error("ERROR! Failed to find GPUs with Vulkan support!");
        }
        // ... fill in vector to hold all of the VkPhysicalDevice handles
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        // Evaluate each handle and see if suitable for operations we want to perform. Exit if one is found.
        for (const auto& device : devices) {
            if (isDeviceSuitable(device)) {
                physicalDevice = device;
                break;
            }
        }

        // If no device is suitable, physicalDevice will stay as VK_NULL_HANDLE
        if (physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("ERROR! Failed to find a suitable GPU!");
        }

    }

    // Check if physical device handle is suitable for operations we need.
    bool isDeviceSuitable(VkPhysicalDevice device) {
        // Start by querying for some details. Basic properties like name, type, and supported Vulkan version can be gotten using vkGetPhysicalDeviceProperties
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        std::cout << "\nPhysical device set to: " << deviceProperties.deviceName << "\n";

        // To get optional features like texture compression, 64 bit floats, and multi viewport rendering (for VR) can be gotten using vkGetPhysicalDeviceFeatures
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);


        // Get the indices of queue families that are supported by the physical device.
        QueueFamilyIndices indices = findQueueFamilies(device);
        

        // Check if all extensions are supported.
        bool extensionsSupported = checkDeviceExtensionSupport(device);

        // Check if the swap chain is adqeuate by checking if there is atleast one supported image format and atleast one supported presentation mode.
        bool swapChainAdequate = false;
        if (extensionsSupported) {
            // NOTE! This is not creating the swap chain we'll be using, simply is checking if there is a valid swap chain. The swap chain will be created after the logical device is created.
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            swapChainAdequate = swapChainSupport.isAdequate();
        }

        // Combine these checks together to see if the device has valid queue families, has extensions supported, and has a valid swap chain.
        return indices.isComplete() && extensionsSupported && swapChainAdequate;
    }

 

    // Find the queue families supported by the physical device and return them in a struct.
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;

        // Assign index to queue families that could be found
        uint32_t queueFamilyCount = 0;
        // Get number of queue families ... 
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        // ... create a vector with that size and ... 
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        // ... get the properties for the queue families.
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());


        // Need to find atleast one queue family that supports VK_QUEUE_GRAPHICS_BIT and supports presentation to a window surface
        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            // Check for graphics support
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }
            // Check for presentation support
            VkBool32 presentationSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentationSupport);
            if (presentationSupport) {
                indices.presentationFamily = i;
            }

            // Early exit if a valid queue family is found.
            if (indices.isComplete()) {
                break;
            }

            i++;
        }

        return indices;
    }

    // Check if the required physical device extensions are supported
    bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
        // Query the # of extensions and then create vector of that size ...
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        // ... and fill data in with another call to vkEnumerateDeviceExtensionProperties
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        // Use a set of strings to represent the unconformed required extensions.
        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
        // For each avaialable extension, remove from set of unconfirmed required extensions.
        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        // If all the required extensions were present in the available extensions, this will be true.
        if (requiredExtensions.empty())
        {
            std::cout << "Physical device extension requirements met!" << "\n";
        }
        return requiredExtensions.empty();
    }

    // Create a logical device to interface with the chosen physical device.
    void createLogicalDevice() {
        // Get the indices of queue families for the physical device
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        // Create a set of all unique queue families that are necessary for the required queues. Also create a vector for the createInfo structs needed for each.
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentationFamily.value()};

        // Vulkan lets you assign priorties (0.0 to 1.0) to queues to influence scheduling of command buffer execution. Required even if there is only a single queue. Loop over the set of unique queue families and create structs for each, and push to vector of createInfo structs.
        float queuePriority = 1.0f;
        for (uint32_t queueFamiliy : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamiliy;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }


        // Next info we need to specify is the set of physical device features we'll be using
        VkPhysicalDeviceFeatures deviceFeatures{};
        // blank for now

        // With those 2 structs in place, can start filling in the main VkDeviceCreateInfo struct
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        // pass in the size and data from the VkDeviceQueueCreateInfo structs ...
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        // ... and the physical device features struct
        createInfo.pEnabledFeatures = &deviceFeatures;

        // Pass in the device extension count and names
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        // New versions of Vulkan ignore distinctions b/w instance and device specific validation layers. Specifying here to be compatible with older implementations.
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else {
            createInfo.enabledLayerCount = 0;
        }

        // Finally, create the logical device. Params are the phys device to interface with, the queue and usage info we just specified, the optional alloc callbacks pointer, and a pointer to store the logical device handle.
        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
            throw std::runtime_error("ERROR! Failed to create a logical device!");
        }

        // Retrieve queue handles for the graphics and presentation queue families. The params are the logical device, the queue family, the queue index (set to 0 because only creating 1 handle from this family), and pointer to store the queue handle in.
        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, indices.presentationFamily.value(), 0, &graphicsQueue);

    }
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
};

int main() {
    HelloTriangleApplication app;

    // If any kind of fatal error occurs, we'll throw a std::runtime_error and propagate the message to the main function and printed to command prompt.
    // Also, catch more general std::exception errors
    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}