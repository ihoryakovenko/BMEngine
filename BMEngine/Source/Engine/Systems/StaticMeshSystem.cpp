#include "StaticMeshSystem.h"

#include "Render/VulkanInterface/VulkanInterface.h"
#include "Render/Render.h"
#include "Render/FrameManager.h"
#include "Render/RenderResourceManger.h"

#include "Util/Settings.h"
#include "Util/Util.h"

#include "ResourceManager.h"
#include "Engine/Scene.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

VkDescriptorSet TestMaterial;
VkDescriptorSet WhiteMaterial;
VkDescriptorSet ContainerMaterial;
VkDescriptorSet BlendWindowMaterial;
VkDescriptorSet GrassMaterial;
VkDescriptorSet SkyBoxMaterial;
VkDescriptorSet TerrainMaterial;

namespace StaticMeshSystem
{
	static void OnDraw();

	static void LoadModel(const char* ModelPath, glm::mat4 Model, VkDescriptorSet CustomMaterial = nullptr);

	static const char ModelsBaseDir[] = "./Resources/Models/";

	static VulkanInterface::RenderPipeline Pipeline;

	static VkSampler DiffuseSampler;
	static VkSampler SpecularSampler;
	static VkSampler ShadowMapArraySampler;

	static VkDescriptorSetLayout StaticMeshLightLayout;
	static VkDescriptorSetLayout StaticMeshSamplerLayout;
	static VkDescriptorSetLayout StaticMeshMaterialLayout;
	static VkDescriptorSetLayout ShadowMapArrayLayout;

	static VulkanInterface::UniformBuffer MaterialBuffer;
	static FrameManager::UniformMemoryHnadle EntityLightBufferHandle;
	
	static VkImageView ShadowMapArrayImageInterface[VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT];

	static VkPushConstantRange PushConstants;

	static VkDescriptorSet StaticMeshLightSet;
	static VkDescriptorSet MaterialSet;
	static VkDescriptorSet ShadowMapArraySet[VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT];

