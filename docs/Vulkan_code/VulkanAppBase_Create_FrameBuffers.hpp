#pragma once
#include "VulkanAppCore.hpp"

#include <glm/glm.hpp>
#include <array>

class VulkanAppBase : public VulkanAppCore
{
protected:
	VkSwapchainKHR swapChain = VK_NULL_HANDLE;  //交换链对象
	std::vector<VkImage> swapChainImages;       //交换链中可以供程序使用的图像
	VkFormat swapChainImageFormat;              //交换链中图像的格式
	VkExtent2D swapChainExtent;                 //交换链中图像信息的扩展，一般是图像的大小（窗口的大小）

	std::vector<VkImageView> swapChainImageViews; //与交换链中图像一一对应的图像的视图对象，程序只有通过VkImageView对象才能访问VkImage对象
    VkRenderPass renderPass;

    std::vector<VkFramebuffer> swapChainFramebuffers;//交换链中图像的帧缓冲对象，帧缓冲对象是渲染操作的目标对象，程序只能通过VkFramebuffer对象才能访问VkImageView对象

public:
	void run()
	{
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}
	void initVulkan()
	{
		VulkanAppCore::initVulkan();
		//在这行后面添加初始化代码
        createSwapChain();
        createImageViews();
        createRenderPass();
        createFramebuffers();

	}

	void cleanup()
	{
        for (auto framebuffer : swapChainFramebuffers) {
            vkDestroyFramebuffer(logicDevice, framebuffer, nullptr);
        }

        if (renderPass != VK_NULL_HANDLE)
        {
            vkDestroyRenderPass(logicDevice, renderPass, nullptr);
            renderPass = VK_NULL_HANDLE;
        }
        for (auto imageView : swapChainImageViews)
        {
            vkDestroyImageView(logicDevice, imageView, nullptr);
        }
        if (swapChain != nullptr)
        {
            vkDestroySwapchainKHR(logicDevice, swapChain, nullptr);
            swapChain = nullptr;
        }

		//在这行前面添加销毁代码
		VulkanAppCore::cleanup();
	}
protected:
    void createSwapChain()
    {
        //交换链的创建首先需要查询交换链的支撑信息，querySwapChainSupport函数我们前面已经实现了
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

        //添加chooseSwapSurfaceFormat函数来选择合适的表面格式
        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        //显示模式  只有VK_PRESENT_MODE_FIFO_KHR保证一定可用
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        //交换范围
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        //设置交换链中的图像个数，也就是交换链的队列可以容纳的图像个数
        //交换链支持的最小图像个数+1，来实现三重缓冲 ;maxImageCount为0表明，只要内存足够，我们可以使用任意数量的图像
        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
        {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }
        //交换链需要填写一个包含大量信息的结构体,希望你能够尽可能的认识相关内容
        VkSwapchainCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;//交换链使用的现实设备关联的suiface

        createInfo.minImageCount = imageCount;//交换链中设计的图像数量
        createInfo.imageFormat = surfaceFormat.format;//图像的格式
        createInfo.imageColorSpace = surfaceFormat.colorSpace;//颜色格式
        createInfo.imageExtent = extent;//显示图像的大小(窗口大小)
        createInfo.imageArrayLayers = 1; //每张图像可以有多层，暂时设置为1，只使用一层；
        //imageUsage位字段指定在交换链中对图像进行的具体操作
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;//VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT表示用于作为渲染目标，即我们将直接对它们进行渲染。

        //findQueueFamilies函数前面已经讲过，用于查询校验设备是否支持图形队列族和显示队列族
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        uint32_t queueFamilyIndices[] = { (uint32_t)indices.graphicsFamily.value(), (uint32_t)indices.presentFamily.value() };
        //根据图形队列族与显示队列住是否相同来设置图像Image对象的共享模式
        if (indices.graphicsFamily != indices.presentFamily)
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0; // Optional
            createInfo.pQueueFamilyIndices = nullptr; // Optional
        }
        //currentTransform属性表示当前屏幕的变换方式，即屏幕的旋转和缩放状态。
        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;////支持的复合 alpha 模式集合
        createInfo.presentMode = presentMode;//交换链显示模式
        createInfo.clipped = VK_TRUE;//指定是否允许交换链图像在呈现时被裁剪

        createInfo.oldSwapchain = VK_NULL_HANDLE;//如果正在重新创建交换链，则此成员指定要替换的旧交换链的句柄

        if (vkCreateSwapchainKHR(logicDevice, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }
        //创建完成后，我们需要获得交换链中图像Image的数量
        vkGetSwapchainImagesKHR(logicDevice, swapChain, &imageCount, nullptr);
        //将交换链中图形Image句柄保存在swapChainImages数组中
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(logicDevice, swapChain, &imageCount, swapChainImages.data());
        //获得交换链中图像格式和大小信息保存起来，我们再后续的渲染操作中会用到
        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;

    }
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
    {
        //VkSurfaceFormatKHR结构中包含format成员表示窗口表面支持的颜色格式，VK_FORMAT_B8G8R8A8_UNORM表示我们以B，G，R和A的顺序，每个颜色通道用8位无符号整型数，每像素总共使用32位表示
        //                            colorSpace成员窗口表面是否支持SRGB颜色空间(包含VK_COLOR_SPACE_SRGB_NONLINEAR_KHR标记位)
        //对于颜色空间，我们使用SRGB，它是标准颜色空间，使用它可以得到更加准确的颜色表示，一种常见的sRGB 颜色格式是VK_FORMAT_B8G8R8A8_SRGB。
        for (const auto& availableFormat : availableFormats)
        {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes)
    {
        for (const auto& availablePresentMode : availablePresentModes)
        {
            //优先选择VK_PRESENT_MODE_MAILBOX_KHR模式，他可以避免图像撕裂
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return availablePresentMode;
            }
        }
        //默认选择VK_PRESENT_MODE_FIFO_KHR模式，他是最安全的模式
        return VK_PRESENT_MODE_FIFO_KHR;
    }
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }
        else
        {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };
            if (actualExtent.width < capabilities.minImageExtent.width)
                actualExtent.width = capabilities.minImageExtent.width;
            else if (actualExtent.width > capabilities.maxImageExtent.width)
                actualExtent.width = capabilities.maxImageExtent.width;

            if (actualExtent.height < capabilities.minImageExtent.height)
                actualExtent.height = capabilities.minImageExtent.height;
            else if (actualExtent.height > capabilities.maxImageExtent.height)
                actualExtent.height = capabilities.maxImageExtent.height;


            return actualExtent;
        }
    }

    void createImageViews()
    {
        swapChainImageViews.resize(swapChainImages.size());
        for (size_t i = 0; i < swapChainImages.size(); i++)
        {
            VkImageViewCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapChainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = swapChainImageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;
            if (vkCreateImageView(logicDevice, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create image views!");
            }
        }
    }
    void createRenderPass() {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(logicDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
    }
    void createFramebuffers()
    {
        swapChainFramebuffers.resize(swapChainImageViews.size());

        for (size_t i = 0; i < swapChainImageViews.size(); i++)
        {
            VkImageView attachments[] = {
                swapChainImageViews[i]
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;//对应的渲染通道对象
            framebufferInfo.attachmentCount = 1;//对应一个交换链中的ImageView对象
            framebufferInfo.pAttachments = attachments;//交换链中图像的视图对象
            framebufferInfo.width = swapChainExtent.width;//交换链中图像的宽度
            framebufferInfo.height = swapChainExtent.height;//交换链中图像的高度
            framebufferInfo.layers = 1;//交换链中图像的层数

            if (vkCreateFramebuffer(logicDevice, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }
};
