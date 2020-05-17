#include "TriangleApplication.h"
#include "root_directory.h"

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

/* --- Helper --- */

/* implicitly enables a whole range of useful diagnostics layers */
const std::vector<const char*> validationLayers = {
	"VK_LAYER_LUNARG_standard_validation"
};

// list of required device extensions
const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME	// Swap Chaine for images
};


/// <summary>
/// Creates a VkShaderModule Object.
/// </summary>
/// <param name="device">The device.</param>
/// <param name="code">The buffer with the bytecode.</param>
/// <returns></returns>
VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code) {
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	// size spezified in bytes
	createInfo.codeSize = code.size();
	// bytecode pointer is a uint32_t pointer
	// performing a cast like this, one needs to ensure that the data satisfies the
	// alignment requirements of uint32_t
	// Luckily the data is stored in an std::vector where the default allocator already  
	// ensures that the data satisfies the worst case alignment requirements
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	// Create a VkShaderModule Object
	VkShaderModule shaderModule;

	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module!");
	}

	return shaderModule;
}

/// <summary>
/// Reads the binary data from file.
/// </summary>
/// <param name="filename">The filename.</param>
/// <returns></returns>
static std::vector<char> readFile(const std::string& filename) {
	// will read all of the bytes from the specified file and return them
	// in a byte array managed by std::vector
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	// Flags:
	//	ate: Start reading at the end of the file
	//	binary: Read the files as binary file (avoid text transformations)

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	// use read position (at the end of the file) to determine 
	size_t fileSize = (size_t)file.tellg();
	// allocate the buffer
	std::vector<char> buffer(fileSize);

	// seek back to the beginning of the file and read all of the bytes at once
	file.seekg(0);
	file.read(buffer.data(), fileSize);

	// close the file and return the bytes
	file.close();

	// DEBUG SHADER FILE SIZE
	//std::cout << fileSize << std::endl;

	return buffer;
};

/*
 * Background Proxy Function
 * Pass struct to vkCreateDebugUtilsMessengerExt function to
 * create the VkDebugUtilsMessengerEXT object
 */
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, 
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
	const VkAllocationCallbacks* pAllocator, 
	VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	//look up the address of the Function
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

/*
 * Background Proxy Function
 * Clean up the VkDebugUtilsMessengerEXT
 */
void DestroyDebugUtilsMessengerEXT(VkInstance instance,
	VkDebugUtilsMessengerEXT debugMessenger,
	const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}


/* --- Public --- */

void TriangleApplication::run() {
	initWindow();
	initVulkan();
	mainLoop();
	cleanup();
}

/* --- Private --- */

/*
 * Initialize the Window
 */
void TriangleApplication::initWindow() {
	// Initialize GLFW library
	glfwInit();
	// Not create an OpenGL context 
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	// Disable window Resizing
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	// Initialize the Window
	//  4. parameter specifiy Monitor to open Window on.
	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}

/*
 * Initialize Vulkan
 */
void TriangleApplication::initVulkan() {
	// create instance (connection between app and Vulkan Library)
	createInstance();
	setupDebugCallback();
	createSurface();
	pickPhysicalDevice();
	// create logical device to interface with it
	createLogicalDevice();
	// create swap chain
	createSwapChain();
	createImageViews();
	// create a render pass object
	createRenderPass();
	// create draphic pipeline for rendering with Vulkan
	createGraphicsPipeline();
	// create Framebuffer object
	createFramebuffers();
	// create command pool object
	createCommandPool();
	// create command Buffers
	createCommandBuffers();
	// create semaphores
	createSyncObjects();
}

void TriangleApplication::createImageViews()
{
	// resize the list to fit all image views
	swapChainImageViews.resize(swapChainImages.size());

	// loop over all of the swap chain images
	for (size_t i = 0; i < swapChainImages.size(); i++) {
		// specification of image view creation parameters
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = swapChainImages[i];
		// specification how the image data should be interpreted
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;	// treat images as 1d 2d or 3d textures (or cubemaps)
		createInfo.format = swapChainImageFormat;
		// componente allows to swizzle the color channels around
		// can create monochrome textures or constant values (like 0 and 1)
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;	// Default Mapping
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		// description of image's purpose and what parts of the image should be accessed
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;	// color target
		createInfo.subresourceRange.baseMipLevel = 0;	// no mimap lvel
		createInfo.subresourceRange.levelCount = 1;	//only 1 level
		createInfo.subresourceRange.baseArrayLayer = 0;	// no mulit layer (for stereographic 3D application
		createInfo.subresourceRange.layerCount = 1;	// only 1 layer

		// creating image view
		if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image views!");
		}
	}
}

