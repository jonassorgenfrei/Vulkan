#version 450
#extension GL_KHR_vulkan_glsl: enable
#extension GL_ARB_separate_shader_objects: enable

//////////////////
/// Data Transport 
//////////////////
layout(location = 0) in vec3 fragColor; // linked together using location directives

layout(location = 0) out vec4 outColor;

void main() {
	// output color red
	outColor = vec4(fragColor, 1.0);
}