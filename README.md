# Learning Vulkan
I decided on December 11, 2020 to start learning Vulkan! Who knows where this journey ends and how far I will get. For detailed progress and more, see the...
## [Wiki!](https://github.com/kwilson33/learning-vulkan/wiki)

# Structure

## Files
* **Vulkan_Project_Template.zip** is a Visual Studio project template for easily creating a project with all the project settings already done. This template was created using these steps for Windows development from the [Development Environment](https://vulkan-tutorial.com/Development_environment#page_Setting-up-Visual-Studio) section of the official Vulkan tutorial. The project settings include steps like adding header directories for Vulkan and GLFW, and those steps will be unique to each person's file locations. But, the template might be helpful anyway! 

## Folders
* **VulkanTest** contains the code from [this part](https://vulkan-tutorial.com/Development_environment#page_Setting-up-Visual-Studio) of the official Vulkan tutorial
* **DrawingATriangle** contains the code from [this part](https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Base_code#page_General-structure) of the official Vulkan tutorial. I split it up into sub-folders to mirror how the tutorial is set-up. Each folder (in descending order) builds upon each other. 
   * DrawingATriangle_Setup
   * DrawingATriangle_Presentation
   * DrawingATriangle_GraphicsPipelineBasics
   * DrawingATriangle_Drawing
   * DrawingATriangle_SwapChainRecreation