void TriangleApplication::createSwapChain()
{
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

	// number of images to have in swap chain
	// using the minimum number plus one to avoid wainting on the driver to complete
	// internal operations vefore acquiring another image to render
	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

	// make sure to not exceed the maximum number of images
	// 0 is a special value, that means that there is no maximum
	if (swapChainSupport.capabilities.maxImageCount > 0 && 
		imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	// specify the details of the swap chain images
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;

	createInfo.imageExtent = extent;
	// amount of layers each image consist of (>1 if developing a steroscopic 3d application)
	createInfo.imageArrayLayers = 1;	
	// specify for what kind of operations the images in the swap chain are used for  
	// to render images to a separate image to perform post processing operations use:
	//     VK_IMAGE_USAGE_TRANSFER_DST_BIT 
	// and using memory operations to transfer the rendered image to swap chain image
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	if (indices.graphicsFamily != indices.presentFamily) { // if families differ
		// image is owned by one queue family ata time and ownership must 
		// be explicitly transfered before using it in another queue family
		// This option offers the best performance
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		// requires to specify in adavance which queue families ownerships will be shared
		createInfo.queueFamilyIndexCount = 2;	// at leaste 2 distinct queue families
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {	// if graphics queue family and presentation queue family are the same

		// Images can be used across multiple queue families without explicit ownership transfers
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
	}

	// specify a certain transfrom to be applied to images in the swap chain it its supported.
	// if no transformation 
	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		// supported Transforms in capabilites
	// specifies if the window should be used for blending with other windows in the window
	// syste
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;	// ignore alpha channel
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;	// dont care about the color of pixels that are obscured
	// for example because another window is in front of them (better performance, unless the necessarity
	// to be able to reade these pixels back and get predictable results is needed)

	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	// query final number of images
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
	// resize the container
	swapChainImages.resize(imageCount);
	// finally call the Function to retrieve the handle
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

	// Store format and extent for the wap chain images in memeber variables
	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;

}

/*
 * Creates an abstract Surface
 */
void TriangleApplication::createSurface() {
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
}

/*
 * Select Graphic Hardware
 */
void TriangleApplication::pickPhysicalDevice() {

	// Querying number of graphic cars
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	// if there is no device with Vulkan support
	if (deviceCount == 0) {
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}

	// allocate an array to hold the VkPhysicalDevice handles
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	// Check devices for meeting requirements 
	for (const auto& device : devices) {
		if (isDeviceSuitable(device)) {
			physicalDevice = device;
			break;
		}
	} 

	if (physicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find a suitable GPU!");
	}

	/* Giving a score to the graphic cards and fall back to the integrated if it's the only available */
	// Use an ordered map to automatically sort candidates by increasing score
	/*std::multimap<int, VkPhysicalDevice> candidates;

	for (const auto& device : devices) {
		int score = rateDeviceSuitability(device);
		candidates.insert(std::make_pair(score, device));
	}

	// Check if the best candidate is suitable at all
	if (candidates.rbegin()->first > 0) {
		physicalDevice = candidates.rbegin()->second;
	}
	else {
		throw std::runtime_error("failed to find a suitable GPU!");
	} */
}

/*
 * Creates logical Device to interface with it
 */
void TriangleApplication::createLogicalDevice() {
	// Specifying the queues to be created
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

	// Create the presentation queue
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}
	
	VkPhysicalDeviceFeatures deviceFeatures = {};

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	// add pointers to the queue creation info and device feature structs
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();

	createInfo.pEnabledFeatures = &deviceFeatures;

	//enable same validation layers for devices as for instance
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	// Instantiate the logical device 
	// parameters:
	//	physical device to interface with
	//  queue and usage info
	//  optional allocation callbacks pointer
	//  pointer to a variable to store the logical device handle in 
	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}

	// retrieve queue handles for each queue family
	// parameters:
	//  logical device
	//  queue family
	//  queue index (because only 1 simplpe queue, index = 0)
	//  pointer to the variable to store queue handle in
	vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
	// Call to retrieve the queue handle
	vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

void TriangleApplication::createCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
	// posible command pool flags
	// VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: hint that command buffers are rerecorded 
	//										with new commands very often (may change memory allocation behavior)
	// VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: allow command buffers to rerecord 
	//										individually, without this flag they all have to 
	//                                      to be reset together
	poolInfo.flags = 0; // Optional

	// actually create command pool
	if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool!");
	}
}

