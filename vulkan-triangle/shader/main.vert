#version 450
// Enable Vulkan GLSL 
#extension GL_KHR_vulkan_glsl: enable
#extension GL_ARB_separate_shader_objects : enable

//////////////////
/// Data Transport 
//////////////////

layout(location = 0) out vec3 fragColor;

//////////////
/// Data
//////////////

// Vertex Positions NDC
vec2 positions[3] = vec2[](
	vec2(0.0, -0.5),
	vec2(0.5, 0.5),
	vec2(-0.5, 0.5)
);

// Vertex Colors
vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

//////////////////
/// Main Function
//////////////////

void main() {
	// built-in gl_VertexIndex contains the index of the current Vertex
	gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
	fragColor = colors[gl_VertexIndex];
}