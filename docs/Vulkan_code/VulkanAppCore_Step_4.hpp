#pragma once
//ͨ��GLFW_INCLUDE_VULKAN�궨�壬��glfw3.hͷ�ļ����Զ�����Vulkan/vulkan.hͷ�ļ�
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>

//ͨ������Ӧ����Ҫ�Ŀ��ļ�
#pragma comment( lib,"vulkan-1.lib" )
#pragma comment( lib,"glfw3.lib" )

#ifdef NDEBUG
inline const bool enableValidationLayers = false;
#else
inline  const bool enableValidationLayers = true;
#endif

//LunarG Vulkan SDK��������ͨ�� VK_LAYER_KHRONOS_validation ����ʽ�ؿ������п��õ�У���
inline const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

class VulkanAppCore 
{
protected:
    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;

    GLFWwindow* window = nullptr;

    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;
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
        //��ʼ��glfw��
        glfwInit();
        //��ʽ������GLFW����������Ҫ����OpenGL������
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        //�����ڴ�С�ı仯��Ҫ�ر�С�ģ����ǻ����Ժ����������ʱ�Ƚ�ֹ�ı䴰�ڴ�С
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void initVulkan() {
        createInstance();
        CreateVkDebugUtilsMessengerEXT();
        createSurface();

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
        {  //�����ҪУ��㣬��ϵͳ��ⲻ��У��㣬��Ҫ�˳�
            throw std::runtime_error("Ӧ�ó�����Ҫ����У���, �����Ǽ�ⲻ����Ҫ��У�飬�����˳�!");
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
        createInfo.enabledLayerCount = _InvalidationLayers.size();             //ϵͳ�����Layer������
        
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};//����һ��VkDebugUtilsMessengerCreateInfoEXT����
        populateDebugMessengerCreateInfo(debugCreateInfo);//��ɶ���ĵ�����Ϣ����

        if (createInfo.enabledLayerCount > 0)
        {
            createInfo.ppEnabledLayerNames = _InvalidationLayers.data();  //ָ��Lager�����������ָ�룬һ��Ϊ��ά����
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        }
        else
        {
            createInfo.ppEnabledLayerNames = nullptr;  //ָ��Lager�����������ָ�룬һ��Ϊ��ά����
            createInfo.pNext = nullptr;//һ�����������ָ��У���ṹ���ָ�룬��������Ϊnullptr
        }
        
        //����Vulkanʵ��
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
    {    //ͨ��glfwCreateWindowSurface��������surface����������ֱ�������ϵͳ�򽻵�
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create window surface!");
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