	void Init()
	{
		Render::AddDrawFunction(OnDraw);

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

		VkSamplerCreateInfo SpecularSamplerCreateInfo = DiffuseSamplerCreateInfo;
		DiffuseSamplerCreateInfo.maxAnisotropy = 1;

		VkSamplerCreateInfo ShadowMapSamplerCreateInfo = { };
		ShadowMapSamplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		ShadowMapSamplerCreateInfo.magFilter = VK_FILTER_LINEAR;						// How to render when image is magnified on screen
		ShadowMapSamplerCreateInfo.minFilter = VK_FILTER_LINEAR;						// How to render when image is minified on screen
		ShadowMapSamplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;	// How to handle texture wrap in U (x) direction
		ShadowMapSamplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;	// How to handle texture wrap in V (y) direction
		ShadowMapSamplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;	// How to handle texture wrap in W (z) direction
		ShadowMapSamplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;	// Border beyond texture (only workds for border clamp)
		ShadowMapSamplerCreateInfo.unnormalizedCoordinates = VK_FALSE;				// Whether coords should be normalized (between 0 and 1)
		ShadowMapSamplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;		// Mipmap interpolation mode
		ShadowMapSamplerCreateInfo.mipLodBias = 0.0f;								// Level of Details bias for mip level
		ShadowMapSamplerCreateInfo.minLod = 0.0f;									// Minimum Level of Detail to pick mip level
		ShadowMapSamplerCreateInfo.maxLod = 0.0f;									// Maximum Level of Detail to pick mip level
		ShadowMapSamplerCreateInfo.anisotropyEnable = VK_TRUE;
		ShadowMapSamplerCreateInfo.maxAnisotropy = 1; // Todo: support in config

		DiffuseSampler = VulkanInterface::CreateSampler(&DiffuseSamplerCreateInfo);
		SpecularSampler = VulkanInterface::CreateSampler(&SpecularSamplerCreateInfo);
		ShadowMapArraySampler = VulkanInterface::CreateSampler(&ShadowMapSamplerCreateInfo);


		const VkDeviceSize LightBufferSize = sizeof(Render::LightBuffer);
		StaticMeshLightLayout = FrameManager::CreateCompatibleLayout(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
		EntityLightBufferHandle = FrameManager::ReserveUniformMemory(LightBufferSize);
		StaticMeshLightSet = FrameManager::CreateAndBindSet(EntityLightBufferHandle, LightBufferSize, StaticMeshLightLayout);

		const VkShaderStageFlags EntitySamplerInputFlags[2] = { VK_SHADER_STAGE_FRAGMENT_BIT, VK_SHADER_STAGE_FRAGMENT_BIT };
		const VkDescriptorType EntitySamplerDescriptorType[2] = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER };
		StaticMeshSamplerLayout = VulkanInterface::CreateUniformLayout(EntitySamplerDescriptorType, EntitySamplerInputFlags, 2);
	
		const VkDeviceSize MaterialSize = sizeof(Material);
		VkBufferCreateInfo MaterialBufferInfo = { };
		MaterialBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		MaterialBufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		MaterialBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		MaterialBufferInfo.size = MaterialSize;
		MaterialBuffer = VulkanInterface::CreateUniformBufferInternal(&MaterialBufferInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		VulkanInterface::UniformSetAttachmentInfo MaterialAttachmentInfo;
		MaterialAttachmentInfo.BufferInfo.buffer = MaterialBuffer.Buffer;
		MaterialAttachmentInfo.BufferInfo.offset = 0;
		MaterialAttachmentInfo.BufferInfo.range = MaterialSize;
		MaterialAttachmentInfo.Type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

		const VkDescriptorType MaterialDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		const VkShaderStageFlags MaterialStageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		StaticMeshMaterialLayout = VulkanInterface::CreateUniformLayout(&MaterialDescriptorType, &MaterialStageFlags, 1);
		VulkanInterface::CreateUniformSets(&StaticMeshMaterialLayout, 1, &MaterialSet);
		VulkanInterface::AttachUniformsToSet(MaterialSet, &MaterialAttachmentInfo, 1);

		const VkDescriptorType ShadowMapArrayDescriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		const VkShaderStageFlags ShadowMapArrayFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		ShadowMapArrayLayout = VulkanInterface::CreateUniformLayout(&ShadowMapArrayDescriptorType, &ShadowMapArrayFlags, 1);

		VulkanInterface::UniformImageInterfaceCreateInfo ShadowMapArrayInterfaceCreateInfo = { };
		ShadowMapArrayInterfaceCreateInfo.Flags = 0; // No flags
		ShadowMapArrayInterfaceCreateInfo.ViewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		ShadowMapArrayInterfaceCreateInfo.Format = DepthFormat;
		ShadowMapArrayInterfaceCreateInfo.Components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		ShadowMapArrayInterfaceCreateInfo.Components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		ShadowMapArrayInterfaceCreateInfo.Components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		ShadowMapArrayInterfaceCreateInfo.Components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		ShadowMapArrayInterfaceCreateInfo.SubresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		ShadowMapArrayInterfaceCreateInfo.SubresourceRange.baseMipLevel = 0;
		ShadowMapArrayInterfaceCreateInfo.SubresourceRange.levelCount = 1;
		ShadowMapArrayInterfaceCreateInfo.SubresourceRange.baseArrayLayer = 0;
		ShadowMapArrayInterfaceCreateInfo.SubresourceRange.layerCount = 2;

		for (u32 i = 0; i < VulkanInterface::GetImageCount(); i++)
		{
			ShadowMapArrayImageInterface[i] = VulkanInterface::CreateImageInterface(&ShadowMapArrayInterfaceCreateInfo,
				Render::TmpGetShadowMapArray(i));

			VulkanInterface::UniformSetAttachmentInfo ShadowMapArrayAttachmentInfo;
			ShadowMapArrayAttachmentInfo.ImageInfo.imageView = ShadowMapArrayImageInterface[i];
			ShadowMapArrayAttachmentInfo.ImageInfo.sampler = ShadowMapArraySampler;
			ShadowMapArrayAttachmentInfo.ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			ShadowMapArrayAttachmentInfo.Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

			VulkanInterface::CreateUniformSets(&ShadowMapArrayLayout, 1, ShadowMapArraySet + i);
			VulkanInterface::AttachUniformsToSet(ShadowMapArraySet[i], &ShadowMapArrayAttachmentInfo, 1);
		}

		VkDescriptorSetLayout StaticMeshDescriptorLayouts[] =
		{
			FrameManager::GetViewProjectionLayout(),
			StaticMeshLightLayout,
			StaticMeshSamplerLayout,
			StaticMeshMaterialLayout,
			ShadowMapArrayLayout
		};

		const u32 StaticMeshDescriptorLayoutCount = sizeof(StaticMeshDescriptorLayouts) / sizeof(StaticMeshDescriptorLayouts[0]);

		PushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		PushConstants.offset = 0;
		// Todo: check constant and model size?
		PushConstants.size = sizeof(Render::BMRModel);

		Pipeline.PipelineLayout = VulkanInterface::CreatePipelineLayout(StaticMeshDescriptorLayouts, StaticMeshDescriptorLayoutCount, &PushConstants, 1);

		const u32 ShaderCount = 2;
		VulkanInterface::Shader Shaders[ShaderCount];

		std::vector<char> VertexShaderCode;
		Util::OpenAndReadFileFull("./Resources/Shaders/vert.spv", VertexShaderCode, "rb");
		std::vector<char> FragmentShaderCode;
		Util::OpenAndReadFileFull("./Resources/Shaders/frag.spv", FragmentShaderCode, "rb");

		Shaders[0].Stage = VK_SHADER_STAGE_VERTEX_BIT;
		Shaders[0].Code = VertexShaderCode.data();
		Shaders[0].CodeSize = VertexShaderCode.size();

		Shaders[1].Stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		Shaders[1].Code = FragmentShaderCode.data();
		Shaders[1].CodeSize = FragmentShaderCode.size();

		VulkanInterface::BMRVertexInputBinding VertexInputBinding[1];
		VertexInputBinding[0].InputAttributes[0] = { "Position", VK_FORMAT_R32G32B32_SFLOAT, offsetof(StaticMeshVertex, Position) };
		VertexInputBinding[0].InputAttributes[1] = { "TextureCoords", VK_FORMAT_R32G32_SFLOAT, offsetof(StaticMeshVertex, TextureCoords) };
		VertexInputBinding[0].InputAttributes[2] = { "Normal", VK_FORMAT_R32G32B32_SFLOAT, offsetof(StaticMeshVertex, Normal) };
		VertexInputBinding[0].InputAttributesCount = 3;
		VertexInputBinding[0].InputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		VertexInputBinding[0].Stride = sizeof(StaticMeshVertex);
		VertexInputBinding[0].VertexInputBindingName = "EntityVertex";

		VulkanInterface::PipelineSettings PipelineSettings;
		Util::LoadPipelineSettings(PipelineSettings, "./Resources/Settings/StaticMeshSystem.ini");
		PipelineSettings.Extent = MainScreenExtent;

		VulkanInterface::PipelineResourceInfo ResourceInfo;
		ResourceInfo.PipelineLayout = Pipeline.PipelineLayout;
		ResourceInfo.RenderPass = Render::TestGetRenderPass();
		ResourceInfo.SubpassIndex = 0;

		Pipeline.Pipeline = VulkanInterface::BatchPipelineCreation(Shaders, ShaderCount, VertexInputBinding, 1,
			&PipelineSettings, &ResourceInfo);










		auto test = *ResourceManager::FindTexture("1giraffe.jpg");

		//ResourceManager::CreateEntityMaterial("TestMaterial", TestTexture.ImageView, TestTexture.ImageView, &TestMaterial);
		//ResourceManager::CreateEntityMaterial("WhiteMaterial", WhiteTexture.ImageView, WhiteTexture.ImageView, &WhiteMaterial);
		//ResourceManager::CreateEntityMaterial("ContainerMaterial", ContainerTexture.ImageView, ContainerSpecularTexture.ImageView, &ContainerMaterial);
		//ResourceManager::CreateEntityMaterial("BlendWindowMaterial", BlendWindow.ImageView, BlendWindow.ImageView, &BlendWindowMaterial);
		//ResourceManager::CreateEntityMaterial("GrassMaterial", GrassTexture.ImageView, GrassTexture.ImageView, &GrassMaterial);

		AttachTextureToStaticMesh(test.ImageView, test.ImageView, &TestMaterial);
		AttachTextureToStaticMesh(test.ImageView, test.ImageView, &WhiteMaterial);
		AttachTextureToStaticMesh(test.ImageView, test.ImageView, &ContainerMaterial);
		AttachTextureToStaticMesh(test.ImageView, test.ImageView, &BlendWindowMaterial);
		AttachTextureToStaticMesh(test.ImageView, test.ImageView, &GrassMaterial);

		Util::Model3DData ModelData = Util::LoadModel3DData(".\\Resources\\Models\\uh60.model");
		Util::Model3D Uh60Model = Util::ParseModel3D(ModelData);

		u64 ModelVertexByteOffset = 0;
		u32 ModelIndexCountOffset = 0;
		for (u32 i = 0; i < Uh60Model.MeshCount; i++)
		{
			const u64 VerticesCount = Uh60Model.VerticesCounts[i];
			const u32 IndicesCount = Uh60Model.IndicesCounts[i];

			const Render::RenderTexture* Texture = ResourceManager::FindTexture(Uh60Model.DiffuseTexturesHashes[i]);
			
			// TODO DELETE!
			VkDescriptorSet Set;
			AttachTextureToStaticMesh(Texture->ImageView, Texture->ImageView, &Set);

			RenderResourceManager::CreateEntity(Uh60Model.VertexData + ModelVertexByteOffset, sizeof(StaticMeshVertex), VerticesCount,
				Uh60Model.IndexData + ModelIndexCountOffset, IndicesCount, Set);

			ModelVertexByteOffset += VerticesCount * sizeof(StaticMeshVertex);
			ModelIndexCountOffset += IndicesCount;
		}


		Util::ClearModel3DData(ModelData);

		{
			glm::vec3 CubePos(0.0f, -5.0f, 0.0f);
			glm::mat4 Model = glm::mat4(1.0f);
			Model = glm::translate(Model, CubePos);
			Model = glm::scale(Model, glm::vec3(20.0f, 1.0f, 20.0f));
			LoadModel("./Resources/Models/cube.obj", Model, WhiteMaterial);
			//LoadModel("./Resources/Models/cube.obj", Model, ResourceManager::FindMaterial("WhiteMaterial"));
		}

		{
			glm::vec3 CubePos(0.0f, 0.0f, 0.0f);
			glm::mat4 Model = glm::mat4(1.0f);
			Model = glm::translate(Model, CubePos);
			Model = glm::scale(Model, glm::vec3(0.2f, 5.0f, 0.2f));
			LoadModel("./Resources/Models/cube.obj", Model, TestMaterial);
		}

		{
			glm::vec3 CubePos(0.0f, 0.0f, 8.0f);
			glm::mat4 Model = glm::mat4(1.0f);
			Model = glm::translate(Model, CubePos);
			Model = glm::rotate(Model, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			Model = glm::scale(Model, glm::vec3(0.5f));
			LoadModel("./Resources/Models/cube.obj", Model, GrassMaterial);
		}

		{
			glm::vec3 LightCubePos(0.0f, 0.0f, 10.0f);
			glm::mat4 Model = glm::mat4(1.0f);
			Model = glm::translate(Model, LightCubePos);
			Model = glm::scale(Model, glm::vec3(0.2f));
			LoadModel("./Resources/Models/cube.obj", Model, WhiteMaterial);
		}

		{
			glm::vec3 CubePos(0.0f, 0.0f, 15.0f);
			glm::mat4 Model = glm::mat4(1.0f);
			Model = glm::translate(Model, CubePos);
			Model = glm::scale(Model, glm::vec3(1.0f));
			LoadModel("./Resources/Models/cube.obj", Model, WhiteMaterial);
		}

		{
			glm::vec3 CubePos(5.0f, 0.0f, 10.0f);
			glm::mat4 Model = glm::mat4(1.0f);
			Model = glm::translate(Model, CubePos);
			Model = glm::rotate(Model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			Model = glm::scale(Model, glm::vec3(1.0f));
			LoadModel("./Resources/Models/cube.obj", Model, ContainerMaterial);
		}
	}

	void DeInit()
	{
		for (u32 i = 0; i < VulkanInterface::GetImageCount(); i++)
		{
			VulkanInterface::DestroyImageInterface(ShadowMapArrayImageInterface[i]);
		}

		VulkanInterface::DestroyUniformBuffer(MaterialBuffer);

		VulkanInterface::DestroyUniformLayout(StaticMeshLightLayout);
		VulkanInterface::DestroyUniformLayout(StaticMeshMaterialLayout);
		VulkanInterface::DestroyUniformLayout(ShadowMapArrayLayout);
		VulkanInterface::DestroyUniformLayout(StaticMeshSamplerLayout);

		VulkanInterface::DestroySampler(ShadowMapArraySampler);
		VulkanInterface::DestroySampler(DiffuseSampler);
		VulkanInterface::DestroySampler(SpecularSampler);
	}

	void OnDraw()
	{
		FrameManager::UpdateUniformMemory(EntityLightBufferHandle, Scene.LightEntity, sizeof(Render::LightBuffer));

		VkCommandBuffer CmdBuffer = VulkanInterface::GetCommandBuffer();
		vkCmdBindPipeline(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Pipeline);
		
		const VkDescriptorSet VpSet = FrameManager::GetViewProjectionSet();

		const u32 DynamicOffset = VulkanInterface::TestGetImageIndex() * sizeof(FrameManager::ViewProjectionBuffer);
		vkCmdBindDescriptorSets(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.PipelineLayout,
			0, 1, &VpSet, 1, &DynamicOffset);

		const u32 LightDynamicOffset = VulkanInterface::TestGetImageIndex() * sizeof(Render::LightBuffer);

		VulkanInterface::VertexBuffer VertexBuffer = RenderResourceManager::GetVertexBuffer();
		VulkanInterface::IndexBuffer IndexBuffer = RenderResourceManager::GetIndexBuffer();

		for (u32 i = 0; i < Scene.DrawEntitiesCount; ++i)
		{
			RenderResourceManager::DrawEntity* DrawEntity = Scene.DrawEntities + i;

			const VkDescriptorSet DescriptorSetGroup[] =
			{
				StaticMeshLightSet,
				DrawEntity->TextureSet,
				MaterialSet,
				ShadowMapArraySet[VulkanInterface::TestGetImageIndex()],
			};
			const u32 DescriptorSetGroupCount = sizeof(DescriptorSetGroup) / sizeof(DescriptorSetGroup[0]);

			const VkShaderStageFlags Flags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			vkCmdPushConstants(CmdBuffer, Pipeline.PipelineLayout, Flags, 0, sizeof(Render::BMRModel), &DrawEntity->Model);

			vkCmdBindDescriptorSets(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.PipelineLayout,
				1, DescriptorSetGroupCount, DescriptorSetGroup, 1, &LightDynamicOffset);

			vkCmdBindVertexBuffers(CmdBuffer, 0, 1, &VertexBuffer.Buffer, &DrawEntity->VertexOffset);
			vkCmdBindIndexBuffer(CmdBuffer, IndexBuffer.Buffer, DrawEntity->IndexOffset, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(CmdBuffer, DrawEntity->IndicesCount, 1, 0, 0, 0);
		}
	}






	void UpdateMaterialBuffer(const Material* Buffer)
	{
		assert(Buffer);
		VulkanInterface::UpdateUniformBuffer(MaterialBuffer, sizeof(Material), 0,
			Buffer);
	}

	void AttachTextureToStaticMesh(VkImageView DefuseImage, VkImageView SpecularImage, VkDescriptorSet* SetToAttach)
	{
		VulkanInterface::CreateUniformSets(&StaticMeshSamplerLayout, 1, SetToAttach);

		VulkanInterface::UniformSetAttachmentInfo SetInfo[2];
		SetInfo[0].ImageInfo.imageView = DefuseImage;
		SetInfo[0].ImageInfo.sampler = DiffuseSampler;
		SetInfo[0].ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		SetInfo[0].Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

		SetInfo[1].ImageInfo.imageView = SpecularImage;
		SetInfo[1].ImageInfo.sampler = SpecularSampler;
		SetInfo[1].ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		SetInfo[1].Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

		VulkanInterface::AttachUniformsToSet(*SetToAttach, SetInfo, 2);
	}



	void LoadModel(const char* ModelPath, glm::mat4 Model, VkDescriptorSet CustomMaterial)
	{
		

	}
}