void TriangleApplication::createCommandBuffers()
{
	// allocate commandBuffers for each swap chaine image
	commandBuffers.resize(swapChainFramebuffers.size());

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	// specify if the allocated command buffers are:
	// VK_COMMAND_BUFFER_LEVEL_PRIMARY: can be submitted to a queue for execution, 
	//									but cannot be called from other 
	//									command buffers
	// VK_COMMAND_BUFFER_LEVEL_SECONDARY: cannot be submitted directly but can be called from primary
	//									command buffers (it's helpfull to reuse common operations from
	//									primary command buffers)
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	std::cout<< " CBC" << commandBuffers.size();
	allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

	if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}

	// begin recording command buffer 
	for (size_t i = 0; i < commandBuffers.size(); i++)
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		// specify how the command buffer should be used
		// VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT: 
		//		command buffer will be rerecorded after executing it once
		// VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT
		//		secondary command buffer that will be entirely within a single render pass
		// VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
		//		command buffer can be resubmitted while its also already pending execution
		beginInfo.flags = 0; // Optional
		// only relevant for secondary command buffers; 
		// specifies which state to inherit from the calling primary command buffers
		beginInfo.pInheritanceInfo = nullptr; // Optional

		if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to begin recording command buffers!");
		}

		// configure render pass
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		// renderpass itself
		renderPassInfo.renderPass = renderPass;
		// attachment to bind
		renderPassInfo.framebuffer = swapChainFramebuffers[i];

		// define the size of the render area (pixels outside will have undefined values)
		// best performance when it matches the size of the attachment
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapChainExtent;

		//define the clear value to use for VK_ATTACHMENT_LOAD_OP_CLEAR
		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };	// Define black with 100% opacity
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		// begin render pass
		// the final parameter contols ow the drawing commands will be provided
		// VK_SUBPASS_CONTENTS_INLINE: will be embedded in the primary command buffer itself and 
		//								no secondary command will be executed 
		// VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS: will be executed from the secondary 
		//												command buffers
		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			// bind graphics pipeline
			// secondary parameter specifies if the pipeline object is a graphics or compute pipeline
			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
			// Define Draw function
			// Parameters:
			//	Command Buffer
			//	vertexCount:	3 Vertices to define a triangle
			//	instanceCount:	1 if no instance rendering should be used
			//	firstVertex:	offset into the vertex buffer, defines the lowest value of gl_VertexIndex
			//	firstInstance:	offset for instanced rendering, defines the lowest value fo gl_InstanceIndex
			vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);
		// end the render pass
		vkCmdEndRenderPass(commandBuffers[i]);

		// finishe recording the command buffers
		if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}
}

