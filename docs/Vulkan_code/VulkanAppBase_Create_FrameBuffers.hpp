#pragma once
#include "VulkanAppCore.hpp"

#include <glm/glm.hpp>
#include <array>

class VulkanAppBase : public VulkanAppCore
{
protected:
	VkSwapchainKHR swapChain = VK_NULL_HANDLE;  //����������
	std::vector<VkImage> swapChainImages;       //�������п��Թ�����ʹ�õ�ͼ��
	VkFormat swapChainImageFormat;              //��������ͼ��ĸ�ʽ
	VkExtent2D swapChainExtent;                 //��������ͼ����Ϣ����չ��һ����ͼ��Ĵ�С�����ڵĴ�С��

	std::vector<VkImageView> swapChainImageViews; //�뽻������ͼ��һһ��Ӧ��ͼ�����ͼ���󣬳���ֻ��ͨ��VkImageView������ܷ���VkImage����
    VkRenderPass renderPass;

    std::vector<VkFramebuffer> swapChainFramebuffers;//��������ͼ���֡�������֡�����������Ⱦ������Ŀ����󣬳���ֻ��ͨ��VkFramebuffer������ܷ���VkImageView����

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
		//�����к�����ӳ�ʼ������
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

		//������ǰ��������ٴ���
		VulkanAppCore::cleanup();
	}
protected:
    void createSwapChain()
    {
        //�������Ĵ���������Ҫ��ѯ��������֧����Ϣ��querySwapChainSupport��������ǰ���Ѿ�ʵ����
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

        //���chooseSwapSurfaceFormat������ѡ����ʵı����ʽ
        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        //��ʾģʽ  ֻ��VK_PRESENT_MODE_FIFO_KHR��֤һ������
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        //������Χ
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        //���ý������е�ͼ�������Ҳ���ǽ������Ķ��п������ɵ�ͼ�����
        //������֧�ֵ���Сͼ�����+1����ʵ�����ػ��� ;maxImageCountΪ0������ֻҪ�ڴ��㹻�����ǿ���ʹ������������ͼ��
        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
        {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }
        //��������Ҫ��дһ������������Ϣ�Ľṹ��,ϣ�����ܹ������ܵ���ʶ�������
        VkSwapchainCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;//������ʹ�õ���ʵ�豸������suiface

        createInfo.minImageCount = imageCount;//����������Ƶ�ͼ������
        createInfo.imageFormat = surfaceFormat.format;//ͼ��ĸ�ʽ
        createInfo.imageColorSpace = surfaceFormat.colorSpace;//��ɫ��ʽ
        createInfo.imageExtent = extent;//��ʾͼ��Ĵ�С(���ڴ�С)
        createInfo.imageArrayLayers = 1; //ÿ��ͼ������ж�㣬��ʱ����Ϊ1��ֻʹ��һ�㣻
        //imageUsageλ�ֶ�ָ���ڽ������ж�ͼ����еľ������
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;//VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT��ʾ������Ϊ��ȾĿ�꣬�����ǽ�ֱ�Ӷ����ǽ�����Ⱦ��

        //findQueueFamilies����ǰ���Ѿ����������ڲ�ѯУ���豸�Ƿ�֧��ͼ�ζ��������ʾ������
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        uint32_t queueFamilyIndices[] = { (uint32_t)indices.graphicsFamily.value(), (uint32_t)indices.presentFamily.value() };
        //����ͼ�ζ���������ʾ����ס�Ƿ���ͬ������ͼ��Image����Ĺ���ģʽ
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
        //currentTransform���Ա�ʾ��ǰ��Ļ�ı任��ʽ������Ļ����ת������״̬��
        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;////֧�ֵĸ��� alpha ģʽ����
        createInfo.presentMode = presentMode;//��������ʾģʽ
        createInfo.clipped = VK_TRUE;//ָ���Ƿ���������ͼ���ڳ���ʱ���ü�

        createInfo.oldSwapchain = VK_NULL_HANDLE;//����������´�������������˳�Աָ��Ҫ�滻�ľɽ������ľ��

        if (vkCreateSwapchainKHR(logicDevice, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }
        //������ɺ�������Ҫ��ý�������ͼ��Image������
        vkGetSwapchainImagesKHR(logicDevice, swapChain, &imageCount, nullptr);
        //����������ͼ��Image���������swapChainImages������
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(logicDevice, swapChain, &imageCount, swapChainImages.data());
        //��ý�������ͼ���ʽ�ʹ�С��Ϣ���������������ٺ�������Ⱦ�����л��õ�
        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;

    }
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
    {
        //VkSurfaceFormatKHR�ṹ�а���format��Ա��ʾ���ڱ���֧�ֵ���ɫ��ʽ��VK_FORMAT_B8G8R8A8_UNORM��ʾ������B��G��R��A��˳��ÿ����ɫͨ����8λ�޷�����������ÿ�����ܹ�ʹ��32λ��ʾ
        //                            colorSpace��Ա���ڱ����Ƿ�֧��SRGB��ɫ�ռ�(����VK_COLOR_SPACE_SRGB_NONLINEAR_KHR���λ)
        //������ɫ�ռ䣬����ʹ��SRGB�����Ǳ�׼��ɫ�ռ䣬ʹ�������Եõ�����׼ȷ����ɫ��ʾ��һ�ֳ�����sRGB ��ɫ��ʽ��VK_FORMAT_B8G8R8A8_SRGB��
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
            //����ѡ��VK_PRESENT_MODE_MAILBOX_KHRģʽ�������Ա���ͼ��˺��
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return availablePresentMode;
            }
        }
        //Ĭ��ѡ��VK_PRESENT_MODE_FIFO_KHRģʽ�������ȫ��ģʽ
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
            framebufferInfo.renderPass = renderPass;//��Ӧ����Ⱦͨ������
            framebufferInfo.attachmentCount = 1;//��Ӧһ���������е�ImageView����
            framebufferInfo.pAttachments = attachments;//��������ͼ�����ͼ����
            framebufferInfo.width = swapChainExtent.width;//��������ͼ��Ŀ��
            framebufferInfo.height = swapChainExtent.height;//��������ͼ��ĸ߶�
            framebufferInfo.layers = 1;//��������ͼ��Ĳ���

            if (vkCreateFramebuffer(logicDevice, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }
};
