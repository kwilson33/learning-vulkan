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
#include <fstream>      // Used for loading in binary SPIR-V data


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

    // Store the render pass object in this handle.
    VkRenderPass renderPass;
    // Store the pipeline layout, which is used to pass in uniform values in shaders for example, in this handle.
    VkPipelineLayout pipelineLayout;
    // Store the graphics pipeline in this handle.
    VkPipeline graphicsPipeline;

    // Hold the framebuffers here. They will provide the attachments needed for the render pass. 
    std::vector<VkFramebuffer> swapChainFramebuffers;

    // Store all of the commands buffers in a command pool. They manage the memory that is used to store the buffers.
    VkCommandPool commandPool;





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

        // Tell Vulkan about the framebuffer attachments that will be used while rendering through a render pass object.
        createRenderPass();
        std::cout << "\n{########## Render pass created. ##########}\n";

        // Now that the Image views are created, there needs to be a pipeline the input data goes through
        createGraphicsPipeline();
        std::cout << "\n{########## Graphics pipeline created. ##########}\n";

        // Now, create framebuffers so the render pass can get the attachments from the swapchain.
        createFramebuffers();
        std::cout << "\n{########## Framebuffers created. ##########}\n";
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

        // Destroy all the framebuffers that reference image views which describe attachments needed for the render pass.
        for (auto framebuffer : swapChainFramebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }

        // Destroy the graphics pipeline.
        vkDestroyPipeline(device, graphicsPipeline, nullptr);

        // Destroy the pipeline layout that is used to send uniform values and push constants to the graphics pipeline.
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

        // Destroy the render pass object which describes to Vulkan about framebuffer attachments and how to handle data.
        vkDestroyRenderPass(device, renderPass, nullptr);

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



    // ~~~~~~~~~~~~~~~~~~~~ Vulkan Instance and Surface ~~~~~~~~~~~~~~~~~~~~~~~
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

    // Creates a VkSurfaceKHR (based on system details like Windows vs Linux) for Vulkan to interface with the window system
    void createSurface() {
        // glfwCreateWindow surface takes care of platform specific instantiations of a VkSurfaceKHR surface. For example, on Windows this call will create a VkWin32SurfaceCreateInfoKHR  struct, fill it in with platform specific details, and then call vkCreateWin32SurfaceKHR(). On Linux, different methods will be used.
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("ERROR! Failed to create window surface!");
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
        std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentationFamily.value() };

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



    // ~~~~~~~~~~~~~~~~~~ Swap Chain & Image Views ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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
    VkPresentModeKHR chooseSwapPresentationMode(const std::vector<VkPresentModeKHR>& availablePresentationModes) {
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
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {

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
        }
        else {
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

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Render Pass ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    /*Create a render pass object to tell Vulkan about the framebuffer attachments that will be used while rendering. Need to specify
    how many color & depth buffers there will be, how many samples to use for each of them, and how
    their contents should be handled throughout the rendering operations.*/
    void createRenderPass() {
        // For this tutorial, just need a single color buffer attachment represented by one of the images from the swap chain.
        VkAttachmentDescription colorAttachment{};
        // Format should match format of swap chain images. Samples is for multisampling.
        colorAttachment.format = swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        /*These determine what to do with the color and depth data in the attachment before rendering and after rendering. For loadOp,
         we will clear the framebuffer to black before drawing a new frame. For storeOp, we will store data memory so it
         can be read later and drawn to screen.*/
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        // Applies to stencil data, which we don't care about.
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        /*The initial layout specifies which layout the image will have before the render pass
        begins. The final layout specifies the layour to automatically transition to when the render
        pass finishes. Using VK_IMAGE_LAYOUT_UNDEFINED for initial layout means we don't care what previous
        layout the image was in. This also means the contents of the image aren't guaranteeed to be
        preserved (doesn't matter b/c we're clearing it anyway). Using VK_IMAGE_LAYOUT_PRESENT_SRC_KHR for
        final layout specifies we want the image to be presented in the swap chain after the render pass.*/
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        /*A single render pass can consist of multiple subpasses, which are subsequent rendering
        operations that depend on the contents of framebuffers in previous passes. Can be used to apply
        post processing effects. Can group these multiple subpasses (or just one) into one render pass, and Vulkan will
        reorder the operations and conserve mem bandwiddth for maybe better perf. Every subpass references
        one or more attachments (one of which we created above). These references are structs themselves.*/
        VkAttachmentReference colorAttachmentRef{};
        // attachment specifies which attachment to reference by its index
        colorAttachmentRef.attachment = 0;
        // layout specifies which layout we would like the attachment to have during a subpass that uses this reference. We intend to use the attachment to function as a color buffer.
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        /*The subpass is described using another structure. The subpass is what
        will use the color attachment through the attachment reference created above*/
        VkSubpassDescription subpass{};
        // Be explicit about this subpass being a graphics subpass.
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        // The index of the attachment in the pColorAttachments array is referenced in the FS with the "layout(location = 0) out vec4 outColor" directive
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        // Now that the attachment and a basic subpass referencing it are made, can create the render pass itself.
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("ERROR! Failed to create render pass!");
        }
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Shaders ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    /* Helper function to load in the binary SPIR-V shader data. This function
    will read all of the bytes from the specified file and retuern them in a byte
    array managed by std::vector.*/
    static std::vector<char> readShaderFile(const std::string& filename) {
        /* Open the file with two flags. ate flag means start reading at end of file so
        * we can get the size of the file. binary flag means read the file as binary.*/
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("ERROR! Failed to open file!");
        }

        // Use the pointer at end of file to get the size & allocate a buffer.
        size_t fileSize = (size_t)file.tellg();
        std::cout << "\n" << filename << " size is " << fileSize << " bytes.\n";
        std::vector<char> buffer(fileSize);

        // Go back to beginning of file and read all of the bytes at once.
        file.seekg(0);
        file.read(buffer.data(), fileSize);

        // Close the file and return the buffer.
        file.close();
        return buffer;
    }

    /* Take a buffer with the bytecode and create a shader module. They are just a thin
     wrapper around shader bytecode.*/
    VkShaderModule createShaderModule(const std::vector<char>& code) {
        // Creating a shader module requires only the bytecode and the length of it.
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        /* The bytecode size is in bytes, but the bytecode ptr is uint32_t intead of
        a char pointer, so have to cast the ptr to uint32_t. Also need to ensure the data satisfies the data
        alignment requirements. The data stored in std::vector already ensures this.*/
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("ERROR! Failed to create shader module!");
        }

        return shaderModule;
    }
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


    // ~~~~~~~~~~~~~~~~~~~~~~~~ Graphics Pipeline ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // Create the pipeline for input data to go into, get processed, and drawn to the window system.
    void createGraphicsPipeline() {
        // ####### Vertex & fragment shader #########
        // Read in the vertex and fragment shader bytecode. 
        auto vertShaderCode = readShaderFile("shaders/vert.spv");
        auto fragShaderCode = readShaderFile("shaders/frag.spv");

        // Store the bytecode in a thin wrapper (shader module)
        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        std::cout << "\nVertex shader module created.\n";
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);
        std::cout << "Fragment shader module created.\n";

        // To actually use the shaders, need to assign them to a specific pipeline stage through structs.
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        // Tell Vulkan which pipeline stage the shader is going to be used.
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        // Specify the shader module containing the code.
        vertShaderStageInfo.module = vertShaderModule;
        // Specify the function to invoke, known as the entrypoint.
        vertShaderStageInfo.pName = "main";
        // This field allows you to specify values for shader constants. More efficient than configuring the shader during render time.
        vertShaderStageInfo.pSpecializationInfo = nullptr;

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        // Store the shader structs in an array.
        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
        // ##########################################


        // ########### Vertex input #################
        /* This struct describes the format of the vertex data that will be passed to the vertex shader.
         It describes this through Bindings (spacing b/w data and whether the data is per-vertex or per-instance)
         and through Attribute descriptions (type of attribs passed to the VS, which binding to load them from & and which offset)*/
        VkPipelineVertexInputStateCreateInfo  vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;            // Vertex data is hardcoded for now
        vertexInputInfo.pVertexBindingDescriptions = nullptr;      // Optional
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr;      // Optional 
        std::cout << "Vertex input format specified.\n";
        // ##########################################


        // ########## Input assembly ################
        /* This struct describes what kind of geom will be drawn from the vertices (topology) and if primitive
        restart should be enabled. */
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        // Draw triangle from every 3 vertices w/o reuse
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        // Used with element buffers to perform optimizations like reusing vertices.
        inputAssembly.primitiveRestartEnable = VK_FALSE;
        std::cout << "Input assembly specified.\n";
        // ##########################################


        // ########## Viewport & scissors ###########
        /* A viewport basically describes the region of the framebuffer that the output
        will be rendered to. Almost always (0,0) to (width, height)*/
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)swapChainExtent.width;
        viewport.height = (float)swapChainExtent.height;
        // minDepth can actually be higher than maxDepth. If not doing anything special, keep min=0.0 and max=1.0
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        /* Scissor rectangles define in which region pixels will actually be stored.
        Any pixels outside the rectangles will be discarded by the rasterizer. They
        act like a filter rather than a transformation. In this tutorial we want to
        draw the entire framebuffer, so just define a rectangle that covers it.*/
        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = swapChainExtent;

        // Combine the viewport and scissor rectangle into a viewport state.
        VkPipelineViewportStateCreateInfo viewPortState{};
        viewPortState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewPortState.viewportCount = 1;
        viewPortState.pViewports = &viewport;
        viewPortState.scissorCount = 1;
        viewPortState.pScissors = &scissor;
        std::cout << "Viewport and scissor rectangle specified.\n";
        // ##########################################


        // ############# Rasterizer #################
        /* The rasterizer takes the geom that is shaped by the vertices from the VS and turns it into
        fragments to be colored by the FS. It also does depth testing, face culling, and the scissor test.
        It can also be configured to output fragments that fill entire polygons or just the edges (wireframe).*/
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        // If true, fragments that are beyond the near & far planes are clampes as opposed to discarding them. Useful for special cases like shadow maps.
        rasterizer.depthClampEnable = VK_FALSE;
        // If true, geom never passes through the rasterization stage. This basically disables any output to the framebuffer.
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        // 3 modes available. FILL, LINE, and POINT. Use FILL to fill the area of the polygon with fragments.
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        // Thickness of the lines in terms of # of fragments. Max width depends on the HW. Anything larger than 1.0f requires you to enabled the wideLines feature.
        rasterizer.lineWidth = 1.0f;
        // Specify the cull mode (for this tutorial, cull the back face). frontFace specifies the vertex order for faces to be considered front-facing.
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        // Rasterizer can alter the depth vals by adding a constant value or biasing them based on a fragment's slope. Sometimes used for shadow mapping.
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f;             // Optional
        rasterizer.depthBiasClamp = 0.0f;             // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f;             // Optional
        std::cout << "Rasterizer specified.\n";
        // ##########################################


        // ############# Multisampling ##############
        /*One of the ways to perform anti-aliasing. Works by combining the FS results of multiple
        polygons that rasterize to the same pixel. In other words, it samples multiple points per
        pixel and averages the results together to smooth out jagged effects. Mainly occurs along edges.
        Significantly less expensive than simply rendering to a higher resolution & then downscaling, because
        even if multiple sample points are covered by the polygon, the FS is only run ONCE by interpolating
        the covered points at the pixel center. So even for 4X MSAA, if 4 sample points (for a pixel) are covered,
        the FS  is still only run once for that pixel.*/
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f;                 // Optional
        multisampling.pSampleMask = nullptr;              // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE;             // Optional
        multisampling.alphaToOneEnable = VK_FALSE;             // Optional
        std::cout << "Multisampling specified (disabled for now).\n";
        // ##########################################


        // ######### Depth & stentcil Testing #######
        /*If using a depth or stencil buffer need to configure it here.*/
        VkPipelineDepthStencilStateCreateInfo depthAndStencil{};
        std::cout << "Depth & stencil tests specified (disabled for now).\n";
        // ##########################################


        // ############# Color blending #############
        /*After a FS has returned a color, needs to be combined with the color already
        in the framebuffer. Can either mix the old and new together to produce a final color
        OR combine the old and new using a bitwise operation. There are 2 structs to fill in for
        configuring color blending.*/

        /* The first struct contains the config per attached framebuffer. The most common way
        is to implement alpha blending where we want new color to be blended with old color
        based on opacity. This is done through the params below. It computs the finalColor like so

        finalColor.rgb = newAlpha * newColor + (1 - newAlpha) * oldColor;
        finalColor.a = newAlpha.a;*/
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;              // Optional
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;             // Optional
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;                  // Optional
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;              // Optional
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;             // Optional
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;                  // Optional    
        std::cout << "Color blend attachment state specified.\n";

        // The second struct contains the global color blending settings.
        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        // If enabled, does bitwise combination for blending. Automatically disables first method.
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;     // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;                 // Optional
        colorBlending.blendConstants[1] = 0.0f;                 // Optional
        colorBlending.blendConstants[2] = 0.0f;                 // Optional
        colorBlending.blendConstants[3] = 0.0f;                 // Optional
        std::cout << "Color blend global settings specified.\n";
        // ##########################################


        // ############# Dynamic state ##############
        /*A limited amount of the state we've specified can be changed w/o recreating the pipeline, like the viewport,
        line width, and blend constants.*/
        std::cout << "Dynamic states specified (disabled for now).\n";
        // ##########################################


        // ########### Pipeline layout ##############
        /*You can use uniform values in shaders, which are globals similar to dynamic state variables, that
        can be changed at drawing time to alter behavior of shaders w/o having to recreate them. Commonly used to
        pass the transformation matrix to the VS, or to create texture samples in the FS. These uniform values
        must be specified during pipeline creation through a VKPipelineLayout object. Even if not using, need
        to create an empty layout.*/
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;            // Optional
        pipelineLayoutInfo.pSetLayouts = nullptr;      // Optional
        // The struct also specifies push constants, another way of passing dynamoc vals to shaders.
        pipelineLayoutInfo.pushConstantRangeCount = 0;            // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr;      // Optional

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("ERROR! Failed to create pipeline layout!");
        }
        std::cout << "Pipeline layout created.\n";
        // ##########################################

        // ####### Putting it all together  #########
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        // Start by referencing the array of VkPipelineShaderStageCreateInfo structs.
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        // Then reference all of the structs described during the fixed-function stage.
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewPortState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = nullptr;           // Optional
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = nullptr;           // Optional
        // Next is the pipeline layout, a Vulkan handle rather than a struct pointer
        pipelineInfo.layout = pipelineLayout;
        // Then, reference the render pass and the index of the subpass where the graphics pipeline will be used.
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        /* Vulkan lets you create a new graphics pipeline by deriving from an existing pipeline.
        The idea is it's less expensive to setup pipelines when they have alot of functionality in common with
        an existing one. Can either specify the handle of an existing pipeline or reference another pipeline
        that us about to be created by index.*/
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;    // Optional
        pipelineInfo.basePipelineIndex = -1;                // Optional

        /* Finally, create the pipeline. More params than the usual Vulkan object creation. Designed to take multiple
        VkGraphicsPipelineCreateInfo objects and create multiple VkPipeline objects in one call. The second param references
        an optional VkPipelineCache object, used to store and reuse data relevant to pipeline creation across multiple calls to vkCreateGraphicsPipelines()
        and even across program executions if the cache is stored in a file.*/
        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("ERROR! Failed to create graphics pipeline!");
        }
        // ##########################################

        /* Destroy the shader modules as soon as pipeline creation is finished,
        because the important bytecode in them has been compiled and linked. */
        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // ~~~~~~~~~~~~~~~~~~~~~~~~ Framebuffers ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    
    void createFramebuffers() {
        // There needs to be a framebuffer for each image view in the swapchain.
        swapChainFramebuffers.resize(swapChainImageViews.size());

        // Loop over all the image views and create framebuffers using that information.
        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            VkImageView attachments[] = {
                swapChainImageViews[i]
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("ERROR! Failed to create framebuffer!");
            }
        }
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