void TriangleApplication::drawFrame()
{
	// wait here for the frame to be finished
	// last parameter is the time out, the previous parameter indicates to wait for all fences
	vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
	
	// aquiring an image from the swap chain
	// ------------------------------------
	uint32_t imageIndex;
	// parameters:
	//	device - logical device
	//	swapChain - swap chain from which the image should be acquired
	//	timeout - timeout in nanoseconds for an image to become available (UINT64_MAX disables the timeout)
	//	synchronication object (semaphore) - to be signaled when the presentation engine is finished
	//  synchronication object (fence) - to be signaled when the presentation engine is finished
	//	index - index of the swap chain image in the swapChainImages Array
	vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore[currentFrame], VK_NULL_HANDLE, &imageIndex);

	// Check if a previous frame is using this image (i.e. there is its fence to wait on)
	if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
		vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}
	// Mark the image as now being in use by this frame
	imagesInFlight[imageIndex] = inFlightFences[currentFrame];

	// submitting the command buffer
	// -----------------------------
	// Queue submission and synchronization configuration
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	
	// specify which semaphores to wait on before execution begins
	VkSemaphore waitSemaphores[] = { imageAvailableSemaphore[currentFrame] };
	// specify which stages of the pipeline to wait
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };	// Stage: Writing colors to image buffer
	submitInfo.waitSemaphoreCount = 1;
	// each entry in the wait Stages array corresponds to the semaphore with the same index in pWaitEmaphores
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	// specify command buffers to actually submit for execution
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

	// specify which semaphores to signale once the command buffers(s) have finish execution
	VkSemaphore signalSemaphores[] = { renderFinishedSemaphore[currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	// manually restore fence to the unsignaled state
	vkResetFences(device, 1, &inFlightFences[currentFrame]);

	// submit the command buffer to the graphics queue
	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	// Presentation
	// ------------
	// submitting the result back to the swap chain to have it show up on the screen

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	// specify which semaphores ot wait on before presentation
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	// specify the swap chains to present images to and the index of the image for each swap chain 
	VkSwapchainKHR swapChains[] = { swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	
	// specify an array of VkResult values  to check for every individual swap chain if presentation was successfull 
	presentInfo.pResults = nullptr; // Optional, when using just one swap chain
	// submit the request to present an image to the swap chain
	vkQueuePresentKHR(presentQueue, &presentInfo);

	// advance current frame to the next
	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

int TriangleApplication::rateDeviceSuitability(VkPhysicalDevice device) {
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;

	// Querying for name, type, supported Vulkan version
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	// Querying for optional features like: texture compression, 64bit floats & multi viewport rendering (for VR)
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	int score = 0;

	// Discrete GPUs have a significant  performance advantage
	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		score += 1000;
	}

	// Maximum possible size of textures affects graphics quality
	score += deviceProperties.limits.maxImageDimension2D;

	// Application can't function without geometry shaders
	if (!deviceFeatures.geometryShader) {
		return 0;
	}

	return score;
}

SwapChainSupportDetails TriangleApplication::querySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details;

	// check basic surface capabilites
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	// querying the supported surface formats
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	if (formatCount != 0) {
		details.formats.resize(formatCount);
		// make sure that the vector is resized to hold all the available formats
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	}

	// querying the supported presentation modes
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

/*
 * Base device suitability checks
 * Evaluation function for graphic cars
 * Check if they are suitable for the operations to be performed
 */
bool TriangleApplication::isDeviceSuitable(VkPhysicalDevice device) {

	/*VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;

	// Querying for name, type, supported Vulkan version
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	// Querying for optional features like: texture compression, 64bit floats & multi viewport rendering (for VR)
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	//  only dedicated graphics cards that support geometry shader
	return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
		deviceFeatures.geometryShader;*/

	//Ensure that the device can process the commands to be used
	QueueFamilyIndices indices = findQueueFamilies(device);

	bool extensionsSupported = checkDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	// imported that query for swap chain support after that the extension is available
	if (extensionsSupported) {
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
		// check if there's at leaste one supported image formate and one supported presentation mode 
		// given the window surface we have
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	return indices.isComplete() && extensionsSupported && swapChainAdequate;

}

bool TriangleApplication::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	// Enumerate the extensions and check if all of the required extensions are among them
	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}
	
	return requiredExtensions.empty();
}

/*
 * Check what queue families are supported by the device and which one of these supports the commands to be used
 */
QueueFamilyIndices TriangleApplication::findQueueFamilies(VkPhysicalDevice device) {
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		// Find device that 
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}

		// look for a queue family that has the capability of presenting to our
		// window surface
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

		// check the value of the boolean and store the presentation family queue index
		if (queueFamily.queueCount > 0 && presentSupport) {
			indices.presentFamily = i;
		}

		if (indices.isComplete()) {
			break;
		}
		i++;
	}

	return indices;
}

void TriangleApplication::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	//allows specifing all the types of serverities you would like the callback to be called for
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	//except for: VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT  
	//filter which types of messages the callback is notified about
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	//specifies the pointer to the callback functions
	createInfo.pfnUserCallback = debugCallback;
}

/*
 * Initilaize Debugging
 */
void TriangleApplication::setupDebugCallback() {
	if (!enableValidationLayers) return;

	//Fill a structure with details about the callback
	VkDebugUtilsMessengerCreateInfoEXT createInfo;

	createInfo.pUserData = nullptr; //optional eg. pointer to TriangleApplication class

	populateDebugMessengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug messenger!");
	}
}

/*
 * Main Loop iterates until Window ist closed
 */
void TriangleApplication::mainLoop() {
	while (!glfwWindowShouldClose(window)) {
		//
		glfwPollEvents();
		drawFrame();
	}

	// wait until operations are done
	vkDeviceWaitIdle(device);


}

/*
 * Deallocate the resources
 */
