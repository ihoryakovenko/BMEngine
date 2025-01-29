#include "TerrainSystem.h"

#include "Render/Render.h"
#include "Render/FrameManager.h"
#include "ResourceManager.h"
#include "Util/Util.h"
#include "Util/Settings.h"

namespace TerrainSystem
{
	struct TerrainVertex
	{
		f32 Altitude;
	};

	static void OnDraw();
	static void LoadTerrain();
	static void GenerateTerrain(std::vector<u32>& Indices);

	static const u32 NumRows = 600;
	static const u32 NumCols = 600;
	static TerrainVertex TerrainVerticesData[NumRows][NumCols];
	static TerrainVertex* TerrainVerticesDataPointer = &(TerrainVerticesData[0][0]);
	static u32 IndicesCount;

	static u64 VertexOffset;
	static u64 IndexOffset;
	static VulkanInterface::RenderPipeline Pipeline;
	static VkDescriptorSet TextureSet;
	static VkDescriptorSetLayout SamplerLayout;
	static VkSampler Sampler;

	void Init()
	{
		Render::AddDrawFunction(OnDraw);

		const u32 ShaderCount = 2;
		VulkanInterface::Shader Shaders[ShaderCount];

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

		const VkDescriptorType Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		const VkShaderStageFlags Flags = VK_SHADER_STAGE_FRAGMENT_BIT;
		SamplerLayout = VulkanInterface::CreateUniformLayout(&Type, &Flags, 1);

		VkDescriptorSetLayout TerrainDescriptorLayouts[] =
		{
			FrameManager::GetViewProjectionLayout(),
			SamplerLayout
		};

		VkSamplerCreateInfo DiffuseSamplerCreateInfo = { };
		DiffuseSamplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		DiffuseSamplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		DiffuseSamplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		DiffuseSamplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		DiffuseSamplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		DiffuseSamplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		DiffuseSamplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		DiffuseSamplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
		DiffuseSamplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		DiffuseSamplerCreateInfo.mipLodBias = 0.0f;
		DiffuseSamplerCreateInfo.minLod = 0.0f;
		DiffuseSamplerCreateInfo.maxLod = 0.0f;
		DiffuseSamplerCreateInfo.anisotropyEnable = VK_TRUE;
		DiffuseSamplerCreateInfo.maxAnisotropy = 16;
		Sampler = VulkanInterface::CreateSampler(&DiffuseSamplerCreateInfo);

		VulkanInterface::CreateUniformSets(&SamplerLayout, 1, &TextureSet);

		VulkanInterface::UniformSetAttachmentInfo SetInfo[1];
		SetInfo[0].ImageInfo.imageView = ResourceManager::FindTexture("1giraffe")->ImageView;
		SetInfo[0].ImageInfo.sampler = Sampler;
		SetInfo[0].ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		SetInfo[0].Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

		VulkanInterface::AttachUniformsToSet(TextureSet, SetInfo, 1);

		const u32 LayoutsCount = sizeof(TerrainDescriptorLayouts) / sizeof(TerrainDescriptorLayouts[0]);

		Pipeline.PipelineLayout = VulkanInterface::CreatePipelineLayout(TerrainDescriptorLayouts, LayoutsCount, nullptr, 0);

		VulkanInterface::PipelineResourceInfo ResourceInfo;
		ResourceInfo.PipelineLayout = Pipeline.PipelineLayout;
		ResourceInfo.RenderPass = Render::TestGetRenderPass();
		ResourceInfo.SubpassIndex = 0;

		const u32 VertexInputCount = 1;
		VulkanInterface::BMRVertexInputBinding VertexInputBinding[VertexInputCount];
		VertexInputBinding[0].InputAttributes[0] = { "Altitude", VK_FORMAT_R32_SFLOAT, offsetof(TerrainVertex, Altitude) };
		VertexInputBinding[0].InputAttributesCount = 1;
		VertexInputBinding[0].InputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		VertexInputBinding[0].Stride = sizeof(TerrainVertex);
		VertexInputBinding[0].VertexInputBindingName = "TerrainVertex";

		VulkanInterface::PipelineSettings TerrainPipelineSettings;
		TerrainPipelineSettings.PipelineName = "Terrain";
		// Rasterizer
		TerrainPipelineSettings.Extent = MainScreenExtent;
		TerrainPipelineSettings.DepthClampEnable = VK_FALSE;
		TerrainPipelineSettings.RasterizerDiscardEnable = VK_FALSE;
		TerrainPipelineSettings.PolygonMode = VK_POLYGON_MODE_FILL;
		TerrainPipelineSettings.LineWidth = 1.0f;
		TerrainPipelineSettings.CullMode = VK_CULL_MODE_BACK_BIT;
		TerrainPipelineSettings.FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		TerrainPipelineSettings.DepthBiasEnable = VK_FALSE;
		// Multisampling
		TerrainPipelineSettings.BlendEnable = VK_TRUE;
		TerrainPipelineSettings.LogicOpEnable = VK_FALSE;
		TerrainPipelineSettings.AttachmentCount = 1;
		TerrainPipelineSettings.ColorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		TerrainPipelineSettings.SrcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		TerrainPipelineSettings.DstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		TerrainPipelineSettings.ColorBlendOp = VK_BLEND_OP_ADD;
		TerrainPipelineSettings.SrcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		TerrainPipelineSettings.DstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		TerrainPipelineSettings.AlphaBlendOp = VK_BLEND_OP_ADD;
		// Depth testing
		TerrainPipelineSettings.DepthTestEnable = VK_TRUE;
		TerrainPipelineSettings.DepthWriteEnable = VK_TRUE;
		TerrainPipelineSettings.DepthCompareOp = VK_COMPARE_OP_LESS;
		TerrainPipelineSettings.DepthBoundsTestEnable = VK_FALSE;
		TerrainPipelineSettings.StencilTestEnable = VK_FALSE;

		Pipeline.Pipeline = VulkanInterface::BatchPipelineCreation(Shaders, ShaderCount, VertexInputBinding, VertexInputCount,
			&TerrainPipelineSettings, &ResourceInfo);

		LoadTerrain();
	}

