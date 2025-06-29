#include "TerrainRender.h"

#include <random>

#include "Engine/Systems/Render/Render.h"
#include "Deprecated/FrameManager.h"
#include "MainPass.h"
#include "Engine/Systems/Render/VulkanHelper.h"
#include "Util/Util.h"
#include "Util/Settings.h"

namespace TerrainRender
{
	struct TerrainVertex
	{
		f32 Altitude;
	};

	// TMP
	struct PushConstantsData
	{
		glm::mat4 Model;
		s32 matIndex;
	};

	static void LoadTerrain();
	static void GenerateTerrain(std::vector<u32>& Indices);

	static const u32 NumRows = 600;
	static const u32 NumCols = 600;
	static TerrainVertex TerrainVerticesData[NumRows][NumCols];
	static TerrainVertex* TerrainVerticesDataPointer = &(TerrainVerticesData[0][0]);
	static u32 IndicesCount;

	static VkDescriptorSet TerrainSet;

	static VulkanHelper::RenderPipeline Pipeline;
	static Render::DrawEntity TerrainDrawObject;

	static VkPushConstantRange PushConstants;

	void Init()
	{
		VkDevice Device = VulkanInterface::GetDevice();

		const u32 ShaderCount = 2;
		VulkanHelper::Shader Shaders[ShaderCount];

		std::vector<char> VertexShaderCode;
		Util::OpenAndReadFileFull("./Resources/Shaders/TerrainGenerator_vert.spv", VertexShaderCode, "rb");
		std::vector<char> FragmentShaderCode;
		Util::OpenAndReadFileFull("./Resources/Shaders/TerrainGenerator_frag.spv", FragmentShaderCode, "rb");

		Shaders[0].Stage = VK_SHADER_STAGE_VERTEX_BIT;
		Shaders[0].Code = VertexShaderCode.data();
		Shaders[0].CodeSize = VertexShaderCode.size();

		Shaders[1].Stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		Shaders[1].Code = FragmentShaderCode.data();
		Shaders[1].CodeSize = FragmentShaderCode.size();

		const Render::RenderState* State = Render::GetRenderState();

		VkDescriptorSetLayout TerrainDescriptorLayouts[] =
		{
			FrameManager::GetViewProjectionLayout(),
			State->RenderResources.Textures.BindlesTexturesLayout,
			State->RenderResources.Materials.MaterialLayout,
		};

		const u32 LayoutsCount = sizeof(TerrainDescriptorLayouts) / sizeof(TerrainDescriptorLayouts[0]);

		PushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		PushConstants.offset = 0;
		// Todo: check constant and model size?
		PushConstants.size = sizeof(PushConstantsData);

		VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = { };
		PipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		PipelineLayoutCreateInfo.setLayoutCount = LayoutsCount;
		PipelineLayoutCreateInfo.pSetLayouts = TerrainDescriptorLayouts;
		PipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		PipelineLayoutCreateInfo.pPushConstantRanges = &PushConstants;

		VULKAN_CHECK_RESULT(vkCreatePipelineLayout(Device, &PipelineLayoutCreateInfo, nullptr, &Pipeline.PipelineLayout));

		VulkanHelper::PipelineResourceInfo ResourceInfo;
		ResourceInfo.PipelineLayout = Pipeline.PipelineLayout;
		ResourceInfo.PipelineAttachmentData = *MainPass::GetAttachmentData();

		const u32 VertexInputCount = 1;
		VulkanHelper::BMRVertexInputBinding VertexInputBinding[VertexInputCount];
		VertexInputBinding[0].InputAttributes[0] = { "Altitude", VK_FORMAT_R32_SFLOAT, offsetof(TerrainVertex, Altitude) };
		VertexInputBinding[0].InputAttributesCount = 1;
		VertexInputBinding[0].InputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		VertexInputBinding[0].Stride = sizeof(TerrainVertex);
		VertexInputBinding[0].VertexInputBindingName = "TerrainVertex";

		VulkanHelper::PipelineSettings PipelineSettings;
		Util::LoadPipelineSettings(PipelineSettings, "./Resources/Settings/StaticMeshRender.ini");
		PipelineSettings.Extent = MainScreenExtent;

		Pipeline.Pipeline = VulkanHelper::BatchPipelineCreation(Device, Shaders, ShaderCount, VertexInputBinding, VertexInputCount,
			&PipelineSettings, &ResourceInfo);

		LoadTerrain();
	}

	void DeInit()
	{
		VkDevice Device = VulkanInterface::GetDevice();

		vkDestroyPipeline(Device, Pipeline.Pipeline, nullptr);
		vkDestroyPipelineLayout(Device, Pipeline.PipelineLayout, nullptr);
	}

