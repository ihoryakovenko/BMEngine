%VULKAN_SDK%/Bin/glslangValidator.exe -V Shader.vert
%VULKAN_SDK%/Bin/glslangValidator.exe -V Shader.frag

%VULKAN_SDK%/Bin/glslangValidator.exe -o second_vert.spv -V Second.vert
%VULKAN_SDK%/Bin/glslangValidator.exe -o second_frag.spv -V Second.frag

%VULKAN_SDK%/Bin/glslangValidator.exe -o TerrainGenerator_vert.spv -V TerrainGenerator.vert
%VULKAN_SDK%/Bin/glslangValidator.exe -o TerrainGenerator_frag.spv -V TerrainGenerator.frag
pause