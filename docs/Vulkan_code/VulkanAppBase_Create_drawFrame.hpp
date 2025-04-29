#pragma once
#include "VulkanAppCore.hpp"

#include <glm/glm.hpp>
#include <array>

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription = {};

        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        return attributeDescriptions;
    }

    static uint32_t getVertiesSize()
    {
        return sizeof(Vertex);
    }
};

const std::vector<Vertex> vertices = {
    {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
};


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

    VkPipelineLayout     pipelineLayout; //����VkPipeLine����ɫ���������ݵĲ�ṹLayout����
    VkPipeline           graphicsPipeline;//����VkPipeline����

    VkCommandPool                 commandPool;
    VkCommandBuffer               commandBuffer;
    VulkanVertexBuffer<Vertex,2>  vertexBufferObject;

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence     inFlightFence;

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
        createGraphicsPipeline();
        vertexBufferObject.Initialize(physicalDevice, logicDevice, vertices);
        createCommandPool();
        createCommandBuffer();

        createSyncObjects();

	}
    void mainLoop() {
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
            drawFrame();
        }
        vkDeviceWaitIdle(logicDevice);
    }
	void cleanup()
	{
        if (renderFinishedSemaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(logicDevice, renderFinishedSemaphore, nullptr);
            renderFinishedSemaphore = VK_NULL_HANDLE;
        }
        if (imageAvailableSemaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(logicDevice, imageAvailableSemaphore, nullptr);
            imageAvailableSemaphore = VK_NULL_HANDLE;
        }
        if (inFlightFence != VK_NULL_HANDLE)
        {
            vkDestroyFence(logicDevice, inFlightFence, nullptr);
            inFlightFence = VK_NULL_HANDLE;
        }

        vertexBufferObject.Destory();
        if (commandBuffer != VK_NULL_HANDLE)
        {
            vkFreeCommandBuffers(logicDevice, commandPool, 1, &commandBuffer);
            commandBuffer = VK_NULL_HANDLE;
        }

        if (commandPool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(logicDevice, commandPool, nullptr);
            commandPool = VK_NULL_HANDLE;
        }
        // ����ͼ�ιܵ�
        if (graphicsPipeline != nullptr)
        {
            vkDestroyPipeline(logicDevice, graphicsPipeline, nullptr);
            graphicsPipeline = nullptr;
        }
        // �ͷŹܵ�����
        if (pipelineLayout != nullptr)
        {
            vkDestroyPipelineLayout(logicDevice, pipelineLayout, nullptr);
            pipelineLayout = nullptr;
        }
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

    void drawFrame()
    {
        /* vkWaitForFences �������������ȴ�һ��դ�� (fence) �е�һ����
        ȫ��դ�� (fence) �����źš�������������Ƕ���ʹ�õ� VK_TRUE
        ��������ָ�����ȴ�������������ָ����դ�� (fence)����������ֻ��
        һ��դ�� (fence) ��Ҫ�ȴ������Բ�ʹ�� VK_TRUE Ҳ�ǿ��Եġ���
        vkAcquireNextImageKHR ����һ����vkWaitForFences ����Ҳ��һ����ʱ
        ���������ź�����ͬ���ȴ�դ�������źź�������Ҫ���� vkResetFences
        �����ֶ���դ�� (fence) ����Ϊδ�����źŵ�״̬��*/

        // �ȴ�ǰһ֡���                           // ����4���ȴ�����Fence��� // ��ʱ
        vkWaitForFences(logicDevice, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
        //// ��Fence����ΪUnsignaled
        vkResetFences(logicDevice, 1, &inFlightFence);
        /**	 vkAcquireNextImageKHR������
         1.ʹ�õ��߼��豸����
         2.����Ҫ��ȡͼ��Ľ�������
         3.ͼ���ȡ�ĳ�ʱʱ�䣬���ǿ���ͨ��ʹ���޷��� 64 λ�������ܱ�ʾ��
           �������������ͼ���ȡ��ʱ
         4,5.ָ��ͼ����ú�֪ͨ��ͬ������.����ָ��һ���ź��������դ������
           ����ͬʱָ���ź�����դ���������ͬ��������
           ���������ָ����һ������ imageAvailableSemaphore ���ź�������
         6.����������õĽ�����ͼ�������������ʹ������������������ǵ�
           swapChainImages�����е�VkImage���󣬲�ʹ����һ�������ύ��Ӧ��ָ��� */

           // �ӽ�������ȡͼ��
        uint32_t imageIndex;                        //��ʱʱ��   //ָ��ͼ����ú�֪ͨ��ͬ�����󣬿���ָ��һ���ź��������դ�����󣬻���ͬʱָ���ź�����դ���������ͬ������          
        VkResult result = vkAcquireNextImageKHR(logicDevice, swapChain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);//��ȡͼ�������ֵ

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            //��⵽���ڴ�С�ı䣬��Ҫ��Ӧ�޸Ĵ��ڴ�С�ı�Ĵ���
            return;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }


        //��λ�������
        vkResetCommandBuffer(commandBuffer, /*VkCommandBufferResetFlagBits*/ 0);
        recordCommandBuffer(commandBuffer, imageIndex);//��¼����

        //�����ύ��ͬ��ͨ��VkSubmitInfo�ṹ����в������á�
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        //Ϊ����ͼ��д����ɫ�����ǻ�ȴ�ͼ��״̬��Ϊavailable��
        // ������ָ��д����ɫ������ͼ�ι��߽׶�
        VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
        //������Ҫд����ɫ���ݵ�ͼ��,��������ָ���ȴ�ͼ����ߵ������д����ɫ���ŵĹ��߽׶�
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        /**		waitSemaphoreCount��pWaitSemaphores ��pWaitDstStageMask ��Ա��������ָ��
        ���п�ʼִ��ǰ��Ҫ�ȴ����ź������Լ���Ҫ�ȴ��Ĺ��߽׶� */
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        //ָ��ʵ�ʱ��ύִ�е�ָ������
       //����Ӧ���ύ�����Ǹոջ�ȡ�Ľ�����ͼ�����Ӧ��ָ������
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        //ָ����ָ���ִ�н����󷢳��źŵ��ź�������
        VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        /**   vkQueueSubmit ����ʹ��vkQueueSubmit�ṹ��������Ϊ����,����ͬʱ�������ύ��.��
       vkQueueSubmit ���������һ��������һ����ѡ��դ������
       ��������ͬ���ύ��ָ���ִ�н�����Ҫ���еĲ�����	 */
       //�ύ���������ͼ�ζ���
        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to submit draw command buffer!");
        }
        //���ž���Ҫ����Ⱦ��ͼ�񷵻ظ����������г��ֲ���
        //��ѯ����ָ������е������ύ��������������ʾ
        VkPresentInfoKHR presentInfo{}; //���ó�����Ϣ
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        //ָ����ʼ���ֲ�����Ҫ�ȴ����ź���
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        //ָ�������ڳ���ͼ��Ľ��������Լ���Ҫ���ֵ�ͼ���ڽ������е�����
        VkSwapchainKHR swapChains[] = { swapChain };
        presentInfo.swapchainCount = 1;//����������
        presentInfo.pSwapchains = swapChains;

        presentInfo.pImageIndices = &imageIndex;
        /**
        ���ǿ���ͨ�� pResults ��Ա������ȡÿ���������ĳ��ֲ����Ƿ�ɹ�
        ����Ϣ���������������ֻʹ����һ��������������ֱ��ʹ�ó��ֺ���
        �ķ���ֵ���жϳ��ֲ����Ƿ�ɹ�
          */
        presentInfo.pResults = nullptr;
        //���󽻻�������ͼ����ֲ���
        result = vkQueuePresentKHR(presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) 
        {
            //��⵽���ڴ�С�ı䣬��Ҫ��Ӧ�޸Ĵ��ڴ�С�ı�Ĵ���
        }
        else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        /**
        �������У�������г��򣬹۲�Ӧ�ó�����ڴ�ʹ�������
        ���Է������ǵ�Ӧ�ó�����ڴ�ʹ����һֱ���������ӡ�����������
        �ǵ� drawFrame �����Ժܿ���ٶ��ύָ���ȴû������һ��ָ��
        �ύʱ�����һ���ύ��ָ���Ƿ��Ѿ�ִ�н�����Ҳ����˵ CPU �ύ
        ָ���� GPU ��ָ��Ĵ����ٶȣ���� GPU ��Ҫ�����ָ�������
        ��������������������£�����ʵ���϶Զ��֡ͬʱʹ������ͬ��
        imageAvailableSemaphore �� renderFinishedSemaphore �ź�����
        ��򵥵Ľ��������һ����ķ�����ʹ�� vkQueueWaitIdle ��������
        ����һ���ύ��ָ�����ִ�У����ύ��һ֡��ָ�
        �����������Ƕ� GPU ������Դ�Ĵ���˷ѡ�ͼ�ι��߿��ܴ󲿷�ʱ
        �䶼���ڿ���״̬.
          */
          //�ȴ�һ���ض�ָ����н���ִ��
        vkQueueWaitIdle(presentQueue);
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

    void createGraphicsPipeline()
    {
        //����VkPipelineVertexInputStateCreateInfo�ṹ��������Ϣ
        // auto bindingDescription = vertexBufferObject.getBindingDescription();
        // auto attributeDescriptions = vertexBufferObject.getAttributeDescriptions();

        //	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        //	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        //	vertexInputInfo.vertexBindingDescriptionCount =  1; //0;
        //	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()); //0;
        //    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        //	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
         
        //����VkPipelineInputAssemblyStateCreateInfo�ṹ�ı������ñ���������θ������붥����������ͼԪ
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;//ͼԪ����
        inputAssembly.primitiveRestartEnable = VK_FALSE;
        
        //��ʼ����ɫ��ģ�����
        VulkanShaderModule  vertShaderModuleObject;
        VulkanShaderModule  fragShaderModuleObject;
        vertShaderModuleObject.Initialize(logicDevice, "shader/shader.vert.spv");
        fragShaderModuleObject.Initialize(logicDevice, "shader/shader.frag.spv");

        //����VkPipelineShaderStageCreateInfo�ṹ��������Ϣ
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;//������ɫ��
        vertShaderStageInfo.module = vertShaderModuleObject.GetShaderModule();
        vertShaderStageInfo.pName = "main";//��ں���

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};

        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;//Ƭ����ɫ��
        fragShaderStageInfo.module = fragShaderModuleObject.GetShaderModule();
        fragShaderStageInfo.pName = "main";//��ں���
        //���������ʽ���ݸ�pipelineInfo.pStages
        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        //�����ӿںͼ��з�Χ
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;//һ���ӿ�
        viewportState.scissorCount = 1;//һ�����з�Χ
        //ע�⣺����������û�������ӿںͼ��з�Χ�ľ���ֵ����ô�ڴ���pipelineInfo����ʱ�����˶�̬״̬
        //�ڻ��������������ӿںͼ��з�Χ��ֵ----������ǲ����ö�̬״̬������Ҫ�����������ӿںͼ��з�Χ��ֵ
        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)swapChainExtent.width;
        viewport.height = (float)swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        // 
        VkRect2D scissor = {};
        scissor.offset = { 0, 0 };
        scissor.extent = swapChainExtent;

        viewportState.pViewports = &viewport;
        viewportState.pScissors = &scissor;
        // 
        //��դ����Ϣ����--�Ӷ�����ɫ����ȡ�ɶ����γɵļ����壬������ת��ΪƬ�Σ��Ա���Ƭ����ɫ����ɫ
        //     ����ִ����Ȳ��ԣ����޳��ͱ�Ե���ԣ����ҿ�������Ϊ��������������λ������Ե��Ƭ��
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE; //VK_TRUE���򳬳���ƽ���Զƽ���Ƭ�ν������Ƶ������У������Ƕ�������
        rasterizer.rasterizerDiscardEnable = VK_FALSE; //VK_TRUE�����դ���׶ν������ã����к�����ͼ�ι��߽׶ζ����ᱻִ��
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;//���ģʽ  VK_POLYGON_MODE_FILL����Ƭ�������������
        //VK_POLYGON_MODE_LINE������α߻���Ϊ��
       //K_POLYGON_MODE_POINT������ζ������Ϊ��
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;//�޳�ģʽ VK_CULL_MODE_BACK_BIT���޳�����
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;//����˳�� VK_FRONT_FACE_CLOCKWISE��˳ʱ��
        rasterizer.depthBiasEnable = VK_FALSE;//�Ƿ��������ƫ��

        //���ز�������---���ز���(Multisampling)��һ�ֿ���ݼ��������ڼ���ͼ���еľ��״��Ե
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        //��ɫ�������---��ɫ���(Blending)�ǽ�Ƭ����ɫ���������ɫ��֡�����������е���ɫ������ϵĹ���
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;
        //���ö�̬״̬����--��̬״̬(Dynamic State)��������������ж�̬�ظı�ĳЩ����״̬���������ڴ�������ʱ��̬����������
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
            VK_DYNAMIC_STATE_LINE_WIDTH,
            VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY_EXT,//�󶨶�̬��Ϳ���ʹ��vkCmdSetPrimitiveTopology�������Ʋ�ͬ���͵�ͼԪ
            VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE_EXT,// �󶨶�̬��Ϳ���ʹ��vkCmdBindVertexBuffers2�������ö��㻺�����Ĳ���
            VK_DYNAMIC_STATE_VERTEX_INPUT_EXT,
        };

        VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
        dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicStateInfo.pDynamicStates = dynamicStates.data();
        dynamicStateInfo.flags = 0;

        //���ô���pipelineLayout����ʱ���õ������Ϣ��Ӧ�ó�����Ҫͨ��pipelineLayout��������Shader�е�uniform����������Ϣ
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;//��������ʹ�õ�Shaderû����Ҫ�����uniform��������������Ϊ0

        //pushConstant��Vulkan�е�һ�������uniform������������Ӧ�ó�������Ⱦ�����ж�̬�ش������ݵ���ɫ����
        pipelineLayoutInfo.pushConstantRangeCount = 0;//����Shader����ҪӦ�ó��򴫵ݵ�pushConstant�����ĸ���

        if (vkCreatePipelineLayout(logicDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;//stageCount��VkPipelineShaderStageCreateInfo�ṹ�壬��Щ�ṹ�������˱������ڸ�ͼ�ι����е�һ����ɫ���׶�
        pipelineInfo.pStages = shaderStages;//ָ����ɫ��ģ�������ָ��
        pipelineInfo.pVertexInputState = nullptr;// &vertexInputInfo;//����������Ϣ�����ṹ
        pipelineInfo.pInputAssemblyState = &inputAssembly;//������ζԶ�����л��ƣ�����ƶ��㡢�����߶Ρ�����������
        pipelineInfo.pViewportState = &viewportState;//�ӿںͼ��з�Χ
        pipelineInfo.pRasterizationState = &rasterizer;//��դ��
        pipelineInfo.pMultisampleState = &multisampling; //�ڹ�դ��������ʱ��ʹ�õĶ��ز���״̬��
        pipelineInfo.pColorBlendState = &colorBlending;//�ڹ�դ��������ʱ������Ⱦ�ڼ䱻���ʵ�������ɫ��������ɫ���״̬
        pipelineInfo.pDynamicState = &dynamicStateInfo;//���ô�����graphicsPipeline��������������ʱ�ڶ�̬�ı��״̬��Ŀ��
        pipelineInfo.pNext = nullptr;//û�ж����״̬��Ϣ
        pipelineInfo.layout = pipelineLayout;//����pipelineLayout���������õ�VulkanShaderModule��������Ҫ���ݵ�uniform����
        pipelineInfo.renderPass = renderPass;//����graphicsPipeline������ʹ�õ���Ⱦͨ�����󣬿������ΪFrameBuffer�ı仯����
        pipelineInfo.subpass = 0;//û�ж������ͨ������������Ϊ0
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;//ͨ������

        if (vkCreateGraphicsPipelines(logicDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }


        vertShaderModuleObject.Destory();
        fragShaderModuleObject.Destory();
    }

    void createCommandPool()
    {
        //�����ѡ�����豸�Ƿ�֧��ͼ�ζ��������ʾ������
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

        //����VkCommandPoolCreateInfo�ṹ��
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

        //VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT��־��ʾ��������е�����������Ա�����,
        //VK_COMMAND_POOL_CREATE_TRANSIENT_BIT��־��ʾ��������е���������Ƕ��ݵģ��ʺ�����ʱʹ��,��ʾ��������ǳ�Ƶ�������¼�¼������(���ܻ�ı��ڴ������Ϊ)
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;//�ñ�־��ʾ������commandBuffer����ᱻcommandPool�������Զ�����ͷ�
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();//ͼ�ζ����������
        //����commandPool����
        if (vkCreateCommandPool(logicDevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool!");
        }
    }

    //��commandPool�ϴ��������������
    void createCommandBuffer()
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;//���������
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(logicDevice, &allocInfo, &commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = swapChainExtent;

        VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        //���û��ƹ��������õ�VK_DYNAMIC_STATE_VERTEX_INPUT_EXT��̬״̬
        //vkCmdBindVertexBuffers2(commandBuffer, 0, 1, vertexBuffers, offsets, vertexBufferObject.GetVertexBufferStride(), 0);
        auto vkCmdSetVertexInputEXT_Fun = (PFN_vkCmdSetVertexInputEXT)vkGetDeviceProcAddr(logicDevice, "vkCmdSetVertexInputEXT");
        if (vkCmdSetVertexInputEXT_Fun != nullptr)
        {
            auto bindingDescription = vertexBufferObject.getBindingDescription();
            auto attributeDescriptions = vertexBufferObject.getAttributeDescriptions();

            VkVertexInputBindingDescription2EXT  bindingDescription2EXT;
            bindingDescription2EXT.sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT;
            bindingDescription2EXT.pNext = nullptr;
            bindingDescription2EXT.binding = bindingDescription.binding;
            bindingDescription2EXT.stride = bindingDescription.stride;
            bindingDescription2EXT.inputRate = bindingDescription.inputRate;
            bindingDescription2EXT.divisor = 1;


            std::array<VkVertexInputAttributeDescription2EXT, 2> attributeDescriptions2EXT = {};
            attributeDescriptions2EXT[0].sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT;
            //VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT
            attributeDescriptions2EXT[0].pNext = nullptr;
            attributeDescriptions2EXT[0].binding = 0;
            attributeDescriptions2EXT[0].location = 0;
            attributeDescriptions2EXT[0].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions2EXT[0].offset = offsetof(Vertex, pos);

            attributeDescriptions2EXT[1].sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT;
            attributeDescriptions2EXT[1].pNext = nullptr;
            attributeDescriptions2EXT[1].binding = 0;
            attributeDescriptions2EXT[1].location = 1;
            attributeDescriptions2EXT[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions2EXT[1].offset = offsetof(Vertex, color);



            vkCmdSetVertexInputEXT_Fun(commandBuffer,//
                1, &bindingDescription2EXT,
                2, attributeDescriptions2EXT.data());

        }
        else
        {
            std::cout << "vkCmdSetVertexInputEXT_Fun is nullptr" << std::endl;

        }

        //�����ô�����Ⱦ����ʱ����VK_DYNAMIC_STATE_VIEWPORT��̬״̬
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapChainExtent.width);
        viewport.height = static_cast<float>(swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        //�����ô�����Ⱦ����ʱ����VK_DYNAMIC_STATE_SCISSOR��̬״̬
        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = swapChainExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        VkBuffer         vertexBuffers[] = { vertexBufferObject.vertexBuffer };
        VkDeviceSize     offsets[] = { 0 };

        //ͨ�����ù����еĶ�̬ͼԪ����---������VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY_EXT
        vkCmdSetPrimitiveTopology(commandBuffer, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        //ͨ�����ù����еĶ�̬�������ݺ�ƫ����	
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        //������VK_DYNAMIC_STATE_LINE_WIDTH��̬����״̬
        vkCmdSetLineWidth(commandBuffer, 2.0f);

        vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertexBufferObject.GetVertexBufferSize()), 1, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }
    void createSyncObjects() {
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if (vkCreateSemaphore(logicDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(logicDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS ||
            vkCreateFence(logicDevice, &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }

    }
};
