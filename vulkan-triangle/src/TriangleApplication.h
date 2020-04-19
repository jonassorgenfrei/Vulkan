#ifndef TRIANGLEAPPLICATION_H
#define TRIANGLEAPPLICATION_H

//Include Vulkan
#include <vulkan/vulkan.h>

//Include GLFW
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <algorithm> 
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint> // Necessary for UINT32_MAX
#include <optional>
#include <set>
#include <functional>
#include <map>
#include <fstream>

// Application Header
#include "shader.h""


/* constants window sizes */
const int WIDTH = 800;
const int HEIGHT = 600;

// struct for Queue families
struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;		// ensure that the device can present 
												// images to the surface

	bool isComplete() {
		return graphicsFamily.has_value() && 
			presentFamily.has_value();
	}
};

/**
 * Three kinds of property compatiblities
 * to check
 */
struct SwapChainSupportDetails {
	// Basic surface capabilites (min/max number of images in swap chain, min/max width and height of images)
	VkSurfaceCapabilitiesKHR capabilities;
	// Surface formats (pixel format, color space)
	std::vector<VkSurfaceFormatKHR> formats;
	// Available presentation modes
	std::vector<VkPresentModeKHR> presentModes;
};

/* Application Class */
class TriangleApplication {
public:
	void run();
private:
	// -------------------------
	// Class Memebers
	// -------------------------

	/* Function to store a reference to the actual Window */
	GLFWwindow* window;

	VkInstance instance;
	/* Tell Vulkan about callback function */
	VkDebugUtilsMessengerEXT debugMessenger;
	/* Abstract type of surface to present rendered images */
	VkSurfaceKHR surface;

	/* Graphic card, that willbe selected
	   (will be implicitly destroyed when the VkInstance is destroyed) */
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	/* logical device handle */
	VkDevice device;

	/* Set if device features that will be used */
	VkPhysicalDeviceFeatures deviceFeatures = {};

	/* Handle to the graphics queue (will be implicitly destroyed when the device is destroyed) */
	VkQueue graphicsQueue;
	/* Presentation Queue */
	VkQueue presentQueue;

	/* Stores the VkSwapchainKHR object */
	VkSwapchainKHR swapChain;
	/* Stores the handles of the VkImages */
	std::vector<VkImage> swapChainImages;
	/* Stores the Swap Chaine Image Format */
	VkFormat swapChainImageFormat;
	/* Swap Chain Extent Object */
	VkExtent2D swapChainExtent;

	/** 
	 * Object to View into an image 
	 * it describes how to access the image and which part of the image to access */
	std::vector<VkImageView> swapChainImageViews;

	/**
	 * Render Pass Object to store Information about Framebuffers
	 */
	VkRenderPass renderPass;

	/** 
	 * Stores the pipelineLayout which holds the information about the uniform 
	 * variables and push constants
	 */
	VkPipelineLayout pipelineLayout;

	VkPipeline graphicsPipeline;
	// -------------------------
	// Functions
	// -------------------------

	/**
	 * Initialize the Window
	 */
	void initWindow();

	/**
	 * Initialize Vulkan
	 */
	void initVulkan();

	/**
	 * Main Loop iterates until Window ist closed
	 */
	void mainLoop();

	/**
	 * Deallocate the resources
	 */
	void cleanup();

	/**
	 * Creates a basic image view in the swap chain
	 */
	void createImageViews();

	/**
	 * Function that calls all the SwapChain Help Functions
	 */
	void createSwapChain();
		
	/**
	 * Creates a surface to draw on
	 */
	void createSurface();
	
	/**
	 * Select Graphic Hardware
	 */
	void pickPhysicalDevice();

	/**
	 * Creates logical Device to interface with it
	 */
	void createLogicalDevice();

	int rateDeviceSuitability(VkPhysicalDevice device);

	/**
	 * Function to poplate the SwapChainSupportDetails Struct
	 */
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

	/**
	 * Base device suitability checks
	 * Evaluation function for graphic cars
	 * Check if they are suitable for the operations to be performed
	 */
	bool isDeviceSuitable(VkPhysicalDevice device);

	/**
	 * Check if the exteions are supported by the hardware
	 */
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);

	/*
	 * Check what queue families are supported by the device and which one of these supports the commands to be used
	 */
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
	
	/**/
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	/*
	 * Initilaize Debugging
	 */
	void setupDebugCallback();

	/**
	 * Set up a callback
	 * @return required list of extensions (whether validation layers are enabled or not)
	 */
	std::vector<const char*> getRequiredExtensions();

	/**
	 * Creates instance (connection between app and Vulkan Library)
	 */
	void createInstance();

	/**
	 * Checkin for extension support
	 */
	void checkForExtensionSupport();

	/**
	 * Checks if all of the requested layers are available.
	 */
	bool checkValidationLayerSupport();

	/**
	 * Chooses the Surface format (color depth)
	 */
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

	/**
	 * Choose the presentation modes (conditions for "swapping" images to the screen)
	 */
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

	/**
	 * Swap extent (resolution of images in swap chain)
	 * Always exavtly equal to the resolution of the window that we're drawing to
	 */
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	/**
	 * Graphics Pipeline
	 */
	void createGraphicsPipeline();

	/**
	 * Create a Renderpass object
	 * Specifies how many color and depth buffer attachments there be,
	 * how many samples to use for each of them and how their contents 
	 * should be handled throughout the rendering operations
	 */
	void createRenderPass();

	/*
	 * Callback Function for prototype PFN_vkDebugUtilsMessengercallbackExt
	 * Type: VKAPI_ATTR & VKAPI_CALL
	 *
	 * @param messageSeverity - specifies the severity of the message
	 *				VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT		- diagnostic message
	 *				VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT		- informational message like the creation of a resource
	 *				VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT		- message about behavior that is not nexessarily an error, but very likely a bug in your application
	 *				VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT		- message about behavior that is invalid and may cuase crashes
	 *
	 *				--> comparison operation to check possible
	 *
	 *	@param messageType - type of the Message
	 *				VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT			- Some event has happend that is unrelated to the specification or performance
	 *				VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT		- Something has happend that violates the specification or indicates a possible mistake
	 *				VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT		- Potential non-optimal use of Vulkan
	 *
	 *	@param pCallbackData - referes to VkDebugUtilsMessengerCallbackDataEXT struct containint the details of the messsage itself
	 *				members:
	 *					pMessage - debug message as a null-terminated string
	 *					pObjects - array of Vulkan object handles related to the message
	 *					objectCount - number of objects in array
	 *
	 *	@param pUserData - contains pointer that was specified during the setupt of the callback, allows to pass own data
	 *
	 *	@return boolean, that indicates if the Vulkan call that triffered the validation message should be aborted
	 *					if true - call aborted with VK_ERROR_CALIDATION_FAILED_EXT
	 */
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
														VkDebugUtilsMessageTypeFlagsEXT messageType, 
														const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData, 
														void * pUserData) {
		std::cerr << "validation layer:" << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}

};
#endif // !TRIANGLEAPPLICATION_H