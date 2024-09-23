#include "VulkanHelper.h"

#include "Util/Util.h"
#include <cassert>

namespace Core
{
	bool CreateShader(VkDevice LogicalDevice, const uint32_t* Code, uint32_t CodeSize,
		VkShaderModule &VertexShaderModule)
	{
		VkShaderModuleCreateInfo ShaderModuleCreateInfo = { };
		ShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		ShaderModuleCreateInfo.codeSize = CodeSize;
		ShaderModuleCreateInfo.pCode = Code;

		VkResult Result = vkCreateShaderModule(LogicalDevice, &ShaderModuleCreateInfo, nullptr, &VertexShaderModule);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateShaderModule result is {}", static_cast<int>(Result));
			return false;
		}

		return true;
	}
}