// Including GLFW will included GLFW definitions (duh) and also load the Vulkan header from the LunarG SDK, which provides the functions, structures and enumerations.
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// The stdexcept and iostream headers are included for reporting and propagating errors.
#include <iostream>
#include <stdexcept>

// The cstdlib header provides the EXIT_SUCCESS and EXIT_FAILURE macros
#include <cstdlib>


const uint32_t WIDTH  = 800;
const uint32_t HEIGHT = 600;

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
    
    // private class member to store reference to window
    GLFWwindow* window;
    // private class member to store the Vulkan instance
    VkInstance instance;

    // The instance is connection b/w your app and the Vulkan library
    void createInstance() {
        // first, fill in struct. Data is technicall optional, but may provide some useful information to the driver in order to optimize our specific app.
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pNext = nullptr;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;
        

        // this next struct is NOT optional and tells the Vulkan driver which global extensions and validation layers we want to use.
        VkInstanceCreateInfo createInfo{};  
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        // GLFW has built-in func that returns the extensions it needs for Vulkan to interface with the window system.
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        // Use the built-in func to get # extensions, and add it, and the extension names, to the struct
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        createInfo.enabledExtensionCount = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;

        // Deternines the global validation layers to enable.
        createInfo.enabledLayerCount = 0;

        // We've now specified everything Vulkan needs to create an instance
        // The general pattern for object creation function paramers is
        // 1) Pointer to struct with creation info
        // 2) Pointer to custom allocator callbacks, always nullptr for this tutorial
        // 3) Pointer to the variable that store the handle to the new object
        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }

    }

    // calls funcs to initiate Vulkan objects
    void initVulkan() {
        // Very first thing to init Vulkan library is by creating an instance.
        createInstance();

    }

    

    
    // initialize GLFW and create a window
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

    // iterates until the window is closed
    void mainLoop() {
        // loops and checks for events like pressing the Close/X button.
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    // deallocate resources. In C++ it's possible to perform automatic resource management like using RAII, but in this tutorial, it will be explicitly done.
    void cleanup() {
        // Once window is closed, must destroy it.
        glfwDestroyWindow(window);

        // Terminate glfw itself
        glfwTerminate();

    }
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