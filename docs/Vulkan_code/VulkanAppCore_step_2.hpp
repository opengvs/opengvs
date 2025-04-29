#pragma once
//通过GLFW_INCLUDE_VULKAN宏定义，在glfw3.h头文件中自动包含Vulkan/vulkan.h头文件
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>

//通过程序应用需要的库文件
#pragma comment( lib,"vulkan-1.lib" )
#pragma comment( lib,"glfw3.lib" )

class VulkanAppCore 
{
protected:
    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;

    GLFWwindow* window = nullptr;

    VkInstance instance;

public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

protected:
    void initWindow()
    {
        //初始化glfw库
        glfwInit();
        //显式地设置GLFW，告诉它不要创建OpenGL上下文
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        //处理窗口大小的变化需要特别小心，我们会在以后介绍它，暂时先禁止改变窗口大小
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void initVulkan() {
        createInstance();
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanup() {
        if (instance != nullptr)
        {
            vkDestroyInstance(instance, nullptr);
            instance = nullptr;
        }
        
        glfwDestroyWindow(window);

        glfwTerminate();
    }
private:
    void createInstance()
    {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 3, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 3, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        
        createInfo.ppEnabledExtensionNames = extensions.data();

        auto InvalidationLayers = getValidationLayersExtensions(true);
        createInfo.enabledLayerCount = InvalidationLayers.size();             //系统允许的Layer层数量
        if (createInfo.enabledLayerCount > 0)
        {
            createInfo.ppEnabledLayerNames = InvalidationLayers.data();  //指向Lager层名称数组的指针，一般为二维数组
        }
        else
        {
            createInfo.ppEnabledLayerNames = nullptr;  //指向Lager层名称数组的指针，一般为二维数组
        }
        //创建Vulkan实例
        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) 
        {
            throw std::runtime_error("failed to create instance!");
        }
    }
    std::vector<const char*>  m_VkInstanceExtensions;
    std::vector<const char*>  m_VkValidationLayers;

    std::vector<const char*> getRequiredExtensions(bool _enableValidationLayers=false) {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (_enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }
    std::vector<const char*>  getValidationLayersExtensions(bool _enableValidationLayers = true)
    {
        std::vector<const char*>  ValidationLayers;


        return ValidationLayers;
    }
};