void TriangleApplication::cleanup() {
	// clean up semaphores 
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(device, renderFinishedSemaphore[i], nullptr);
		vkDestroySemaphore(device, imageAvailableSemaphore[i], nullptr);
		vkDestroyFence(device, inFlightFences[i], nullptr);
	}
	
	vkDestroyCommandPool(device, commandPool, nullptr);

	// Destroy the created Framebuffers
	for (auto framebuffer : swapChainFramebuffers) {
		vkDestroyFramebuffer(device, framebuffer, nullptr);
	}
	// Destroy Graphics Pipeline
	vkDestroyPipeline(device, graphicsPipeline, nullptr);
	// Destroy the Render Pipeline
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	// Destroy the Render Pass Object
	vkDestroyRenderPass(device, renderPass, nullptr);
	// destory explicitly created image views 
	for (auto imageView : swapChainImageViews) {
		vkDestroyImageView(device, imageView, nullptr);
	}

	// clean up the swap chain
	vkDestroySwapchainKHR(device, swapChain, nullptr);
	// destroy logical device
	vkDestroyDevice(device, nullptr);

	//Destroy Debug Util
	if (enableValidationLayers) {
		DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	}

	// destroying the surface
	vkDestroySurfaceKHR(instance, surface, nullptr);	// has to be destroyed before the instance

	/* All other Vulkan resources created, should be cleaned up before,
	 * the instance is destoyed
	 */
	vkDestroyInstance(instance, nullptr);

	glfwDestroyWindow(window);

	glfwTerminate();
}

/// <summary>
/// Creates the framebuffers.
/// </summary>
void TriangleApplication::createFramebuffers()
{
	// resize the container to hold all of the framebuffers
	swapChainFramebuffers.resize(swapChainImageViews.size());

	// iterate through the image views and create frambuffers from them
	for (size_t i = 0; i < swapChainImageViews.size(); i++) {
		VkImageView attachments[] = {
			swapChainImageViews[i]
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		// set renderPass with which the framebuffer needs to be compatible with
		framebufferInfo.renderPass = renderPass;
		// specification of the VkImageView Objects the should be bound to the 
		// respective attachment description in the render pass pAttachment array
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;

		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		// number of layers in image arrays
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer!");
		}
	}

}

/*
 * Set up a callback
 * @return required list of extensions (whether validation layers are enabled or not)
 */
std::vector<const char*> TriangleApplication::getRequiredExtensions() {
	uint32_t glfwExtensionsCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionsCount);

	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); //equal to VK_EXT_debug_utils
		//Debug report extension is conditionally added
	}

	return extensions;
}


/*
 * Creates instance (connection between app and Vulkan Library)
 */
void TriangleApplication::createInstance() {
	if (enableValidationLayers && !checkValidationLayerSupport()) {
		throw std::runtime_error("validation layers requested, but not available!");
	}

	// Fill a struct with Informationen about Application 
	// (optional), may provide useful informationen to the driver for optimisation
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;	//specify struct Type
	appInfo.pApplicationName = "Application Name";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	// (non optional) tells the Vulkan driver which global extension and validation 
	// layers we want to use
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	// Get Extension to interface with the window system
	auto extensions = getRequiredExtensions();

	// enabling the device extension 
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;

	// enable/disable global validation layer
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else {
		createInfo.enabledLayerCount = 0;

		createInfo.pNext = nullptr;
	}

	// Parameters 
	// - pointer to struct with creation info
	// - pointer to custom allocator callbacks
	// - pointer to the variable that stores the handle to the new object
	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
		throw std::runtime_error("failed to create instance!");
	}

	// Debug
	//checkForExtensionSupport();
}

/*
 * Checkin for extension support
 */
void TriangleApplication::checkForExtensionSupport() {
	//Number of exensions
	uint32_t extensionCount = 0;
	//retrieve number of extensions (leaving latter paramter empty
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	//allocate an array to hold extensions details
	std::vector<VkExtensionProperties> extensions(extensionCount);

	//query extensions details
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	std::cout << "available extensions:" << std::endl;

	for (const auto& extension : extensions) {
		std::cout << "\t" << extension.extensionName << std::endl;
	}

	std::cout << "required extensions:" << std::endl;

	uint32_t requiredcount;
	const char** reqExtensions = glfwGetRequiredInstanceExtensions(&requiredcount);

	for (uint32_t i = 0; i < requiredcount; i++) {
		std::cout << "\t" << reqExtensions[i] << std::endl;
	}

	std::cout << "missing extensions:" << std::endl;

	for (uint32_t i = 0; i < requiredcount; i++) {
		bool cont = false;

		for (const auto& extension : extensions) {
			if (strcmp(reqExtensions[i], extension.extensionName) == 0) {
				cont = true;
			}
		}

		if (!cont) {
			std::cout << "\t" << reqExtensions[i] << std::endl;
		}
	}
}

