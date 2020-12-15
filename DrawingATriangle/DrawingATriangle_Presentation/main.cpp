// Including GLFW will included GLFW definitions (duh) and also load the Vulkan header from the LunarG SDK, which provides the functions, structures and enumerations.
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// The stdexcept and iostream headers are included for reporting and propagating errors.
#include <iostream>
#include <stdexcept>
// Enables use of  std::vector
#include <vector>
// Enables creating std::stringstream error messages for exceptions
#include <sstream>
// Allows use of std::optional, a wrapper that contains no value until you assign something it.
#include <optional>
// The cstdlib header provides the EXIT_SUCCESS and EXIT_FAILURE macros
#include <cstdlib>

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


// This struct will hold queue families (almost all Vulkan commands are submitted to queues)
struct QueueFamilyIndices {
    // Need to use std::optional, because any int value could be a valid queue family, leaving no value to show an invalid family. So, using std::optional lets us check if there was any value assigned.
    std::optional<uint32_t> graphicsFamily;

    // Check if there was any value assigned to the queue family
    bool isComplete() {
        return graphicsFamily.has_value();
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
    // GPU that is picked is stored in this handle
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    // Logical device handle
    VkDevice device;
    // Queues are automatically created with the logical device, but we need a handle to interface with them.
    VkQueue graphicsQueue;



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
        // Then, get the validation layers callback working by setting up the debug messenger
        setupDebugMessenger();
        // Pick a GPU that supports the features we need
        pickPhysicalDevice();
        // Once the physical device is chosen, need to use a logical device to interface with it.
        createLogicalDevice();
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

        // Destroy the logical device which interacts with the chosen physical device. 
        vkDestroyDevice(device, nullptr);

        // Destroy the VkDebugUtilsMessengerEXT object
        if (enableValidationLayers) {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }

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


        // Get the indices of queue families that are supported by the physical device ...
        QueueFamilyIndices indices = findQueueFamilies(device);
        // ... and if atleast one of the device families is valid (has value set).
        return indices.isComplete();
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


        // Need to find atleast one queue family that supports VK_QUEUE_GRAPHICS_BIT
        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }

            // Early exit if a valid queue family is found.
            if (indices.isComplete()) {
                break;
            }

            i++;
        }

        return indices;
    }

    // Create a logical device to interface with the chosen physical device.
    void createLogicalDevice() {
        // Get the indices of queue families for the physical device
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        // We only care about a queue with graphics capabilities right now.
        queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
        queueCreateInfo.queueCount = 1;

        // Vulkan lets you assign priorties (0.0 to 1.0) to queues to influence scheduling of command buffer execution. Required even if there is only a single queue.
        float queuePriority = 1.0f;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        // Next info we need to specify is the set of physical device features we'll be using
        VkPhysicalDeviceFeatures deviceFeatures{};

        // With those 2 structs in place, can start filling in the main VkDeviceCreateInfo struct
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        // pass in the queue struct ...
        createInfo.pQueueCreateInfos = &queueCreateInfo;
        createInfo.queueCreateInfoCount = 1;
        // ... and the physical device features struct
        createInfo.pEnabledFeatures = &deviceFeatures;

        createInfo.enabledExtensionCount = 0;

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

        // Retrieve queue handle for the graphics queue family. The params are the logical device, the queue family, the queue index (set to 0 because only creating 1 handle from this family), and pointer to store the queue handle in.
        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);

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

