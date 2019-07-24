#include "TriangleApplication.h"

/* --- Helper --- */

/* implicitly enables a whole range of useful diagnostics layers */
const std::vector<const char *> validationLayers = {
	"VK_LAYER_LUNARG_standard_validation"
};

// Enable ValidationLayers depending on Debug Level
#ifdef  NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif //  NDEBUG

/*
 * Background Proxy Function
 * Pass struct to vkCreateDebugUtilsMessengerExt funct. to
 * create the VkDebugUtilsMessengerEXT object
 */
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks * pAllocator,
	VkDebugUtilsMessengerEXT * pCallback)
{
	//look up the address of the Function
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pCallback);
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
	VkDebugUtilsMessengerEXT callback,
	const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, callback, pAllocator);
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
	/*for (const auto& device : devices) {
		if (isDeviceSuitable(device)) {
			physicalDevice = device;
			break;
		}
	} */

	/* Giving a score to the graphic cards and fall back to the integrated if it's the only available */
	// Use an ordered map to automatically sort candidates by increasing score
	std::multimap<int, VkPhysicalDevice> candidates;

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
	}
}

/*
 * Creates logical Device to interface with it
 */
void TriangleApplication::createLogicalDevice() {
	// Specifying the queues to be created
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

	// Create the presentation queue
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = {	indices.graphicsFamily.value(), 
												indices.presentFamily.value()};

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
	createInfo.enabledExtensionCount = 0;

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

	return indices.isComplete();

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

/*
 * Initilaize Debugging
 */
void TriangleApplication::setupDebugCallback() {
	if (!enableValidationLayers) return;

	//Fill a structure with details about the callback
	VkDebugUtilsMessengerCreateInfoEXT  createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	//allows specifing all the types of serverities you would like the callback to be called for
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT; //except for: VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT  
//filter which types of messages the callback is notified about
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	//specifies the pointer to the callback functions
	createInfo.pfnUserCallback = debugCallback;
	createInfo.pUserData = nullptr; //optional eg. pointer to TriangleApplication class


	if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &callback) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug callback!");
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
	// destroy logical device
	vkDestroyDevice(device, nullptr);

	//Destroy Debug Util
	if (enableValidationLayers) {
		DestroyDebugUtilsMessengerEXT(instance, callback, nullptr);
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

	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	// enable/disable global validation layer
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	// Parameters 
	// - pointer to struct with creation info
	// - pointer to custom allocator callbacks
	// - pointer to the variable that stores the handle to the new object
	VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to create instance!");
	}

	// Debug
	checkForExtensionSupport();
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
