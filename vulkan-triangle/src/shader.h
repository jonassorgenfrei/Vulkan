#pragma once
//Include Vulkan
#include <vulkan/vulkan.h>
#include <vector>
#include <fstream>

class Shader {
public:


	/// <summary>
	/// Initializes a new instance of the <see cref="Shader" /> class.
	/// </summary>
	/// <param name="device">The device.</param>
	/// <param name="vertexShader">The vertex shader.</param>
	/// <param name="fragmentShader">The fragment shader.</param>
	Shader(VkDevice device, const std::string& vertexShader,
			const std::string& fragmentShader) {
		auto vertShaderCode = readFile(vertexShader);
		auto fragShaderCode = readFile(fragmentShader);

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
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		// Shader Type
		vertShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;

		// specify the shader Module
		vertShaderStageInfo.module = fragShaderModule;
		vertShaderStageInfo.pName = "main";	// function to invoke, known as the entrypoint
		


		// Array to containe the Shader structs
		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };



		// clean up temporary Shader Modules
		vkDestroyShaderModule(device, fragShaderModule, nullptr);
		vkDestroyShaderModule(device, vertShaderModule, nullptr);
	};
private:
	

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
};
