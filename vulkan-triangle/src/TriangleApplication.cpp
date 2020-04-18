#include "TriangleApplication.h"
#include "root_directory.h"

/* --- Helper --- */

/* implicitly enables a whole range of useful diagnostics layers */
const std::vector<const char*> validationLayers = {
	"VK_LAYER_LUNARG_standard_validation"
	//"VK_LAYER_LUNARG_standard_validation"
};

// list of required device extensions
const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME	// Swap Chaine for images
};

// Enable ValidationLayers depending on Debug Level
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif //  NDEBUG

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
	// Create Logical Device to interface with it
	createLogicalDevice();
	// Create Swap Chain
	createSwapChain();
	createImageViews();
	// create a Render Pass Object
	createRenderPass();
	// Create Graphic Pipeline for rendering with Vulkan
	createGraphicsPipeline();
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
	}
}

/*
 * Deallocate the resources
 */
void TriangleApplication::cleanup() {
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
	Shader shader(device, "../shadercomp/vert.spv", "../shadercomp/frag.spv");

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

}

void TriangleApplication::createRenderPass()
{
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}
