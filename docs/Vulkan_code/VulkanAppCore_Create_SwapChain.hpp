#pragma warning(disable: 4996)
#pragma once
//通过GLFW_INCLUDE_VULKAN宏定义，在glfw3.h头文件中自动包含Vulkan/vulkan.h头文件
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <optional>
#include <set>
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define VULKAN_HPP_NO_EXCEPTIONS
//#define VULKAN_HPP_TYPESAFE_CONVERSION
#define VULKAN_HPP_TYPESAFE_CONVERSION 1
#define NOMINMAX
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.hpp>

#include <vulkan/vulkan_structs.hpp>
//#include "CreateLogicalDevice.hpp"

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
    "VK_LAYER_KHRONOS_validation",
    //VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
};

//VK_KHR_SWAPCHAIN_EXTENSION_NAME 是Vulkan SDK中定义的一个宏，表示交换链扩展名称
inline const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_EXT_VERTEX_INPUT_DYNAMIC_STATE_EXTENSION_NAME,
    VK_KHR_MAINTENANCE_1_EXTENSION_NAME,
    VK_KHR_VERTEX_ATTRIBUTE_DIVISOR_EXTENSION_NAME,
    VK_EXT_CUSTOM_BORDER_COLOR_EXTENSION_NAME
};

//在显卡GPU驱动设计过程中，图形队列族并不一定能够将所拥有的图形信息传输到显示设备上，
// Vulkan还设计了显示队列族，图形队列族和显示队列族可以是同一个队列族，也可以是不同的队列族。
struct QueueFamilyIndices 
{
    std::optional<uint32_t>      graphicsFamily;   //图形队列族索引
    std::optional<uint32_t>      presentFamily;    //显示队列族索引
    bool isComplete() 
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};
struct SwapChainSupportDetails 
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class VulkanAppCore 
{
protected:
    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;

    GLFWwindow* window = nullptr;

    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice         logicDevice = VK_NULL_HANDLE;

    VkQueue          graphicsQueue = VK_NULL_HANDLE;
    VkQueue          presentQueue =  VK_NULL_HANDLE;



public:
    VulkanAppCore()
    {

    }
    void run() 
    {
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
        createLogicalDevice();
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
           // drawFrame();
        }
        vkDeviceWaitIdle(logicDevice);
    }

    void cleanup() 
    {

        if (logicDevice != nullptr)
        {
            vkDestroyDevice(logicDevice, nullptr);
            logicDevice = nullptr;
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
        if (surface != nullptr)
        {
            vkDestroySurfaceKHR(instance, surface, nullptr);
            surface = nullptr;
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
        createInfo.enabledLayerCount = (uint32_t)_InvalidationLayers.size();             //系统允许的Layer层数量
        
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};//定义一个VkDebugUtilsMessengerCreateInfoEXT对象
        populateDebugMessengerCreateInfo(debugCreateInfo);                  //配置调试信息输出的相关设置

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
   

    void createLogicalDevice() 
    {
        //❶ 指定要创建的队列
        //创建逻辑设备需要VkDeviceQueueCreateInfo结构体，这一结构体描述了针对一个队列族我们所需的队列数量，
        // 目前我们使用带有图形能力的队列族和显示队列族。
        QueueFamilyIndices   indices = findQueueFamilies(physicalDevice);

        //我们需要一个队列列表，保存需要创建的逻辑设备一同创建的队列族类型
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        //从indices结构中检索出我们需要的队列族放到集合中
        std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

        //Vulkan需要我们赋予队列一个0.0到1.0之间的浮点数来控制队列优先级。即使只有一个队列，也要显式地赋予队列优先级：
        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) //根据集合中的队列信息设置VkDeviceQueueCreateInfo列表
        {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }



        //❷ 指定设备特性
        //我们要指定应用程序使用的设备特性，这些是在vkGetPhysicalDeviceFeatures查询出的设备支持的功能，例如几何着色器。
        //暂时先不填写，之后再回来填写：
  
        VkPhysicalDeviceProperties props = { 0 };

        bool supportDeviceProperties2 = false;
        vkGetPhysicalDeviceProperties(physicalDevice, &props);
        if (props.apiVersion >= VK_MAKE_VERSION(1, 1, 0)) {
            supportDeviceProperties2 = true;
        }

        // vertexAttributeInstanceRateDivisor
        VkPhysicalDeviceVertexAttributeDivisorFeatures  VertexAttributeDivisorFeatures = {};
        VertexAttributeDivisorFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_FEATURES;
        VertexAttributeDivisorFeatures.vertexAttributeInstanceRateDivisor = VK_TRUE;
        VertexAttributeDivisorFeatures.vertexAttributeInstanceRateZeroDivisor = VK_TRUE;
        VertexAttributeDivisorFeatures.pNext = nullptr;

        VkPhysicalDeviceVertexInputDynamicStateFeaturesEXT vertexInputDynamicStateFeature = {};
        vertexInputDynamicStateFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_INPUT_DYNAMIC_STATE_FEATURES_EXT,
            // link to customBorderColorFeature node
            vertexInputDynamicStateFeature.pNext = &VertexAttributeDivisorFeatures;

        // VK_EXT_custom_border_color feature
        VkPhysicalDeviceCustomBorderColorFeaturesEXT customBorderColorFeature = {};
        customBorderColorFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_FEATURES_EXT,
            // This is the last node
            customBorderColorFeature.pNext = &vertexInputDynamicStateFeature;


        // VK_EXT_subgroup_size_control feature
        VkPhysicalDeviceSubgroupSizeControlFeaturesEXT subgroupSizeControlFeature = {};
        subgroupSizeControlFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES_EXT,
            // link to customBorderColorFeature node
            subgroupSizeControlFeature.pNext = &customBorderColorFeature;


        // physical device feature 2
        VkPhysicalDeviceFeatures2 features2 = {};
        features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            features2.features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT };
        // link to subgroupSizeControlFeature node
        features2.pNext = &subgroupSizeControlFeature;


        // If VK_KHR_get_physical_device_properties2 is not supported, fallback to general feature settings
        VkPhysicalDeviceFeatures features = { 0 };

        // Query all above features
        if (supportDeviceProperties2)
        {
            vkGetPhysicalDeviceFeatures2(physicalDevice, &features2);
        }
        else {
            vkGetPhysicalDeviceFeatures(physicalDevice, &features);
        }


        // ❸ 创建逻辑设备
        VkDeviceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        //设置随逻辑设备创建时一同创建的队列族信息
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        //设备特征
		if (supportDeviceProperties2==true)
		{
			createInfo.pNext = &features2;
            createInfo.pEnabledFeatures = nullptr;
		}
		else
		{
			createInfo.pEnabledFeatures = &features;
			createInfo.pNext = nullptr;
		}
        

        //我们可以对设备和Vulkan实例使用相同地校验层，在检索物理设备时已经验证了需要的扩展
        createInfo.enabledExtensionCount = deviceExtensions.size();
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();
        
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else {
            createInfo.enabledLayerCount = 0;
       }


        //vkCreateDevice函数的参数包括要连接的物理设备、需要使用的队列、可选的分配器回调，以及用来存储逻辑设备句柄的指针。
        //逻辑设备并不直接与Vulkan实例交互，所以创建逻辑设备时不需要使用Vulkan实例作为参数。
        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicDevice) != VK_SUCCESS)
        {
            //与Vulkan实例对象的创建函数类似，在启用不存在的扩展或指定不支持的功能时，会返回错误码。
            throw std::runtime_error("failed to create logical device!");
        }
        //❹ 获取队列句柄 ----创建逻辑设备时指定的队列会同时被创建，为了方便，我们添加了一个VkQueue类成员变量来存储图形队列的句柄：
        //vkGetDeviceQueue获取指定队列族的队列句柄,参数依次是逻辑设备，队列族索引，队列索引和存储返回的队列句柄的指针。
        // 因为我们只创建了一个队列，所以可以直接使用索引0：
        vkGetDeviceQueue(logicDevice, indices.graphicsFamily.value(), 0, &graphicsQueue);
        //graphicsQueue逻辑设备相关联的队列句柄是随着physicalDevice创建自动生成的，他会会随着逻辑设备的销毁自动销毁，所以不需要在cleanup函数中进行队列的销毁操作
        // 获得显示队列住族句柄
        vkGetDeviceQueue(logicDevice, indices.presentFamily.value(), 0, &presentQueue);
    }


    bool isDeviceSuitable(VkPhysicalDevice _physicaldevice)
    {
        QueueFamilyIndices indices = findQueueFamilies(_physicaldevice);
        bool isPhysicalDeviceExtension = CheckPhysicalDeviceExtensionSupport(_physicaldevice,
            deviceExtensions);

        //对isDeviceSuitable函数进行补充，检测交换链的能力是否满足需求。这里只需要交换链至少支持一种图像格式和一种窗口表面的呈现模式即可：
        bool swapChainAdequate = false;
        if (isPhysicalDeviceExtension) 
        {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(_physicaldevice);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        return indices.isComplete() && isPhysicalDeviceExtension&& swapChainAdequate;
    }




protected:
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
protected:
    std::vector<const char*>  m_VkInstanceExtensions;
    std::vector<const char*>  m_VkValidationLayers;
    std::vector<VkExtensionProperties> m_VkDeviceExtensionPropertis;

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device)
    {
        SwapChainSupportDetails details;
        //❶ 调用下面的函数查询基础表面功能：
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
        //❷ 查询表面支持的格式。查询结果是一个结构体列表，函数调用2次，首先查询格式数量，然后分配数组保存结果：
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
        if (formatCount != 0)
        {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }
        //❸调用vkGetPhysicalDeviceSurfacePresentModesKHR查询支持的显示模式：
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }
        return details;
    }
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

        if (_enableValidationLayers) 
        {
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
        createInfo.flags = 0;
        createInfo.pfnUserCallback = debugCallback;
    }
    public:
        VkShaderModule createShaderModule(const std::vector<char>& code) {
            VkShaderModuleCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.codeSize = code.size();
            createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

            VkShaderModule shaderModule;
            if (vkCreateShaderModule(logicDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
                throw std::runtime_error("failed to create shader module!");
            }

            return shaderModule;
        }
        static std::vector<char> readFile(const std::string& filename) {
            std::ifstream file(filename, std::ios::ate | std::ios::binary);

            if (!file.is_open()) {
                throw std::runtime_error("failed to open file!");
            }

            size_t fileSize = (size_t)file.tellg();
            std::vector<char> buffer(fileSize);

            file.seekg(0);
            file.read(buffer.data(), fileSize);

            file.close();

            return buffer;
        }
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
    {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }
};
/*
class VulkanShaderModule
{
    VkShaderModule     shaderModule=nullptr;
    VkDevice           logicDevide = nullptr;
public:
    VulkanShaderModule() = default;
    ~VulkanShaderModule()
    {
        if (logicDevide != nullptr)
        {
            Destory(logicDevide);
            logicDevide = nullptr;
        }
    }
	VkShaderModule& GetShaderModule()
	{
		return shaderModule;
	}
    VkShaderModule& Initialize(VkDevice& _logicDevide, const std::string& _shaderFileName)
    {
       const std::vector<uint32_t> _shadeCode = LoadShader(_shaderFileName);

       return Initialize(_logicDevide, _shadeCode);
    }
    VkShaderModule& Initialize(VkDevice& _logicDevide, const std::vector<uint32_t>& _shaderCode)  
    {  
       logicDevide = _logicDevide;  
       VkShaderModuleCreateInfo createInfo{};  
       createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;  
       createInfo.codeSize = _shaderCode.size() * sizeof(uint32_t);  
       createInfo.pCode = _shaderCode.data();  

       if (vkCreateShaderModule(logicDevide, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)  
       {  
           throw std::runtime_error("failed to create shader module!");  
       }  
       return shaderModule;  
    }

    void Destory(VkDevice _logicDevide=nullptr)
    {
        if (_logicDevide != nullptr)
        {
			logicDevide = _logicDevide;
        }
        if (shaderModule != nullptr)
        {
            vkDestroyShaderModule(logicDevide, shaderModule, nullptr);
            shaderModule = nullptr;
        }
    }
    // 读取SPIR-V字节码文件
    std::vector<uint32_t> LoadShader(const std::string& filePath) 
    {
        std::ifstream file(filePath, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("failed to open shader file!");
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

        file.seekg(0);
        file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
        file.close();

        return buffer;
    }
    
};
template<typename T, size_t N = 2>
class VulkanVertexBuffer
{
    VkDevice          logicDevice = nullptr;
public:
	VkBuffer          vertexBuffer = nullptr;
	VkDeviceMemory    vertexBufferMemory = nullptr;
    int               bufferSize = 0;
	VulkanVertexBuffer() = default;
	~VulkanVertexBuffer()
	{
        Destory();
        logicDevice = nullptr;
	}

	void Initialize(VkPhysicalDevice& _physicalDevice,VkDevice& _logicDevice,const std::vector<T>&  _vertexData)
	{
		logicDevice = _logicDevice;
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = sizeof(_vertexData[0]) * _vertexData.size();
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(logicDevice, &bufferInfo, nullptr, &vertexBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create vertex buffer!");
        }
        bufferSize = bufferInfo.size;

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(logicDevice, vertexBuffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(_physicalDevice,memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (vkAllocateMemory(logicDevice, &allocInfo, nullptr, &vertexBufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate vertex buffer memory!");
        }

        vkBindBufferMemory(logicDevice, vertexBuffer, vertexBufferMemory, 0);

        void* data;
        vkMapMemory(logicDevice, vertexBufferMemory, 0, bufferInfo.size, 0, &data);
        memcpy(data, _vertexData.data(), (size_t)bufferInfo.size);
        vkUnmapMemory(logicDevice, vertexBufferMemory);
	}
    void Destory(VkDevice _logicDevide=nullptr)
    {
        if (_logicDevide != nullptr)
        {
            logicDevice = _logicDevide;
        }
        if (vertexBuffer != nullptr)
        {
            vkDestroyBuffer(logicDevice, vertexBuffer, nullptr);
			vertexBuffer = nullptr;
        } 
        if (vertexBufferMemory != nullptr)
        {
            vkFreeMemory(logicDevice, vertexBufferMemory, nullptr);
            vertexBufferMemory = nullptr;
        }
    }
    int GetVertexBufferSize()
    {
        return bufferSize;
    }

    VkVertexInputBindingDescription getBindingDescription()
    {
        return T::getBindingDescription();
    }

    std::array<VkVertexInputAttributeDescription, N> getAttributeDescriptions()
    {
        return T::getAttributeDescriptions();
    }
    //为要分配的buffer找到正确的合适的显存类型
    uint32_t findMemoryType(VkPhysicalDevice _physicalDevice,uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        //查询所有可用的内存类型
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(_physicalDevice, &memProperties);
        //找到buffer适合的内存类型
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }
};
*/
