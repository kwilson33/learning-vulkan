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

    // calls funcs to initiate Vulkan objects
    void initVulkan() {

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