#pragma once
//通过GLFW_INCLUDE_VULKAN宏定义，在glfw3.h头文件中自动包含Vulkan/vulkan.h头文件
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <optional>

//通过程序应用需要的库文件
#pragma comment( lib,"vulkan-1.lib" )
#pragma comment( lib,"glfw3.lib" )

#ifdef NDEBUG
inline const bool enableValidationLayers = false;
#else
inline  const bool enableValidationLayers = true;
#endif

//LunarG Vulkan SDK允许我们通过 VK_LAYER_KHRONOS_validation 来隐式地开启所有可用的校验层
inline const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

//VK_KHR_SWAPCHAIN_EXTENSION_NAME 是Vulkan SDK中定义的一个宏，表示交换链扩展名称
inline const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

bool isDeviceSuitable(VkPhysicalDevice _physicaldevice)
{
    QueueFamilyIndices indices = findQueueFamilies(_physicaldevice);
    bool isPhysicalDeviceExtension = CheckPhysicalDeviceExtensionSupport(_physicaldevice,
        deviceExtensions);


    return indices.isComplete() && isPhysicalDeviceExtension;
}
class VulkanAppCore 
{
protected:
    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;

    GLFWwindow* window = nullptr;

    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

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
        CreateVkDebugUtilsMessengerEXT();
        createSurface();
        pickPhysicalDevice();
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanup() 
    {
        if (surface != nullptr)
        {
            vkDestroySurfaceKHR(instance, surface, nullptr);
            surface = nullptr;
        }
        if (enableValidationLayers) 
        {
            if (debugMessenger != nullptr)
            {
                auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
                if (func != nullptr)
                {
                    func(instance, debugMessenger, nullptr);
                }
                debugMessenger = NULL;
            }
            
        }

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
        if (enableValidationLayers && !checkValidationLayerSupport()) 
        {  //如果需要校验层，而系统检测不到校验层，需要退出
            throw std::runtime_error("应用程序需要设置校验层, 而我们检测不到需要的校验，程序退出!");
        }

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

        auto extensions = getRequiredExtensions(enableValidationLayers);
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        
        createInfo.ppEnabledExtensionNames = extensions.data();

        auto _InvalidationLayers = getValidationLayersExtensions(enableValidationLayers);
        createInfo.enabledLayerCount = _InvalidationLayers.size();             //系统允许的Layer层数量
        
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};//定义一个VkDebugUtilsMessengerCreateInfoEXT对象
        populateDebugMessengerCreateInfo(debugCreateInfo);//完成对象的调试信息设置

        if (createInfo.enabledLayerCount > 0)
        {
            createInfo.ppEnabledLayerNames = _InvalidationLayers.data();  //指向Lager层名称数组的指针，一般为二维数组
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        }
        else
        {
            createInfo.ppEnabledLayerNames = nullptr;  //指向Lager层名称数组的指针，一般为二维数组
            createInfo.pNext = nullptr;//一般情况下用于指向校验层结构体的指针，这里先设为nullptr
        }
        