/*
 * Checks if all of the requested layers are available.
 */
bool TriangleApplication::checkValidationLayerSupport() {

	//Get available Layers
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	//Check if all layers in valdiationLayers exist in the availableLayers list
	for (const char * layerName : validationLayers) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
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

VkSurfaceFormatKHR TriangleApplication::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&	//	format specifies the color channels and types
			availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR // indicates if the SRGB Color Space is supported or not
			)
		{
			return availableFormat;
		}
	}
	// if choosen format is not avaiable its okay to settle the first format that is specified
	return availableFormats[0];
}

VkPresentModeKHR TriangleApplication::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	/**
	 * Vulkan possible Swap Chaine Presentation Modes:
	 * VK_PRESENT_MODE_IMMEDIATE_KHR: Images submitted by your application are transferred to the screen right away, 
	 *                                which may result in tearing.
     * VK_PRESENT_MODE_FIFO_KHR: The swap chain is a queue where the display takes an image from the front of the 
	 *                           queue when the display is refreshed and the program inserts rendered images at the 
	 *                           back of the queue. If the queue is full then the program has to wait. This is most 
	 *                           similar to vertical sync as found in modern games. The moment that the display is 
	 *                           refreshed is known as "vertical blank".
     * VK_PRESENT_MODE_FIFO_RELAXED_KHR: This mode only differs from the previous one if the application is late and 
	 *                                   the queue was empty at the last vertical blank. Instead of waiting for the next 
	 *                                   vertical blank, the image is transferred right away when it finally arrives. 
	 *                                   This may result in visible tearing.
     * VK_PRESENT_MODE_MAILBOX_KHR: This is another variation of the second mode. Instead of blocking the application 
	 *                              when the queue is full, the images that are already queued are simply replaced with 
	 *                              the newer ones. This mode can be used to implement triple buffering, which allows you
	 *                              to avoid tearing with significantly less latency issues than standard vertical sync that 
	 *                              uses double buffering.
	 *
	 */

	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentMode;
		}
	}

	// FIFO is the only mode to be guaranteed to be available
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D TriangleApplication::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != UINT32_MAX) {
		return capabilities.currentExtent;
	}
	else {
		VkExtent2D actualExtent = { WIDTH, HEIGHT };

		// clamp the value of WIDTH and HEIGHT
		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}