	void Draw()
	{
		const Render::RenderState* State = Render::GetRenderState();

		VkCommandBuffer CmdBuffer = VulkanInterface::GetCommandBuffer();

		const VkDescriptorSet Sets[] = {
			FrameManager::GetViewProjectionSet(),
			State->RenderResources.Textures.BindlesTexturesSet,
			State->RenderResources.Materials.MaterialSet,
		};

		const u32 TerrainDescriptorSetGroupCount = sizeof(Sets) / sizeof(Sets[0]);

		vkCmdBindPipeline(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Pipeline);

		const u32 DynamicOffset = VulkanInterface::TestGetImageIndex() * sizeof(FrameManager::ViewProjectionBuffer);
		vkCmdBindDescriptorSets(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.PipelineLayout,
			0, TerrainDescriptorSetGroupCount, Sets, 1, &DynamicOffset);

		PushConstantsData Constants;
		//Constants.Model = TerrainDrawObject.Model;
		//Constants.matIndex = TerrainDrawObject.MaterialIndex;

		const VkShaderStageFlags Flags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		vkCmdPushConstants(CmdBuffer, Pipeline.PipelineLayout, Flags, 0, sizeof(PushConstantsData), &Constants);

		//VkBuffer VertexBuffer = RenderResources::GetVertexBuffer().Buffer;
		//VkBuffer IndexBuffer = RenderResources::GetIndexBuffer().Buffer;

		//u32 Count;
		//const u64 VertexOffset = TerrainDrawObject.VertexOffset;
		//const u64 IndexOffset = TerrainDrawObject.IndexOffset;

		//vkCmdBindVertexBuffers(CmdBuffer, 0, 1, &VertexBuffer, &VertexOffset);
		//vkCmdBindIndexBuffer(CmdBuffer, IndexBuffer, IndexOffset, VK_INDEX_TYPE_UINT32);
		//vkCmdDrawIndexed(CmdBuffer, IndicesCount, 1, 0, 0, 0);
	}

	void LoadTerrain()
	{
		std::vector<u32> TerrainIndices;
		GenerateTerrain(TerrainIndices);

		IndicesCount = TerrainIndices.size();

		Render::Material Mat = { };
		//u32 MaterialIndex = Render::CreateMaterial(&Mat);
		//TerrainDrawObject = RenderResources::CreateTerrain(&TerrainVerticesData[0][0], sizeof(TerrainVertex), NumRows * NumCols,
			//TerrainIndices.data(), IndicesCount, MaterialIndex);
	}

	void GenerateTerrain(std::vector<u32>& Indices)
	{
		const f32 MaxAltitude = 0.0f;
		const f32 MinAltitude = -10.0f;
		const f32 SmoothMin = 7.0f;
		const f32 SmoothMax = 3.0f;
		const f32 SmoothFactor = 0.5f;
		const f32 ScaleFactor = 0.2f;

		std::mt19937 Gen(1);
		std::uniform_real_distribution<f32> Dist(MinAltitude, MaxAltitude);

		bool UpFactor = false;
		bool DownFactor = false;

		for (int i = 0; i < NumRows; ++i)
		{
			for (int j = 0; j < NumCols; ++j)
			{
				const f32 RandomAltitude = Dist(Gen);
				const f32 Probability = (RandomAltitude - MinAltitude) / (MaxAltitude - MinAltitude);

				const f32 PreviousCornerAltitude = i > 0 && j > 0 ? TerrainVerticesData[i - 1][j - 1].Altitude : 5.0f;
				const f32 PreviousIAltitude = i > 0 ? TerrainVerticesData[i - 1][j].Altitude : 5.0f;
				const f32 PreviousJAltitude = j > 0 ? TerrainVerticesData[i][j - 1].Altitude : 5.0f;

				const f32 PreviousAverageAltitude = (PreviousCornerAltitude + PreviousIAltitude + PreviousJAltitude) / 3.0f;

				f32 NormalizedAltitude = (PreviousAverageAltitude - MinAltitude) / (MaxAltitude - MinAltitude);

				const f32 Smooth = (PreviousAverageAltitude <= SmoothMin || PreviousAverageAltitude >= SmoothMax) ? SmoothFactor : 1.0f;

				if (UpFactor)
				{
					NormalizedAltitude *= ScaleFactor;
				}
				else if (DownFactor)
				{
					NormalizedAltitude /= ScaleFactor;
				}

				if (NormalizedAltitude > Probability)
				{
					TerrainVerticesData[i][j].Altitude = PreviousAverageAltitude - Probability * Smooth;
					UpFactor = false;
					DownFactor = true;
				}
				else
				{
					TerrainVerticesData[i][j].Altitude = PreviousAverageAltitude + Probability * Smooth;
					UpFactor = true;
					DownFactor = false;
				}
			}
		}

		Indices.reserve(NumRows * NumCols * 6);

		for (int row = 0; row < NumRows - 1; ++row)
		{
			for (int col = 0; col < NumCols - 1; ++col)
			{
				u32 topLeft = row * NumCols + col;
				u32 topRight = topLeft + 1;
				u32 bottomLeft = (row + 1) * NumCols + col;
				u32 bottomRight = bottomLeft + 1;

				// First triangle (Top-left, Bottom-left, Bottom-right)
				Indices.push_back(topLeft);
				Indices.push_back(bottomLeft);
				Indices.push_back(bottomRight);

				// Second triangle (Top-left, Bottom-right, Top-right)
				Indices.push_back(topLeft);
				Indices.push_back(bottomRight);
				Indices.push_back(topRight);
			}
		}
	}
}
