#include "Util.h"

#include "EngineTypes.h"

namespace Util
{
	bool ReadFileFull(FILE* File, std::vector<char>& OutFileData)
	{
		_fseeki64(File, 0LL, static_cast<int>(SEEK_END));
		const long long FileSize = _ftelli64(File);
		if (FileSize != -1LL)
		{
			rewind(File); // Put file pointer to 0 index

			OutFileData.resize(static_cast<u64>(FileSize));
			// Need to check fread result if we know the size?
			const u64 ReadResult = fread(OutFileData.data(), 1, static_cast<u64>(FileSize), File);
			if (ReadResult == static_cast<u64>(FileSize))
			{
				return true;
			}

			return false;
		}

		return false;
	}

	bool OpenAndReadFileFull(const char* FileName, std::vector<char>& OutFileData, const char* Mode)
	{
		FILE* File = nullptr;
		const int Result = fopen_s(&File, FileName, Mode);
		if (Result == 0)
		{
			if (ReadFileFull(File, OutFileData))
			{
				fclose(File);
				return true;
			}

			Log::Error("Failed to read file {}: ", FileName);
			fclose(File);
			return false;
		}

		Log::Error("Cannot open file {}: Result = {}", FileName, Result);
		return false;
	}
	void LoadPipelineSettings(VulkanInterface::PipelineSettings& Settings, const char* FilePath)
	{
		// TODO: static
		static char buffer[256];

		GetPrivateProfileStringA("Pipeline", "PipelineName", "None", buffer, sizeof(buffer), FilePath);
		Settings.PipelineName = buffer;

		char PipelineFilePath[MAX_PATH];
		if (GetPrivateProfileStringA("Pipeline", "PipelineSettingsFile", FilePath,
			PipelineFilePath, sizeof(PipelineFilePath), FilePath) != 0)
		{
			FilePath = PipelineFilePath;
		}

		Settings.DepthClampEnable = GetPrivateProfileIntA("Pipeline", "DepthClampEnable", 0, FilePath);
		Settings.RasterizerDiscardEnable = GetPrivateProfileIntA("Pipeline", "RasterizerDiscardEnable", 0, FilePath);
		Settings.PolygonMode = (VkPolygonMode)GetPrivateProfileIntA("Pipeline", "PolygonMode", VK_POLYGON_MODE_FILL, FilePath);
		Settings.LineWidth = (float)GetPrivateProfileIntA("Pipeline", "LineWidth", 1, FilePath);
		Settings.CullMode = (VkCullModeFlags)GetPrivateProfileIntA("Pipeline", "CullMode", VK_CULL_MODE_BACK_BIT, FilePath);
		Settings.FrontFace = (VkFrontFace)GetPrivateProfileIntA("Pipeline", "FrontFace", VK_FRONT_FACE_COUNTER_CLOCKWISE, FilePath);
		Settings.DepthBiasEnable = GetPrivateProfileIntA("Pipeline", "DepthBiasEnable", 0, FilePath);
		Settings.BlendEnable = GetPrivateProfileIntA("Pipeline", "BlendEnable", 1, FilePath);
		Settings.LogicOpEnable = GetPrivateProfileIntA("Pipeline", "LogicOpEnable", 0, FilePath);
		Settings.AttachmentCount = GetPrivateProfileIntA("Pipeline", "AttachmentCount", 1, FilePath);
		Settings.ColorWriteMask = GetPrivateProfileIntA("Pipeline", "ColorWriteMask", VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, FilePath);
		Settings.SrcColorBlendFactor = (VkBlendFactor)GetPrivateProfileIntA("Pipeline", "SrcColorBlendFactor", VK_BLEND_FACTOR_SRC_ALPHA, FilePath);
		Settings.DstColorBlendFactor = (VkBlendFactor)GetPrivateProfileIntA("Pipeline", "DstColorBlendFactor", VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, FilePath);
		Settings.ColorBlendOp = (VkBlendOp)GetPrivateProfileIntA("Pipeline", "ColorBlendOp", VK_BLEND_OP_ADD, FilePath);
		Settings.SrcAlphaBlendFactor = (VkBlendFactor)GetPrivateProfileIntA("Pipeline", "SrcAlphaBlendFactor", VK_BLEND_FACTOR_ONE, FilePath);
		Settings.DstAlphaBlendFactor = (VkBlendFactor)GetPrivateProfileIntA("Pipeline", "DstAlphaBlendFactor", VK_BLEND_FACTOR_ZERO, FilePath);
		Settings.AlphaBlendOp = (VkBlendOp)GetPrivateProfileIntA("Pipeline", "AlphaBlendOp", VK_BLEND_OP_ADD, FilePath);
		Settings.DepthTestEnable = GetPrivateProfileIntA("Pipeline", "DepthTestEnable", 1, FilePath);
		Settings.DepthWriteEnable = GetPrivateProfileIntA("Pipeline", "DepthWriteEnable", 1, FilePath);
		Settings.DepthCompareOp = (VkCompareOp)GetPrivateProfileIntA("Pipeline", "DepthCompareOp", VK_COMPARE_OP_LESS, FilePath);
		Settings.DepthBoundsTestEnable = GetPrivateProfileIntA("Pipeline", "DepthBoundsTestEnable", 0, FilePath);
		Settings.StencilTestEnable = GetPrivateProfileIntA("Pipeline", "StencilTestEnable", 0, FilePath);
	}
}