void TriangleApplication::createGraphicsPipeline()
{
	// Create Shader
	auto vertShaderCode = readFile("../shadercomp/vert.spv");
	auto fragShaderCode = readFile("../shadercomp/frag.spv");

	VkShaderModule vertShaderModule = createShaderModule(device, vertShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(device, fragShaderCode);

	// Vertex Shader
	/////

	// Shader Stage Creation (assign the shaders to a the VkPipelineShaderStageCreateInfo structures)
	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	// setting the pipeline stage the shader is going to be used in
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	// Shader Type
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

	// specify the shader Module
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";	// function to invoke, known as the entrypoint
	// optional member, used to specify values for shader constants
	//vertShaderStageInfo.pSpecializationInfo 

	// Fragment Shader		
	/////

	// Shader Stage Creation (assign the shaders to a the VkPipelineShaderStageCreateInfo structures)
	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	// setting the pipeline stage the shader is going to be used in
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	// Shader Type
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;

	// specify the shader Module
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";	// function to invoke, known as the entrypoint



	// Array to containe the Shader structs
	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	// describes the format of the vertex data that will be passed to the 
	// vertex shader
	// binds: spacing between data and whether the data is per-vertex or per-instance
	// attributes descriptions: type of the attributes passed to the vertex shader which
	//                          binding to load them from and at which offset
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};								// NOTE: Empty because data is hard coded in shader 
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	// point to an array of structs that describe the aforementioned details for loading vertex data
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

	// description what kind of geometry willl be drawn from the vertices and if primitive 
	// restart should be enabled
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	// possible memebers:
	// VK_PRIMITIVE_TOPOLOGY_POINT_LIST - points from vertices
	// VK_PRIMITIVE_TOPOLOGY_LINE_LIST - line from every 
	// VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY
	// VK_PRIMITIVE_TOPOLOGY_LINE_STRIP	- the end vertex of every line is used as start vertex for the next line
	// VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST - traingle from every 3 vertices without reuse
	// VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP	- the second and third vertex of every triangle are used as first two vertices of the next triangle
	// VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN
	// VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY
	// VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY
	// VK_PRIMITIVE_TOPOLOGY_PATCH_LIST
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;	// draw Triangles 
	// if set to True break up _STRIP by using index 0xFFFF or 0xFFFFFFFF
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// Setting Viewport
	// ----------------
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)swapChainExtent.width;
	viewport.height = (float)swapChainExtent.height;
	// Depth values must be in the range [0,1]
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	// Setting Scissors
	// ---------------
	// define in which regions pixels will actually be stored
	// any pixels outside the scissor rectangles will be discarded by the rasterizer
	// Like a Filter (not transform)
	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };	// draw entire framebuffer; scissor rectangle covers everything
	scissor.extent = swapChainExtent;

	// combine viewport state
	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;	// reference an array to allow multiple viewports on some graphics cards (needs an GPU feature enabled)
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;	// reference an array to allow multiple scissor rectangles on some graphics cards

	// Rasterizer
	// ----------
	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	// if True: fragments that are beyond the near and far planes are clamped to them as opposed
	// to discarding them (usefull for shadow maps); NOTE: needs a GPU feature enabled
	rasterizer.depthClampEnable = VK_FALSE;
	// if True: geometry never passes through the rasterizer stage (disables any output to the framebuffer)
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	// Polygon drawing Mode
	// VK_POLYGON_MODE_FILL - fill the area of the polygon with fragments
	// VK_POLYGON_MODE_LINE - polygon edges are drawn as lines			(needs GPU feature enabled)
	// VK_POLYGON_MODE_POINT - polygon vertices are drawn as points		(needs GPU feature enabled)
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	// describes the thickness of lines in terms of number fragments (max line width supported depends on the hardware and any 
	// line thicker than 1.0f requires the wideLines GPU feature to be enabled)
	rasterizer.lineWidth = 1.0f;
	// Type of face culling (front culling: VK_CULL_MODE_FRONT_BIT) (front and back culling: VK_CULL_MODE_FRONT_AND_BACK) (no culling: VK_CULL_MODE_NONE)
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;	
	// specifies the order for faces to eb considered front-facing (counter clockwise: VK_FRONT_FACE_COUNTER_CLOCKWISE)
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE; // Clockwise is front face
	// the rasterizer can alter the depth values by adding a constant value o biasing them based on a fragments slope (sometimes used for shadow mapping)
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	// Multisampling
	// -------------
	// one way to perform anti-aliasing
	// needs an GPU feature enabled
	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;	// Disable multisampling (for now)
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional
	
	// Depth and Stencil Testing
	// -------------------------
	// Not in this Program needed
	// VkPipelineDepthStencilStateCreateInfo depthStencil = {};

	// Color Blending 
	// --------------
	// contains the configuration per attached framebuffer
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT; 
	colorBlendAttachment.blendEnable = VK_FALSE;	// color is passed through unmodified
	// IF blendEnable is TRUE: 
	// two mixing operations are performed to compute a new color.
	// The result is AND'd witht the colorWriteMask to determine which channels are actually passed through
	/**
	 *  // Enabled Alpha Blending
	 *	colorBlendAttachment.blendEnable = VK_TRUE;
	 *	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	 *	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	 *	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	 *	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	 *	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	 *	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	 */
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	// contains the global color blending settings
	// references the array of structures for all of the framebuffers and allows to set blend constants
	// that can be used as blend factors in the calculations
	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	// enables bitwise combination for blending (will disable the color blending in VkPipelineColorBlendAttachmentState 
	// for every attached framebuffer)
	// Will also use the colorWriteMask to determine which channels in the framebuffer will actually be affected
	colorBlending.logicOpEnable = VK_FALSE;
	// sets the bitwise operation
	colorBlending.logicOp = VK_LOGIC_OP_COPY;	// Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;		// Optional
	colorBlending.blendConstants[1] = 0.0f;		// Optional
	colorBlending.blendConstants[2] = 0.0f;		// Optional
	colorBlending.blendConstants[3] = 0.0f;		// Optional

	// Dynamic State
	// -------------
	// fill in the dynamic States which can be changed without recreating the pipeline
	// ignore values of the configuration and need to be specified at drawing time
	// In this programm substitued by a nullptr
	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_LINE_WIDTH
	};

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;

	// Pipeline Layout
	// ---------------
	// Specify uniform variables to be passed to the shader
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0; // Optional
	pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
	// Push constants 
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	// Combining the Pipeline Setups
	// -----------------------------
	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	// Shader array referencing
	pipelineInfo.pStages = shaderStages; 
	// referencing all of the fixed function stages
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = nullptr; // Optional
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr; // Optional
	// referencing the pipeline Layout
	pipelineInfo.layout = pipelineLayout;
	// reference to the render pass and the index of the sub pass where 
	// this graphics pipeline will be used
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	// Used for pipeline derivatives, to be less expensive when they have much functionality in common
	// Only used if the VK_PIPELINE_CREATE_DERIVATIVE_BIT flag also specified in the flags field of 
	// VkGraphicsPipelineCreateInfo
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional; handle to existing pipeline
	pipelineInfo.basePipelineIndex = -1; // Optional; referencing another pipeline that is about to be created by index

	// Create the graphics pipeline
	// the Function is designed to take multiple VkGraphicsPipelineCreateInfo objects and ceate multiple VkPipeline objects in a single call
	// The second argument references an optional VkPipelineCache Object; to use to store and reuse data relevant to pipeline creation across
	// multiple calls to vkCreateGraphicsPipelines and even across program executions if the cache is stored to a file.
	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	// clean up temporary Shader Modules
	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void TriangleApplication::createRenderPass()
{
	// Creating a color attachment
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = swapChainImageFormat;	// set color attachment to swap chain attachment
	// 1 Sample (since no multisampling)
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	// set what to do with the data
	// Load Operations:
	// VK_ATTACHMENT_LOAD_OP_LOAD - preserve the existing contents of the attachments
	// VK_ATTACHMENT_LOAD_OP_CLEAR - clear the values to a constant at the start
	// VK_ATTACHMENT_LOAD_OP_DONT_CARE - existing contents are undefined (dont care)
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	// Store Operations:
	// VK_ATTACHMENT_STORE_OP_STORE - rendered contents will be stored in memory 
	//                                and can be read later
	// VK_ATTACHMENT_STORE_OP_DONT_CARE - contents of the framebuffer will be 
	//                                    undefined after the rendering operation
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	// set what to do with the stencil data
	// nothing will be done with stencil buffer in this programm
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	// most common layouts:
	// VK_IMAGE_LAYOUT_UNDEFINED - the content of the image are not guaranteed to be preserved
	// VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL - images used as color attachments
	// VK_IMAGE_LAYOUT_PRESENT_SRC_KHR - images to be presented in the swap chain
	// VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL - images to be used as destination for
	//                                        a memory copy operation
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // specify which layout the image will have before the render pass begins
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // specify to automatically transition to when the render pass finishes

	// Subpasses and attachment references
	// -----------------------------------
	// Using a single subpass
	// Subpass reference
	VkAttachmentReference colorAttachmentRef = {};
	// which parameter to reference by its index in the 
	// attachment descriptions array
	colorAttachmentRef.attachment = 0;
	// specification which layout the attachments has during a subpass that
	// uses this reference
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// description of subpass
	VkSubpassDescription subpass = {};
	// explicit about this being a graphics subpass
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	// Specify the reference to the color attachment
	subpass.colorAttachmentCount = 1;
	// the following types of attachments can be reference by a subpass
	// pInputAttachments - Attachments that a read from a shader
	// pResolveAttachments - Attachments used for multisampling color attachments
	// pDepthStencilAttachments - Attachment for depth and stencil data
	// pPreserveAttachments - Attachments that are not used by this subpass,
	//                        but for which the data must be preserved
	subpass.pColorAttachments = &colorAttachmentRef;

	// dependency 
	// ---------

	// Make the renderpass wait for the VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT stage


	VkSubpassDependency dependency{};
	// specify indices of the dependency and the dependent subpass 
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;	// implicit subpass before or after the render pass depending on wheter it is specified in srcSubpass or dstSubpass
	dependency.dstSubpass = 0;	// subpass which is the first and only one
	// specify the operation to wait on and the stage in which these opeations occur
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;	// waiting for the color attachment output stage
	dependency.srcAccessMask = 0;
	// 
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	// Render pass
	// -----------
	// create Render Object by filling in an array of attachments and subpasses
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	// specify an array of dependencies
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}

/// <summary>
/// Creates the semaphores.
/// </summary>
void TriangleApplication::createSyncObjects()
{	
	// allocate sempaphores for each frame
	imageAvailableSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	// explicitly initialize to no fence
	imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);

	// create semaphores
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;	// initalize the fences to be initialized in the signal state

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{

		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore[i]) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore[i]) != VK_SUCCESS ||
			vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create synchronization objects for a frame!");
		}
	}
}