	void DeInit()
	{
		VulkanInterface::DestroySampler(Sampler);
		VulkanInterface::DestroyUniformLayout(SamplerLayout);
		VulkanInterface::DestroyPipelineLayout(Pipeline.PipelineLayout);
		VulkanInterface::DestroyPipeline(Pipeline.Pipeline);
	}

	void OnDraw()
	{
		const VkDescriptorSet Sets[] = {
			FrameManager::GetViewProjectionSet(),
			TextureSet
		};

		const u32 TerrainDescriptorSetGroupCount = sizeof(Sets) / sizeof(Sets[0]);
		const u32 DynamicOffset = VulkanInterface::TestGetImageIndex() * sizeof(FrameManager::ViewProjectionBuffer);

		Render::BindPipeline(Pipeline.Pipeline);
		Render::BindDescriptorSet(Sets, TerrainDescriptorSetGroupCount, Pipeline.PipelineLayout, 0, &DynamicOffset, 1);
		Render::BindVertexBuffer(VertexOffset);
		Render::BindIndexBuffer(IndexOffset);
		Render::DrawIndexed(IndicesCount);
	}

	void LoadTerrain()
	{
		std::vector<u32> TerrainIndices;
		GenerateTerrain(TerrainIndices);

		IndicesCount = TerrainIndices.size();

		VertexOffset = Render::LoadVertices(&TerrainVerticesData[0][0], sizeof(TerrainVertex), NumRows * NumCols);
		IndexOffset = Render::LoadIndices(TerrainIndices.data(), IndicesCount);
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




	void TestAttachSkyNoxTerrainTexture(VkImageView DefuseImage, VkDescriptorSet* SetToAttach)
	{
		VulkanInterface::CreateUniformSets(&SamplerLayout, 1, SetToAttach);

		VulkanInterface::UniformSetAttachmentInfo SetInfo[1];
		SetInfo[0].ImageInfo.imageView = DefuseImage;
		SetInfo[0].ImageInfo.sampler = Sampler;
		SetInfo[0].ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		SetInfo[0].Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

		VulkanInterface::AttachUniformsToSet(*SetToAttach, SetInfo, 1);
	}
}