        //创建Vulkan实例
        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) 
        {
            throw std::runtime_error("failed to create instance!");
        }
    }

    VkResult CreateVkDebugUtilsMessengerEXT()
    {
        if (!enableValidationLayers) 
            return  VK_NOT_READY;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) 
        {
            return func(instance, &createInfo, nullptr, &debugMessenger);
        }
        else 
        {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }
    void createSurface()
    {    //通过glfwCreateWindowSurface函数创建surface避免了我们直接与操作系统打交道
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create window surface!");
        }
    }
    void pickPhysicalDevice() 
    {   
        //❶首先需要查询系统中安装的显示设备GPU的数量
        uint32_t _VkPhysicaldeviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &_VkPhysicaldeviceCount, nullptr);
        if (_VkPhysicaldeviceCount == 0) //如果显示设备GPU的数量为您报错退出
        {
            throw std::runtime_error("没有发现 GPUs设备，程序退出！");
        }
        //❷ 枚举系统中所有显示设备GPU,并保存到数组中
        std::vector<VkPhysicalDevice> _VkPhysicaldevices(_VkPhysicaldeviceCount);
        vkEnumeratePhysicalDevices(instance, &_VkPhysicaldeviceCount, _VkPhysicaldevices.data());
       
        // ❸对std::vector<VkPhysicalDevice> _VkPhysicaldevices数组中每个物理设备进行遍历，
        //   选择一个可用的物理设备,我们通过一个独立的isDeviceSuitable函数进行判断
        for (const auto& device : _VkPhysicaldevices) 
        {
            if (isDeviceSuitable(device)) 
            {
                physicalDevice = device;
                m_VkDeviceExtensionPropertis =  getAvailableExtensionsProperties(physicalDevice);
                break;
            }
        }
        //❹ 如果没有找到合适的物理设备，程序报错退出
        if (physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }
    bool isDeviceSuitable(VkPhysicalDevice _physicaldevice)
    {
        QueueFamilyIndices indices = findQueueFamilies(_physicaldevice);
        bool isPhysicalDeviceExtension = CheckPhysicalDeviceExtensionSupport(_physicaldevice,
            deviceExtensions);


        return indices.isComplete() && isPhysicalDeviceExtension;
    }
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice _physicalDevice) 
    {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queueFamilyCount, queueFamilies.data());

        for (int i = 0;i< queueFamilies.size();i++ ) 
        {
            const VkQueueFamilyProperties& queueFamily = queueFamilies[i];
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) //判定支持VK_QUEUE_GRAPHICS_BIT类型的队列族
            {
                indices.graphicsFamily = i;
            }
			
			VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(_physicalDevice, i, surface, &presentSupport);

            if (presentSupport) 
            {
                indices.presentFamily = i;
            }

            if (indices.isComplete()) {
                break;
            }
        }

        return indices;
    }

    std::vector<const char*>  m_VkInstanceExtensions;
    std::vector<const char*>  m_VkValidationLayers;
    std::vector<VkExtensionProperties> m_VkDeviceExtensionPropertis;

    bool CheckPhysicalDeviceExtensionSupport(VkPhysicalDevice _physicaldevice, 
        const std::vector<const char*>&  _AppNeedPhysicalDeviceExtensions)
    {
        std::vector<VkExtensionProperties>  availableExtensions = getAvailableExtensionsProperties(_physicaldevice);
        bool  hResult = true;
        for (int i = 0; i < _AppNeedPhysicalDeviceExtensions.size(); i++)
        {
            std::string tempExtensionName = _AppNeedPhysicalDeviceExtensions[i];
            bool  bFind = false;
            for (int j = 0; j < availableExtensions.size(); j++)
            {
                if (tempExtensionName == availableExtensions[j].extensionName)
                {
                    bFind = true;
                    break;
                }
            }
            if (bFind == false)
            {
                hResult = false;
                std::cerr << "没有找到需要的扩展属性" << tempExtensionName << std::endl;
                break;
            }
        }
        return hResult;
    }
    //获得设备支持的所有Device类型的扩展属性列表
    std::vector<VkExtensionProperties>  getAvailableExtensionsProperties(VkPhysicalDevice _physicalDevice)
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(_physicalDevice, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties>      extensions(extensionCount);
        if (extensionCount == 0) {
            throw std::runtime_error("No extensions found!");
        }
        vkEnumerateDeviceExtensionProperties(_physicalDevice, nullptr, &extensionCount, extensions.data());
        return extensions;
    }

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
    const std::vector<const char*>  getValidationLayersExtensions(bool _enableValidationLayers = true)
    {
        if (_enableValidationLayers == true)
        {
            return validationLayers;
        }
        else
        {
            std::vector<const char*>  tempValidationLayers;
            return tempValidationLayers;
        }     
    }
    bool checkValidationLayerSupport() 
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : validationLayers) 
        {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) 
            {
                if (strcmp(layerName, layerProperties.layerName) == 0) 
                {
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

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
    {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }
};


