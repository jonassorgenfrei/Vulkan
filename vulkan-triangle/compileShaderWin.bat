REM change Path to Vulkan SDK Path
%VULKAN_SDK%/Bin32/glslc.exe shader/main.vert -o shadercomp/vert.spv
%VULKAN_SDK%/Bin32/glslc.exe shader/main.frag -o shadercomp/frag.spv
pause