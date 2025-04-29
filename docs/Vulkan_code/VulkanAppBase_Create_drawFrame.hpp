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
	VkSwapchainKHR swapChain = VK_NULL_HANDLE;  //交换链对象
	std::vector<VkImage> swapChainImages;       //交换链中可以供程序使用的图像
	VkFormat swapChainImageFormat;              //交换链中图像的格式
	VkExtent2D swapChainExtent;                 //交换链中图像信息的扩展，一般是图像的大小（窗口的大小）

	std::vector<VkImageView> swapChainImageViews; //与交换链中图像一一对应的图像的视图对象，程序只有通过VkImageView对象才能访问VkImage对象
    VkRenderPass renderPass;

    std::vector<VkFramebuffer> swapChainFramebuffers;//交换链中图像的帧缓冲对象，帧缓冲对象是渲染操作的目标对象，程序只能通过VkFramebuffer对象才能访问VkImageView对象

    VkPipelineLayout     pipelineLayout; //定义VkPipeLine中着色器顶点数据的层结构Layout对象
    VkPipeline           graphicsPipeline;//定义VkPipeline对象

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
		//在这行后面添加初始化代码
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
        // 销毁图形管道
        if (graphicsPipeline != nullptr)
        {
            vkDestroyPipeline(logicDevice, graphicsPipeline, nullptr);
            graphicsPipeline = nullptr;
        }
        // 释放管道布局
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

		//在这行前面添加销毁代码
		VulkanAppCore::cleanup();
	}

    void drawFrame()
    {
        /* vkWaitForFences 函数可以用来等待一组栅栏 (fence) 中的一个或
        全部栅栏 (fence) 发出信号。上面代码中我们对它使用的 VK_TRUE
        参数用来指定它等待所有在数组中指定的栅栏 (fence)。我们现在只有
        一个栅栏 (fence) 需要等待，所以不使用 VK_TRUE 也是可以的。和
        vkAcquireNextImageKHR 函数一样，vkWaitForFences 函数也有一个超时
        参数。和信号量不同，等待栅栏发出信号后，我们需要调用 vkResetFences
        函数手动将栅栏 (fence) 重置为未发出信号的状态。*/

        // 等待前一帧完成                           // 参数4：等待所有Fence完成 // 超时
        vkWaitForFences(logicDevice, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
        //// 将Fence重置为Unsignaled
        vkResetFences(logicDevice, 1, &inFlightFence);
        /**	 vkAcquireNextImageKHR参数：
         1.使用的逻辑设备对象
         2.我们要获取图像的交换链，
         3.图像获取的超时时间，我们可以通过使用无符号 64 位整型所能表示的
           最大整数来禁用图像获取超时
         4,5.指定图像可用后通知的同步对象.可以指定一个信号量对象或栅栏对象，
           或是同时指定信号量和栅栏对象进行同步操作。
           在这里，我们指定了一个叫做 imageAvailableSemaphore 的信号量对象
         6.用于输出可用的交换链图像的索引，我们使用这个索引来引用我们的
           swapChainImages数组中的VkImage对象，并使用这一索引来提交对应的指令缓冲 */

           // 从交换链获取图像
        uint32_t imageIndex;                        //超时时间   //指定图像可用后通知的同步对象，可以指定一个信号量对象或栅栏对象，或是同时指定信号量和栅栏对象进行同步操作          
        VkResult result = vkAcquireNextImageKHR(logicDevice, swapChain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);//获取图像的索引值

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            //检测到窗口大小改变，需要响应修改窗口大小改变的代码
            return;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }


        //复位命令缓冲区
        vkResetCommandBuffer(commandBuffer, /*VkCommandBufferResetFlagBits*/ 0);
        recordCommandBuffer(commandBuffer, imageIndex);//记录命令

        //队列提交和同步通过VkSubmitInfo结构体进行参数配置。
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        //为了向图像写入颜色，我们会等待图像状态变为available，
        // 所我们指定写入颜色附件的图形管线阶段
        VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
        //这里需要写入颜色数据到图像,所以我们指定等待图像管线到达可以写入颜色附着的管线阶段
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        /**		waitSemaphoreCount、pWaitSemaphores 和pWaitDstStageMask 成员变量用于指定
        队列开始执行前需要等待的信号量，以及需要等待的管线阶段 */
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        //指定实际被提交执行的指令缓冲对象
       //我们应该提交和我们刚刚获取的交换链图像相对应的指令缓冲对象
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        //指定在指令缓冲执行结束后发出信号的信号量对象
        VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        /**   vkQueueSubmit 函数使用vkQueueSubmit结构体数组作为参数,可以同时大批量提交数.。
       vkQueueSubmit 函数的最后一个参数是一个可选的栅栏对象，
       可以用它同步提交的指令缓冲执行结束后要进行的操作。	 */
       //提交命令缓冲区到图形队列
        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to submit draw command buffer!");
        }
        //接着就需要将渲染的图像返回给交换链进行呈现操作
        //查询绘制指令队列中的命令提交给交换链进行显示
        VkPresentInfoKHR presentInfo{}; //配置呈现信息
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        //指定开始呈现操作需要等待的信号量
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        //指定了用于呈现图像的交换链，以及需要呈现的图像在交换链中的索引
        VkSwapchainKHR swapChains[] = { swapChain };
        presentInfo.swapchainCount = 1;//交换链索引
        presentInfo.pSwapchains = swapChains;

        presentInfo.pImageIndices = &imageIndex;
        /**
        我们可以通过 pResults 成员变量获取每个交换链的呈现操作是否成功
        的信息。在这里，由于我们只使用了一个交换链，可以直接使用呈现函数
        的返回值来判断呈现操作是否成功
          */
        presentInfo.pResults = nullptr;
        //请求交换链进行图像呈现操作
        result = vkQueuePresentKHR(presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) 
        {
            //检测到窗口大小改变，需要响应修改窗口大小改变的代码
        }
        else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        /**
        如果开启校验层后运行程序，观察应用程序的内存使用情况，
        可以发现我们的应用程序的内存使用量一直在慢慢增加。这是由于我
        们的 drawFrame 函数以很快地速度提交指令，但却没有在下一次指令
        提交时检查上一次提交的指令是否已经执行结束。也就是说 CPU 提交
        指令快过 GPU 对指令的处理速度，造成 GPU 需要处理的指令大量堆
        积。更糟糕的是这种情况下，我们实际上对多个帧同时使用了相同的
        imageAvailableSemaphore 和 renderFinishedSemaphore 信号量。
        最简单的解决上面这一问题的方法是使用 vkQueueWaitIdle 函数来等
        待上一次提交的指令结束执行，再提交下一帧的指令：
        但这样做，是对 GPU 计算资源的大大浪费。图形管线可能大部分时
        间都处于空闲状态.
          */
          //等待一个特定指令队列结束执行
        vkQueueWaitIdle(presentQueue);
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

    void createGraphicsPipeline()
    {
        //设置VkPipelineVertexInputStateCreateInfo结构体的相关信息
        // auto bindingDescription = vertexBufferObject.getBindingDescription();
        // auto attributeDescriptions = vertexBufferObject.getAttributeDescriptions();

        //	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        //	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        //	vertexInputInfo.vertexBindingDescriptionCount =  1; //0;
        //	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()); //0;
        //    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        //	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
         
        //设置VkPipelineInputAssemblyStateCreateInfo结构的变量，该变量决定如何根据输入顶点数据生成图元
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;//图元类型
        inputAssembly.primitiveRestartEnable = VK_FALSE;
        
        //初始化着色器模块对象
        VulkanShaderModule  vertShaderModuleObject;
        VulkanShaderModule  fragShaderModuleObject;
        vertShaderModuleObject.Initialize(logicDevice, "shader/shader.vert.spv");
        fragShaderModuleObject.Initialize(logicDevice, "shader/shader.frag.spv");

        //设置VkPipelineShaderStageCreateInfo结构体的相关信息
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;//顶点着色器
        vertShaderStageInfo.module = vertShaderModuleObject.GetShaderModule();
        vertShaderStageInfo.pName = "main";//入口函数

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};

        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;//片段着色器
        fragShaderStageInfo.module = fragShaderModuleObject.GetShaderModule();
        fragShaderStageInfo.pName = "main";//入口函数
        //以数组的形式传递给pipelineInfo.pStages
        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        //设置视口和剪切范围
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;//一个视口
        viewportState.scissorCount = 1;//一个剪切范围
        //注意：本例中我们没有设置视口和剪切范围的具体值，我么在创建pipelineInfo对象时设置了动态状态
        //在绘制命令中设置视口和剪切范围的值----如果我们不设置动态状态，则需要在这里设置视口和剪切范围的值
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
        //光栅化信息配置--从顶点着色器获取由顶点形成的几何体，并将其转换为片段，以便由片段着色器着色
        //     它还执行深度测试，面剔除和边缘测试，并且可以配置为输出填充整个多边形或仅填充边缘的片段
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE; //VK_TRUE，则超出近平面和远平面的片段将被限制到它们中，而不是丢弃它们
        rasterizer.rasterizerDiscardEnable = VK_FALSE; //VK_TRUE，则光栅化阶段将被禁用，所有后续的图形管线阶段都不会被执行
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;//填充模式  VK_POLYGON_MODE_FILL：用片段填充多边形区域
        //VK_POLYGON_MODE_LINE：多边形边绘制为线
       //K_POLYGON_MODE_POINT：多边形顶点绘制为点
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;//剔除模式 VK_CULL_MODE_BACK_BIT：剔除背面
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;//顶点顺序 VK_FRONT_FACE_CLOCKWISE：顺时针
        rasterizer.depthBiasEnable = VK_FALSE;//是否启用深度偏移

        //多重采样配置---多重采样(Multisampling)是一种抗锯齿技术，用于减少图像中的锯齿状边缘
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        //颜色混合配置---颜色混合(Blending)是将片段着色器输出的颜色与帧缓冲区中已有的颜色进行组合的过程
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
        //设置动态状态配置--动态状态(Dynamic State)允许在命令缓冲区中动态地改变某些管线状态，而不是在创建管线时静态地设置它们
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
            VK_DYNAMIC_STATE_LINE_WIDTH,
            VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY_EXT,//绑定动态后就可以使用vkCmdSetPrimitiveTopology函数绘制不同类型的图元
            VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE_EXT,// 绑定动态后就可以使用vkCmdBindVertexBuffers2函数设置顶点缓冲区的步幅
            VK_DYNAMIC_STATE_VERTEX_INPUT_EXT,
        };

        VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
        dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicStateInfo.pDynamicStates = dynamicStates.data();
        dynamicStateInfo.flags = 0;

        //设置创建pipelineLayout对象时设置的相关信息，应用程序主要通过pipelineLayout的设置与Shader中的uniform变量交换信息
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;//我们这里使用的Shader没有需要传输的uniform变量，所以设置为0

        //pushConstant是Vulkan中的一种特殊的uniform变量，它允许应用程序在渲染过程中动态地传递数据到着色器中
        pipelineLayoutInfo.pushConstantRangeCount = 0;//设置Shader中需要应用程序传递的pushConstant变量的个数

        if (vkCreatePipelineLayout(logicDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;//stageCount个VkPipelineShaderStageCreateInfo结构体，这些结构体描述了被包含在该图形管线中的一组着色器阶段
        pipelineInfo.pStages = shaderStages;//指向着色器模块数组的指针
        pipelineInfo.pVertexInputState = nullptr;// &vertexInputInfo;//顶点输入信息描述结构
        pipelineInfo.pInputAssemblyState = &inputAssembly;//决定如何对顶点进行绘制，如绘制顶点、绘制线段、绘制三角形
        pipelineInfo.pViewportState = &viewportState;//视口和剪切范围
        pipelineInfo.pRasterizationState = &rasterizer;//光栅化
        pipelineInfo.pMultisampleState = &multisampling; //在光栅化被启用时所使用的多重采样状态。
        pipelineInfo.pColorBlendState = &colorBlending;//在光栅化被启用时，在渲染期间被访问的任意颜色附件的颜色混合状态
        pipelineInfo.pDynamicState = &dynamicStateInfo;//设置创建的graphicsPipeline对象允许在运行时期动态改变的状态项目，
        pipelineInfo.pNext = nullptr;//没有额外的状态信息
        pipelineInfo.layout = pipelineLayout;//设置pipelineLayout对象中引用的VulkanShaderModule对象中需要传递的uniform变量
        pipelineInfo.renderPass = renderPass;//设置graphicsPipeline对象所使用的渲染通道对象，可以理解为FrameBuffer的变化规则
        pipelineInfo.subpass = 0;//没有额外的子通道，所以设置为0
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;//通常不用

        if (vkCreateGraphicsPipelines(logicDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }


        vertShaderModuleObject.Destory();
        fragShaderModuleObject.Destory();
    }

    void createCommandPool()
    {
        //检测所选物理设备是否支持图形队列族和显示队列族
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

        //构建VkCommandPoolCreateInfo结构体
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

        //VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT标志表示该命令池中的命令缓冲区可以被重置,
        //VK_COMMAND_POOL_CREATE_TRANSIENT_BIT标志表示该命令池中的命令缓冲区是短暂的，适合于临时使用,提示命令缓冲区非常频繁的重新记录新命令(可能会改变内存分配行为)
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;//该标志表示创建的commandBuffer对象会被commandPool管理，会自动完成释放
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();//图形队列族的索引
        //创建commandPool对象
        if (vkCreateCommandPool(logicDevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool!");
        }
    }

    //在commandPool上创建命令缓冲区对象
    void createCommandBuffer()
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;//主命令缓冲区
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

        //设置绘制管线中设置的VK_DYNAMIC_STATE_VERTEX_INPUT_EXT动态状态
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

        //需设置创建渲染管线时设置VK_DYNAMIC_STATE_VIEWPORT动态状态
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapChainExtent.width);
        viewport.height = static_cast<float>(swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        //需设置创建渲染管线时设置VK_DYNAMIC_STATE_SCISSOR动态状态
        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = swapChainExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        VkBuffer         vertexBuffers[] = { vertexBufferObject.vertexBuffer };
        VkDeviceSize     offsets[] = { 0 };

        //通过设置管线中的动态图元类型---需设置VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY_EXT
        vkCmdSetPrimitiveTopology(commandBuffer, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        //通过设置管线中的动态顶点数据和偏移量	
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        //需设置VK_DYNAMIC_STATE_LINE_WIDTH动态管线状态
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
