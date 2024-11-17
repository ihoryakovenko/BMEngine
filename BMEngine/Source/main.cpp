#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>

#define STB_IMAGE_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION


#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "Util/Util.h"
#include <cassert>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include "BMR/BMRInterface.h"
#include <tiny_obj_loader.h>
#include <unordered_map>
#include <random>
#include "Memory/MemoryManagmentSystem.h"
#include "Util/EngineTypes.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include "stb_image.h"

#include "ImguiIntegration.h"

#include <thread>
#include "Util/Settings.h"

struct Texture
{
	BMR::BMRUniform UniformData;
	VkImageView ImageView = nullptr;
};

const u32 NumRows = 600;
const u32 NumCols = 600;

Texture TestTextureIndex;
Texture WhiteTextureIndex;
Texture ContainerTextureIndex;
Texture ContainerSpecularTextureIndex;
Texture BlendWindowIndex;
Texture GrassTextureIndex;
Texture SkyBoxCubeTextureIndex;

VkDescriptorSet TestMaterialIndex;
VkDescriptorSet WhiteMaterialIndex;
VkDescriptorSet ContainerMaterialIndex;
VkDescriptorSet BlendWindowMaterial;
VkDescriptorSet GrassMaterial;
VkDescriptorSet SkyBoxMaterial;
VkDescriptorSet TerrainMaterial;

TerrainVertex TerrainVerticesData[NumRows][NumCols];

std::vector<BMR::BMRDrawEntity> DrawEntities;
BMR::BMRConfig Config;
std::vector<std::vector<char>> ShaderCodes(BMR::BMRShaderNames::ShaderNamesCount);
BMR::BMRDrawSkyBoxEntity SkyBox;


VkSampler ShadowMapSampler;
VkSampler DiffuseSampler;
VkSampler SpecularSampler;

VkDescriptorSetLayout EntitySamplerLayout;

std::vector<Texture> Textures;

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

	Indices.reserve(NumRows * NumCols);
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

TerrainVertex* TerrainVerticesDataPointer = &(TerrainVerticesData[0][0]);
u32 TerrainVerticesCount = NumRows * NumCols;

struct TestMesh
{
	std::vector<EntityVertex> vertices;
	std::vector<u32> indices;
	VkDescriptorSet MaterialIndex;
	glm::mat4 Model = glm::mat4(1.0f);
};

struct SkyBoxMesh
{
	std::vector<SkyBoxVertex> vertices;
	std::vector<u32> indices;
	VkDescriptorSet MaterialIndex;
};

namespace std
{
	template<> struct hash<EntityVertex>
	{
		size_t operator()(EntityVertex const& vertex) const
		{
			size_t hashPosition = std::hash<glm::vec3>()(vertex.Position);
			size_t hashColor = std::hash<glm::vec3>()(vertex.Color);
			size_t hashTextureCoords = std::hash<glm::vec2>()(vertex.TextureCoords);
			size_t hashNormal = std::hash<glm::vec3>()(vertex.Normal);

			size_t combinedHash = hashPosition;
			combinedHash ^= (hashColor << 1);
			combinedHash ^= (hashTextureCoords << 1);
			combinedHash ^= (hashNormal << 1);

			return combinedHash;
		}
	};

	template<> struct hash<SkyBoxVertex>
	{
		size_t operator()(SkyBoxVertex const& vertex) const
		{
			size_t hashPosition = std::hash<glm::vec3>()(vertex.Position);
			size_t combinedHash = hashPosition;
			return combinedHash;
		}
	};
}

struct VertexEqual
{
	bool operator()(const EntityVertex& lhs, const EntityVertex& rhs) const
	{
		return lhs.Position == rhs.Position && lhs.Color == rhs.Color && lhs.TextureCoords == rhs.TextureCoords;
	}

	bool operator()(const SkyBoxVertex& lhs, const SkyBoxVertex& rhs) const
	{
		return lhs.Position == rhs.Position;
	}
};

struct Camera
{
	glm::vec3 CameraPosition = glm::vec3(0.0f, 0.0f, 20.0f);
	glm::vec3 CameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 CameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
};

template <typename... TArgs>
static void Error(std::string_view Message, TArgs&&... Args)
{
	std::cout << "\033[31;5m"; // Set red color
	Print("Error", Message, Args...);
	std::cout << "\033[m"; // Reset red color
}

template <typename... TArgs>
static void Warning(std::string_view Message, TArgs&&... Args)
{
	std::cout << "\033[33;5m";
	Print("Warning", Message, Args...);
	std::cout << "\033[m";
}

void BMRLog(BMR::BMRLogType LogType, const char* Format, va_list Args)
{
	switch (LogType)
	{
		case BMR::LogType_Error:
		{
			std::cout << "\033[31;5mError: "; // Set red color
			vprintf(Format, Args);
			std::cout << "\n\033[m"; // Reset red color
			assert(false);
			break;
		}
		case BMR::LogType_Warning:
		{
			std::cout << "\033[33;5mWarning: "; // Set red color
			vprintf(Format, Args);
			std::cout << "\n\033[m"; // Reset red color
			break;
		}
		case BMR::LogType_Info:
		{
			std::cout << "Info: ";
			vprintf(Format, Args);
			std::cout << '\n';
			break;
		}
	}
}

Texture AddTexture(const char* DiffuseTexturePath)
{
	int Width;
	int Height;
	int Channels;

	stbi_uc* ImageData = stbi_load(DiffuseTexturePath, &Width, &Height, &Channels, STBI_rgb_alpha);

	if (ImageData == nullptr)
	{
		assert(false);
	}

	BMR::BMRTextureArrayInfo Info;
	Info.Width = Width;
	Info.Height = Height;
	Info.Format = STBI_rgb_alpha;
	Info.LayersCount = 1;
	Info.Data = &ImageData;

	VkImageCreateInfo ImageCreateInfo;
	ImageCreateInfo = { };
	ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	ImageCreateInfo.extent.width = Info.Width;
	ImageCreateInfo.extent.height = Info.Height;
	ImageCreateInfo.extent.depth = 1;
	ImageCreateInfo.mipLevels = 1;
	ImageCreateInfo.arrayLayers = Info.LayersCount;
	ImageCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	ImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	ImageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	BMR::BMRUniform TextureUniform = BMR::CreateUniformImage(&ImageCreateInfo);
	BMR::CopyDataToImage(TextureUniform.Image, Info.Width, Info.Height, Info.Format, Info.LayersCount, Info.Data);

	stbi_image_free(ImageData);

	BMR::BMRUniformImageInterfaceCreateInfo InterfaceCreateInfo = { };
	InterfaceCreateInfo.Flags = 0;
	InterfaceCreateInfo.ViewType = VK_IMAGE_VIEW_TYPE_2D;
	InterfaceCreateInfo.Format = VK_FORMAT_R8G8B8A8_SRGB;
	InterfaceCreateInfo.Components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	InterfaceCreateInfo.Components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	InterfaceCreateInfo.Components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	InterfaceCreateInfo.Components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	InterfaceCreateInfo.SubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	InterfaceCreateInfo.SubresourceRange.baseMipLevel = 0;
	InterfaceCreateInfo.SubresourceRange.levelCount = 1;
	InterfaceCreateInfo.SubresourceRange.baseArrayLayer = 0;
	InterfaceCreateInfo.SubresourceRange.layerCount = 1;

	VkImageView TextureInterface = BMR::CreateImageInterface(&InterfaceCreateInfo, TextureUniform.Image);

	Texture Tex;
	Tex.UniformData = TextureUniform;
	Tex.ImageView = TextureInterface;
	return Tex;
}

Texture LoadCubeTexture()
{
	const u32 texCount = 6;

	const char* CubeTexturePaths[texCount] = 
	{
		"./Resources/Textures/skybox/right.jpg",
		"./Resources/Textures/skybox/left.jpg",
		"./Resources/Textures/skybox/top.jpg",
		"./Resources/Textures/skybox/bottom.jpg",
		"./Resources/Textures/skybox/front.jpg",
		"./Resources/Textures/skybox/back.jpg",
	};

	stbi_uc* CubeTexture[texCount];

	int Width;
	int Height;
	int Channels;

	for (u32 i = 0; i < texCount; ++i)
	{
		CubeTexture[i] = stbi_load(CubeTexturePaths[i], &Width, &Height, &Channels, STBI_rgb_alpha);

		if (CubeTexture[i] == nullptr)
		{
			assert(false);
		}
	}

	BMR::BMRTextureArrayInfo Info;
	Info.Width = Width;
	Info.Height = Height;
	Info.Format = STBI_rgb_alpha;
	Info.LayersCount = texCount;
	Info.Data = CubeTexture;

	VkImageCreateInfo ImageCreateInfo;
	ImageCreateInfo = { };
	ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	ImageCreateInfo.extent.width = Info.Width;
	ImageCreateInfo.extent.height = Info.Height;
	ImageCreateInfo.extent.depth = 1;
	ImageCreateInfo.mipLevels = 1;
	ImageCreateInfo.arrayLayers = Info.LayersCount;
	ImageCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	ImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	ImageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	ImageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

	BMR::BMRUniform TextureUniform = BMR::CreateUniformImage(&ImageCreateInfo);
	BMR::CopyDataToImage(TextureUniform.Image, Info.Width, Info.Height, Info.Format, Info.LayersCount, Info.Data);

	for (u32 i = 0; i < texCount; ++i)
	{
		stbi_image_free(CubeTexture[i]);
	};

	BMR::BMRUniformImageInterfaceCreateInfo InterfaceCreateInfo = { };
	InterfaceCreateInfo.Flags = 0;
	InterfaceCreateInfo.ViewType = VK_IMAGE_VIEW_TYPE_CUBE;
	InterfaceCreateInfo.Format = VK_FORMAT_R8G8B8A8_SRGB;
	InterfaceCreateInfo.Components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	InterfaceCreateInfo.Components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	InterfaceCreateInfo.Components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	InterfaceCreateInfo.Components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	InterfaceCreateInfo.SubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	InterfaceCreateInfo.SubresourceRange.baseMipLevel = 0;
	InterfaceCreateInfo.SubresourceRange.levelCount = 1;
	InterfaceCreateInfo.SubresourceRange.baseArrayLayer = 0;
	InterfaceCreateInfo.SubresourceRange.layerCount = Info.LayersCount;

	VkImageView TextureInterface = BMR::CreateImageInterface(&InterfaceCreateInfo, TextureUniform.Image);

	Texture Tex;
	Tex.UniformData = TextureUniform;
	Tex.ImageView = TextureInterface;
	return Tex;
}

void WindowCloseCallback(GLFWwindow* Window)
{
	glfwDestroyWindow(Window);
}

bool Close = false;

static bool FirstMouse = true;
static f32 LastX = 400, LastY = 300;
f32 Yaw = -90.0f;
f32 Pitch = 0.0f;

void MoveCamera(GLFWwindow* Window, f32 DeltaTime, Camera& MainCamera)
{
	const f32 RotationSpeed = 0.1f;
	const f32 CameraSpeed = 10.0f;
	const f32 CameraDeltaSpeed = CameraSpeed * DeltaTime;

	// Handle camera movement with keys
	glm::vec3 movement = glm::vec3(0.0f);
	if (glfwGetKey(Window, GLFW_KEY_W) == GLFW_PRESS) movement += MainCamera.CameraFront;
	if (glfwGetKey(Window, GLFW_KEY_S) == GLFW_PRESS) movement -= MainCamera.CameraFront;
	if (glfwGetKey(Window, GLFW_KEY_A) == GLFW_PRESS) movement -= glm::normalize(glm::cross(MainCamera.CameraFront, MainCamera.CameraUp));
	if (glfwGetKey(Window, GLFW_KEY_D) == GLFW_PRESS) movement += glm::normalize(glm::cross(MainCamera.CameraFront, MainCamera.CameraUp));
	if (glfwGetKey(Window, GLFW_KEY_SPACE) == GLFW_PRESS) movement += MainCamera.CameraUp;
	if (glfwGetKey(Window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) movement -= MainCamera.CameraUp;

	MainCamera.CameraPosition += movement * CameraDeltaSpeed;

	if (glfwGetKey(Window, GLFW_KEY_ESCAPE) == GLFW_PRESS) Close = true;

	f64 MouseX, MouseY;
	glfwGetCursorPos(Window, &MouseX, &MouseY);

	if (FirstMouse)
	{
		LastX = MouseX;
		LastY = MouseY;
		FirstMouse = false;
	}

	f32 OffsetX = MouseX - LastX;
	f32 OffsetY = LastY - MouseY;
	LastX = MouseX;
	LastY = MouseY;

	Yaw += OffsetX * RotationSpeed;
	Pitch += OffsetY * RotationSpeed;

	Pitch = glm::clamp(Pitch, -89.0f, 89.0f);

	glm::vec3 Front;
	Front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
	Front.y = sin(glm::radians(Pitch));
	Front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
	MainCamera.CameraFront = glm::normalize(Front);
}

TestMesh CreateCubeMesh(const char* Modelpath, VkDescriptorSet materialIndex)
{
	TestMesh cube;

	const char* BaseDir = "./Resources/Models/";

	tinyobj::attrib_t Attrib;
	std::vector<tinyobj::shape_t> Shapes;
	std::vector<tinyobj::material_t> Materials;
	std::string Warn, Err;

	if (!tinyobj::LoadObj(&Attrib, &Shapes, &Materials, &Warn, &Err, Modelpath, BaseDir))
	{
		assert(false);
	}

	std::unordered_map<EntityVertex, u32, std::hash<EntityVertex>, VertexEqual> uniqueVertices{ };

	for (const auto& Shape : Shapes)
	{
		for (const auto& index : Shape.mesh.indices)
		{
			EntityVertex vertex{ };

			vertex.Position =
			{
				Attrib.vertices[3 * index.vertex_index + 0],
				Attrib.vertices[3 * index.vertex_index + 1],
				Attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.TextureCoords =
			{
				Attrib.texcoords[2 * index.texcoord_index + 0],
				Attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vertex.Color = { 1.0f, 1.0f, 1.0f };

			if (index.normal_index >= 0)
			{
				vertex.Normal =
				{
					Attrib.normals[3 * index.normal_index + 0],
					Attrib.normals[3 * index.normal_index + 1],
					Attrib.normals[3 * index.normal_index + 2]
				};
			}
			else
			{
				assert(false);
				// Fallback if normal is not available (optional)
				vertex.Normal = { 0.0f, 1.0f, 0.0f }; // Default to up-direction, or compute it later
			}

			if (!uniqueVertices.contains(vertex))
			{
				uniqueVertices[vertex] = static_cast<u32>(cube.vertices.size());
				cube.vertices.push_back(vertex);
			}

			cube.indices.push_back(uniqueVertices[vertex]);
		}

		cube.MaterialIndex = materialIndex;
	}

	return cube;
}

SkyBoxMesh CreateSkyBoxMesh(const char* Modelpath, VkDescriptorSet materialIndex)
{
	SkyBoxMesh cube;

	const char* BaseDir = "./Resources/Models/";

	tinyobj::attrib_t Attrib;
	std::vector<tinyobj::shape_t> Shapes;
	std::vector<tinyobj::material_t> Materials;
	std::string Warn, Err;

	if (!tinyobj::LoadObj(&Attrib, &Shapes, &Materials, &Warn, &Err, Modelpath, BaseDir))
	{
		assert(false);
	}

	std::unordered_map<SkyBoxVertex, u32, std::hash<SkyBoxVertex>, VertexEqual> uniqueVertices{ };

	for (const auto& Shape : Shapes)
	{
		for (const auto& index : Shape.mesh.indices)
		{
			SkyBoxVertex vertex{ };

			vertex.Position =
			{
				Attrib.vertices[3 * index.vertex_index + 0],
				Attrib.vertices[3 * index.vertex_index + 1],
				Attrib.vertices[3 * index.vertex_index + 2]
			};

			if (!uniqueVertices.contains(vertex))
			{
				uniqueVertices[vertex] = static_cast<u32>(cube.vertices.size());
				cube.vertices.push_back(vertex);
			}

			cube.indices.push_back(uniqueVertices[vertex]);
		}

		cube.MaterialIndex = materialIndex;
	}

	return cube;
}

void LoadDrawEntities()
{
	const char* Grass = "./Resources/Textures/grass.png";
	const char* BlendWindow = "./Resources/Textures/blending_transparent_window.png";
	const char* ContainerSpecularTexture = "./Resources/Textures/container2_specular.png";
	const char* ContainerTexture = "./Resources/Textures/container2.png";
	const char* WhiteTexture = "./Resources/Textures/White.png";
	const char* TestTexture = "./Resources/Textures/1giraffe.jpg";
	const char* Modelpath = "./Resources/Models/uh60.obj";
	const char* CubeObj = "./Resources/Models/cube.obj";
	const char* SkyBoxObj = "./Resources/Models/SkyBox.obj";
	const char* BaseDir = "./Resources/Models/";

	TestTextureIndex = AddTexture(TestTexture);
	WhiteTextureIndex = AddTexture(WhiteTexture);
	ContainerTextureIndex = AddTexture(ContainerTexture);
	ContainerSpecularTextureIndex = AddTexture(ContainerSpecularTexture);
	BlendWindowIndex = AddTexture(BlendWindow);
	GrassTextureIndex = AddTexture(Grass);

	SkyBoxCubeTextureIndex = LoadCubeTexture();



	BMR::BMRUniformSetAttachmentInfo SetInfo[2];
	SetInfo[0].ImageInfo.imageView = TestTextureIndex.ImageView;
	SetInfo[0].ImageInfo.sampler = DiffuseSampler;
	SetInfo[0].ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	SetInfo[0].Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	SetInfo[1].ImageInfo.imageView = TestTextureIndex.ImageView;
	SetInfo[1].ImageInfo.sampler = SpecularSampler;
	SetInfo[1].ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	SetInfo[1].Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	BMR::AttachUniformsToSet(TestMaterialIndex, SetInfo, 2);

	SetInfo[0].ImageInfo.imageView = WhiteTextureIndex.ImageView;
	SetInfo[1].ImageInfo.imageView = WhiteTextureIndex.ImageView;
	BMR::AttachUniformsToSet(WhiteMaterialIndex, SetInfo, 2);

	SetInfo[0].ImageInfo.imageView = ContainerTextureIndex.ImageView;
	SetInfo[1].ImageInfo.imageView = ContainerSpecularTextureIndex.ImageView;
	BMR::AttachUniformsToSet(ContainerMaterialIndex, SetInfo, 2);

	SetInfo[0].ImageInfo.imageView = BlendWindowIndex.ImageView;
	SetInfo[1].ImageInfo.imageView = BlendWindowIndex.ImageView;
	BMR::AttachUniformsToSet(BlendWindowMaterial, SetInfo, 2);

	SetInfo[0].ImageInfo.imageView = GrassTextureIndex.ImageView;
	SetInfo[1].ImageInfo.imageView = GrassTextureIndex.ImageView;
	BMR::AttachUniformsToSet(GrassMaterial, SetInfo, 2);

	SetInfo[0].ImageInfo.imageView = SkyBoxCubeTextureIndex.ImageView;
	BMR::AttachUniformsToSet(SkyBoxMaterial, SetInfo, 1);

	SetInfo[0].ImageInfo.imageView = TestTextureIndex.ImageView;
	BMR::AttachUniformsToSet(TerrainMaterial, SetInfo, 1);

	tinyobj::attrib_t Attrib;
	std::vector<tinyobj::shape_t> Shapes;
	std::vector<tinyobj::material_t> Materials;
	std::string Warn, Err;

	if (!tinyobj::LoadObj(&Attrib, &Shapes, &Materials, &Warn, &Err, Modelpath, BaseDir))
	{
		assert(false);
	}

	std::vector<VkDescriptorSet> MaterialToTexture(Materials.size());

	for (size_t i = 0; i < Materials.size(); i++)
	{
		const tinyobj::material_t& Material = Materials[i];
		if (!Material.diffuse_texname.empty())
		{
			int Idx = Material.diffuse_texname.rfind("\\");
			std::string FileName = "./Resources/Textures/" + Material.diffuse_texname.substr(Idx + 1);

			const Texture NewTextureIndex = AddTexture(FileName.c_str());

			BMR::CreateUniformSets(&EntitySamplerLayout, 1, &MaterialToTexture[i]);

			BMR::BMRUniformSetAttachmentInfo SetInfo[2];
			SetInfo[0].ImageInfo.imageView = NewTextureIndex.ImageView;
			SetInfo[0].ImageInfo.sampler = DiffuseSampler;
			SetInfo[0].ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			SetInfo[0].Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

			SetInfo[1].ImageInfo.imageView = NewTextureIndex.ImageView;
			SetInfo[1].ImageInfo.sampler = SpecularSampler;
			SetInfo[1].ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			SetInfo[1].Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

			BMR::AttachUniformsToSet(MaterialToTexture[i], SetInfo, 2);
		}
		else
		{
			MaterialToTexture[i] = BlendWindowMaterial;
		}
	}

	std::vector<TestMesh> ModelMeshes;
	ModelMeshes.reserve(Shapes.size());

	std::unordered_map<EntityVertex, u32, std::hash<EntityVertex>, VertexEqual> uniqueVertices{ };

	for (const auto& Shape : Shapes)
	{
		TestMesh Tm;

		for (const auto& index : Shape.mesh.indices)
		{
			EntityVertex vertex{ };

			vertex.Position =
			{
				Attrib.vertices[3 * index.vertex_index + 0],
				Attrib.vertices[3 * index.vertex_index + 1],
				Attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.TextureCoords =
			{
				Attrib.texcoords[2 * index.texcoord_index + 0],
				Attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vertex.Color = { 1.0f, 1.0f, 1.0f };

			if (index.normal_index >= 0)
			{
				vertex.Normal =
				{
					Attrib.normals[3 * index.normal_index + 0],
					Attrib.normals[3 * index.normal_index + 1],
					Attrib.normals[3 * index.normal_index + 2]
				};
			}
			else
			{
				assert(false);
				// Fallback if normal is not available (optional)
				vertex.Normal = { 0.0f, 1.0f, 0.0f }; // Default to up-direction, or compute it later
			}

			if (!uniqueVertices.contains(vertex))
			{
				uniqueVertices[vertex] = static_cast<u32>(Tm.vertices.size());
				Tm.vertices.push_back(vertex);
			}

			Tm.indices.push_back(uniqueVertices[vertex]);
		}

		Tm.MaterialIndex = MaterialToTexture[Shape.mesh.material_ids[0]];

		ModelMeshes.push_back(Tm);
	}
	
	{
		auto FloorMesh = CreateCubeMesh(CubeObj, WhiteMaterialIndex);

		glm::vec3 CubePos(0.0f, -5.0f, 0.0f);
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, CubePos);
		//model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(20.0f, 1.0f, 20.0f));
		FloorMesh.Model = model;

		ModelMeshes.emplace_back(FloorMesh);
	}

	{
		auto CenterMesh = CreateCubeMesh(CubeObj, TestMaterialIndex);

		glm::vec3 CubePos(0.0f, 0.0f, 0.0f);
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, CubePos);
		model = glm::scale(model, glm::vec3(0.2f, 5.0f, 0.2f));
		CenterMesh.Model = model;

		ModelMeshes.emplace_back(CenterMesh);
	}
	
	ModelMeshes.emplace_back(CreateCubeMesh(CubeObj, GrassMaterial));
	ModelMeshes.emplace_back(CreateCubeMesh(CubeObj, WhiteMaterialIndex));
	ModelMeshes.emplace_back(CreateCubeMesh(CubeObj, WhiteMaterialIndex));
	ModelMeshes.emplace_back(CreateCubeMesh(CubeObj, ContainerMaterialIndex));

	DrawEntities.resize(ModelMeshes.size());

	for (int i = 0; i < ModelMeshes.size(); ++i)
	{
		DrawEntities[i].VertexOffset = BMR::LoadVertices(ModelMeshes[i].vertices.data(),
			sizeof(EntityVertex), ModelMeshes[i].vertices.size());

		DrawEntities[i].IndexOffset = BMR::LoadIndices(ModelMeshes[i].indices.data(),
			ModelMeshes[i].indices.size());

		DrawEntities[i].IndicesCount = ModelMeshes[i].indices.size();
		DrawEntities[i].Model = ModelMeshes[i].Model;
		DrawEntities[i].TextureSet = ModelMeshes[i].MaterialIndex;
	}

	auto SkyBoxMesh = CreateSkyBoxMesh(SkyBoxObj, SkyBoxMaterial);

	SkyBox.VertexOffset = BMR::LoadVertices(SkyBoxMesh.vertices.data(),
		sizeof(SkyBoxVertex), SkyBoxMesh.vertices.size());
	SkyBox.IndexOffset = BMR::LoadIndices(SkyBoxMesh.indices.data(), SkyBoxMesh.indices.size());
	SkyBox.IndicesCount = SkyBoxMesh.indices.size();
	SkyBox.TextureSet = SkyBoxMesh.MaterialIndex;

	{
		glm::vec3 CubePos(0.0f, 0.0f, 8.0f);
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, CubePos);
		model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.5f));

		DrawEntities[ModelMeshes.size() - 4].Model = model;
	}

	{
		glm::vec3 LightCubePos(0.0f, 0.0f, 10.0f);
		glm::mat4 Lightmodel = glm::mat4(1.0f);
		Lightmodel = glm::translate(Lightmodel, LightCubePos);
		Lightmodel = glm::scale(Lightmodel, glm::vec3(0.2f));

		DrawEntities[ModelMeshes.size() - 3].Model = Lightmodel;
	}

	{
		glm::vec3 CubePos(0.0f, 0.0f, 15.0f);
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, CubePos);
		model = glm::scale(model, glm::vec3(1.0f));

		DrawEntities[ModelMeshes.size() - 2].Model = model;
	}

	{
		glm::vec3 CubePos(5.0f, 0.0f, 10.0f);
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, CubePos);
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(1.0f));

		DrawEntities[ModelMeshes.size() - 1].Model = model;
	}

}

void LoadShaders()
{
	std::vector<const char*> ShaderPaths;
	std::vector<BMR::BMRPipelineHandles> HandleToPath;
	std::vector<BMR::BMRShaderStage> StageToStages;

	ShaderPaths.reserve(BMR::BMRShaderNames::ShaderNamesCount);
	HandleToPath.reserve(BMR::BMRPipelineHandles::PipelineHandlesCount);
	StageToStages.reserve(BMR::BMRShaderStage::ShaderStagesCount);

	ShaderPaths.push_back("./Resources/Shaders/TerrainGenerator_vert.spv");
	HandleToPath.push_back(BMR::BMRPipelineHandles::Terrain);
	StageToStages.push_back(BMR::BMRShaderStage::Vertex);

	ShaderPaths.push_back("./Resources/Shaders/TerrainGenerator_frag.spv");
	HandleToPath.push_back(BMR::BMRPipelineHandles::Terrain);
	StageToStages.push_back(BMR::BMRShaderStage::Fragment);

	ShaderPaths.push_back("./Resources/Shaders/vert.spv");
	HandleToPath.push_back(BMR::BMRPipelineHandles::Entity);
	StageToStages.push_back(BMR::BMRShaderStage::Vertex);

	ShaderPaths.push_back("./Resources/Shaders/frag.spv");
	HandleToPath.push_back(BMR::BMRPipelineHandles::Entity);
	StageToStages.push_back(BMR::BMRShaderStage::Fragment);

	ShaderPaths.push_back("./Resources/Shaders/second_vert.spv");
	HandleToPath.push_back(BMR::BMRPipelineHandles::Deferred);
	StageToStages.push_back(BMR::BMRShaderStage::Vertex);

	ShaderPaths.push_back("./Resources/Shaders/second_frag.spv");
	HandleToPath.push_back(BMR::BMRPipelineHandles::Deferred);
	StageToStages.push_back(BMR::BMRShaderStage::Fragment);

	ShaderPaths.push_back("./Resources/Shaders/SkyBox_vert.spv");
	HandleToPath.push_back(BMR::BMRPipelineHandles::SkyBox);
	StageToStages.push_back(BMR::BMRShaderStage::Vertex);

	ShaderPaths.push_back("./Resources/Shaders/SkyBox_frag.spv");
	HandleToPath.push_back(BMR::BMRPipelineHandles::SkyBox);
	StageToStages.push_back(BMR::BMRShaderStage::Fragment);

	ShaderPaths.push_back("./Resources/Shaders/Depth_vert.spv");
	HandleToPath.push_back(BMR::BMRPipelineHandles::Depth);
	StageToStages.push_back(BMR::BMRShaderStage::Vertex);

	for (u32 i = 0; i < BMR::BMRShaderNames::ShaderNamesCount; ++i)
	{
		Util::OpenAndReadFileFull(ShaderPaths[i], ShaderCodes[i], "rb");
		Config.RenderShaders[i].Code = reinterpret_cast<u32*>(ShaderCodes[i].data());
		Config.RenderShaders[i].CodeSize = ShaderCodes[i].size();
		Config.RenderShaders[i].Handle = HandleToPath[i];
		Config.RenderShaders[i].Stage = StageToStages[i];
	}
}

void UpdateScene(BMR::BMRDrawScene& Scene, f32 DeltaTime)
{
	static f32 Angle = 0.0f;

	Angle += 0.5f * static_cast<f32>(DeltaTime);
	if (Angle > 360.0f)
	{
		Angle -= 360.0f;
	}

	for (int i = 0; i < Scene.DrawEntitiesCount; ++i)
	{
		glm::mat4 TestMat = glm::rotate(Scene.DrawEntities[i].Model, glm::radians(0.5f), glm::vec3(0.0f, 1.0f, 0.0f));
		//Scene.DrawEntities[i].Model = TestMat;
	}
}

int main()
{
	const u32 FrameAllocSize = 1024 * 1024;

	Memory::BmMemoryManagementSystem::Init(FrameAllocSize);

	if (glfwInit() == GL_FALSE)
	{
		Util::Log::Error("glfwInit result is GL_FALSE");
		return -1;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	s32 WindowWidth = 1920;
	s32 WindowHeight = 1080;

	GLFWwindow* Window = glfwCreateWindow(WindowWidth, WindowHeight, "BMEngine", nullptr, nullptr);
	if (Window == nullptr)
	{
		Util::Log::GlfwLogError();
		glfwTerminate();
		return -1;
	}

	
	glfwGetWindowSize(Window, &WindowWidth, &WindowHeight);

	LoadSettings(WindowWidth, WindowHeight);

	glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	std::vector<u32> TerrainIndices;
	GenerateTerrain(TerrainIndices);
	LoadShaders();

	Config.MaxTextures = 90;
	Config.LogHandler = BMRLog;
	Config.EnableValidationLayers = true;

	BMR::Init(glfwGetWin32Window(Window), Config);



	ShadowMapSampler = BMR::CreateSampler(&ShadowMapSamplerCreateInfo);
	DiffuseSampler = BMR::CreateSampler(&DiffuseSamplerCreateInfo);
	SpecularSampler = BMR::CreateSampler(&SpecularSamplerCreateInfo);
	
	VkDescriptorSetLayout VpLayout = BMR::CreateUniformLayout(&VpDescriptorType, &VpStageFlags, 1);
	VkDescriptorSetLayout EntityLightLayout = BMR::CreateUniformLayout(&EntityLightDescriptorType, &EntityLightStageFlags, 1);
	VkDescriptorSetLayout LightSpaceMatrixLayout = BMR::CreateUniformLayout(&LightSpaceMatrixDescriptorType, &LightSpaceMatrixStageFlags, 1);
	VkDescriptorSetLayout MaterialLayout = BMR::CreateUniformLayout(&MaterialDescriptorType, &MaterialStageFlags, 1);
	VkDescriptorSetLayout DeferredInputLayout = BMR::CreateUniformLayout(DeferredInputDescriptorType, DeferredInputFlags, 2);
	VkDescriptorSetLayout ShadowMapArrayLayout = BMR::CreateUniformLayout(&ShadowMapArrayDescriptorType, &ShadowMapArrayFlags, 1);
	EntitySamplerLayout = BMR::CreateUniformLayout(EntitySamplerDescriptorType, EntitySamplerInputFlags, 2);
	VkDescriptorSetLayout TerrainSkyBoxLayout = BMR::CreateUniformLayout(&TerrainSkyBoxSamplerDescriptorType, &TerrainSkyBoxArrayFlags, 1);

	BMR::BMRUniform VpBuffer[3];
	BMR::BMRUniform EntityLightBuffer[3];
	BMR::BMRUniform LightSpaceMatrixBuffer[3];
	BMR::BMRUniform MaterialBuffer;
	BMR::BMRUniform DeferredInputDepthImage[3];
	BMR::BMRUniform DeferredInputColorImage[3];
	BMR::BMRUniform ShadowMapArray[3];

	VkImageView DeferredInputDepthImageInterface[3];
	VkImageView DeferredInputColorImageInterface[3];
	VkImageView ShadowMapArrayImageInterface[3];
	VkImageView ShadowMapElement1ImageInterface[3];
	VkImageView ShadowMapElement2ImageInterface[3];

	VkDescriptorSet VpSet[3];
	VkDescriptorSet EntityLightSet[3];
	VkDescriptorSet LightSpaceMatrixSet[3];
	VkDescriptorSet MaterialSet;
	VkDescriptorSet DeferredInputSet[3];
	VkDescriptorSet ShadowMapArraySet[3];

	for (u32 i = 0; i < BMR::GetImageCount(); i++)
	{
		//const VkDeviceSize AlignedVpSize = VulkanMemoryManagementSystem::CalculateBufferAlignedSize(VpBufferSize);
		const VkDeviceSize VpBufferSize = sizeof(BMR::BMRUboViewProjection);
		const VkDeviceSize LightBufferSize = sizeof(BMR::BMRLightBuffer);
		const VkDeviceSize LightSpaceMatrixSize = sizeof(BMR::BMRLightSpaceMatrix);

		VpBufferInfo.size = VpBufferSize;
		VpBuffer[i] = BMR::CreateUniformBuffer(&VpBufferInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		VpBufferInfo.size = LightBufferSize;
		EntityLightBuffer[i] = BMR::CreateUniformBuffer(&VpBufferInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		VpBufferInfo.size = LightSpaceMatrixSize;
		LightSpaceMatrixBuffer[i] = BMR::CreateUniformBuffer(&VpBufferInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		DeferredInputDepthImage[i] = BMR::CreateUniformImage(&DeferredInputDepthUniformCreateInfo);
		DeferredInputColorImage[i] = BMR::CreateUniformImage(&DeferredInputColorUniformCreateInfo);
		ShadowMapArray[i] = BMR::CreateUniformImage(&ShadowMapArrayCreateInfo);

		DeferredInputDepthImageInterface[i] = BMR::CreateImageInterface(&DeferredInputDepthUniformInterfaceCreateInfo,
			DeferredInputDepthImage[i].Image);
		DeferredInputColorImageInterface[i] = BMR::CreateImageInterface(&DeferredInputUniformColorInterfaceCreateInfo,
			DeferredInputColorImage[i].Image);
		ShadowMapArrayImageInterface[i] = BMR::CreateImageInterface(&ShadowMapArrayInterfaceCreateInfo,
			ShadowMapArray[i].Image);
		ShadowMapElement1ImageInterface[i] = BMR::CreateImageInterface(&ShadowMapElement1InterfaceCreateInfo,
			ShadowMapArray[i].Image);
		ShadowMapElement2ImageInterface[i] = BMR::CreateImageInterface(&ShadowMapElement2InterfaceCreateInfo,
			ShadowMapArray[i].Image);
	
		BMR::CreateUniformSets(&VpLayout, 1, VpSet + i);
		BMR::CreateUniformSets(&EntityLightLayout, 1, EntityLightSet + i);
		BMR::CreateUniformSets(&LightSpaceMatrixLayout, 1, LightSpaceMatrixSet + i);
		BMR::CreateUniformSets(&DeferredInputLayout, 1, DeferredInputSet + i);
		BMR::CreateUniformSets(&ShadowMapArrayLayout, 1, ShadowMapArraySet + i);

		BMR::BMRUniformSetAttachmentInfo VpBufferAttachmentInfo;
		VpBufferAttachmentInfo.BufferInfo.buffer = VpBuffer[i].Buffer;
		VpBufferAttachmentInfo.BufferInfo.offset = 0;
		VpBufferAttachmentInfo.BufferInfo.range = VpBufferSize;
		VpBufferAttachmentInfo.Type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

		BMR::BMRUniformSetAttachmentInfo EntityLightAttachmentInfo;
		EntityLightAttachmentInfo.BufferInfo.buffer = EntityLightBuffer[i].Buffer;
		EntityLightAttachmentInfo.BufferInfo.offset = 0;
		EntityLightAttachmentInfo.BufferInfo.range = LightBufferSize;
		EntityLightAttachmentInfo.Type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

		BMR::BMRUniformSetAttachmentInfo LightSpaceMatrixAttachmentInfo;
		LightSpaceMatrixAttachmentInfo.BufferInfo.buffer = LightSpaceMatrixBuffer[i].Buffer;
		LightSpaceMatrixAttachmentInfo.BufferInfo.offset = 0;
		LightSpaceMatrixAttachmentInfo.BufferInfo.range = LightSpaceMatrixSize;
		LightSpaceMatrixAttachmentInfo.Type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

		BMR::BMRUniformSetAttachmentInfo DeferredInputAttachmentInfo[2];
		DeferredInputAttachmentInfo[0].ImageInfo.imageView = DeferredInputColorImageInterface[i];
		DeferredInputAttachmentInfo[0].ImageInfo.sampler = nullptr;
		DeferredInputAttachmentInfo[0].ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		DeferredInputAttachmentInfo[0].Type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;

		DeferredInputAttachmentInfo[1].ImageInfo.imageView = DeferredInputDepthImageInterface[i];
		DeferredInputAttachmentInfo[1].ImageInfo.sampler = nullptr;
		DeferredInputAttachmentInfo[1].ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		DeferredInputAttachmentInfo[1].Type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;

		BMR::BMRUniformSetAttachmentInfo ShadowMapArrayAttachmentInfo;
		ShadowMapArrayAttachmentInfo.ImageInfo.imageView = ShadowMapArrayImageInterface[i];
		ShadowMapArrayAttachmentInfo.ImageInfo.sampler = ShadowMapSampler;
		ShadowMapArrayAttachmentInfo.ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		ShadowMapArrayAttachmentInfo.Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

		BMR::AttachUniformsToSet(VpSet[i], &VpBufferAttachmentInfo, 1);
		BMR::AttachUniformsToSet(EntityLightSet[i], &EntityLightAttachmentInfo, 1);
		BMR::AttachUniformsToSet(LightSpaceMatrixSet[i], &LightSpaceMatrixAttachmentInfo, 1);
		BMR::AttachUniformsToSet(DeferredInputSet[i], DeferredInputAttachmentInfo, 2);
		BMR::AttachUniformsToSet(ShadowMapArraySet[i], &ShadowMapArrayAttachmentInfo, 1);
	}

	const VkDeviceSize MaterialSize = sizeof(BMR::BMRMaterial);
	VpBufferInfo.size = MaterialSize;
	MaterialBuffer = BMR::CreateUniformBuffer(&VpBufferInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	BMR::CreateUniformSets(&MaterialLayout, 1, &MaterialSet);

	BMR::BMRUniformSetAttachmentInfo MaterialAttachmentInfo;
	MaterialAttachmentInfo.BufferInfo.buffer = MaterialBuffer.Buffer;
	MaterialAttachmentInfo.BufferInfo.offset = 0;
	MaterialAttachmentInfo.BufferInfo.range = MaterialSize;
	MaterialAttachmentInfo.Type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

	BMR::AttachUniformsToSet(MaterialSet, &MaterialAttachmentInfo, 1);

	BMR::CreateUniformSets(&EntitySamplerLayout, 1, &TestMaterialIndex);
	BMR::CreateUniformSets(&EntitySamplerLayout, 1, &WhiteMaterialIndex);
	BMR::CreateUniformSets(&EntitySamplerLayout, 1, &ContainerMaterialIndex);
	BMR::CreateUniformSets(&EntitySamplerLayout, 1, &BlendWindowMaterial);
	BMR::CreateUniformSets(&EntitySamplerLayout, 1, &GrassMaterial);
	BMR::CreateUniformSets(&TerrainSkyBoxLayout, 1, &SkyBoxMaterial);
	BMR::CreateUniformSets(&TerrainSkyBoxLayout, 1, &TerrainMaterial);

	std::vector<BMR::BMRRenderPass> RenderPasses(2);
	{
		BMR::BMRRenderTarget RenderTarget;
		for (u32 ImageIndex = 0; ImageIndex < BMR::GetImageCount(); ImageIndex++)
		{
			BMR::BMRAttachmentView* AttachmentView = RenderTarget.AttachmentViews + ImageIndex;

			AttachmentView->ImageViews = Memory::BmMemoryManagementSystem::FrameAlloc<VkImageView>(MainRenderPassSettings.AttachmentDescriptionsCount);
			AttachmentView->ImageViews[0] = BMR::GetSwapchainImageViews()[ImageIndex];
			AttachmentView->ImageViews[1] = DeferredInputColorImageInterface[ImageIndex];
			AttachmentView->ImageViews[2] = DeferredInputDepthImageInterface[ImageIndex];
		
		}

		BMR::CreateRenderPass(&MainRenderPassSettings, &RenderTarget, MainScreenExtent, 1, BMR::GetImageCount(), &RenderPasses[0]);
	}
	{
		BMR::BMRRenderTarget RenderTargets[2];
		for (u32 ImageIndex = 0; ImageIndex < BMR::GetImageCount(); ImageIndex++)
		{
			BMR::BMRAttachmentView* Target1AttachmentView = RenderTargets[0].AttachmentViews + ImageIndex;
			BMR::BMRAttachmentView* Target2AttachmentView = RenderTargets[1].AttachmentViews + ImageIndex;

			Target1AttachmentView->ImageViews = Memory::BmMemoryManagementSystem::FrameAlloc<VkImageView>(DepthRenderPassSettings.AttachmentDescriptionsCount);
			Target2AttachmentView->ImageViews = Memory::BmMemoryManagementSystem::FrameAlloc<VkImageView>(DepthRenderPassSettings.AttachmentDescriptionsCount);

			Target1AttachmentView->ImageViews[0] = ShadowMapElement1ImageInterface[ImageIndex];
			Target2AttachmentView->ImageViews[0] = ShadowMapElement2ImageInterface[ImageIndex];
		}

		BMR::CreateRenderPass(&DepthRenderPassSettings, RenderTargets, DepthViewportExtent, 2, BMR::GetImageCount(), &RenderPasses[1]);
	}

	BMR::TestSetRendeRpasses(RenderPasses[0], RenderPasses[1],
		VpBuffer, VpLayout, VpSet,
		EntityLightBuffer, EntityLightLayout, EntityLightSet,
		LightSpaceMatrixBuffer, LightSpaceMatrixLayout, LightSpaceMatrixSet,
		MaterialBuffer, MaterialLayout, MaterialSet,
		DeferredInputDepthImageInterface, DeferredInputColorImageInterface,
		DeferredInputLayout, DeferredInputSet,
		ShadowMapArray, ShadowMapArrayLayout, ShadowMapArraySet,
		EntitySamplerLayout, TerrainSkyBoxLayout);











	ShaderCodes.clear();
	ShaderCodes.shrink_to_fit();

	LoadDrawEntities();

	BMR::BMRDrawScene Scene;
	Scene.SkyBox = SkyBox;
	Scene.DrawSkyBox = true;

	const f32 Near = 0.1f;
	const f32 Far = 100.0f;

	const float Aspect = (float)MainScreenExtent.width / (float)MainScreenExtent.height;

	Scene.ViewProjection.Projection = glm::perspective(glm::radians(45.f),
		Aspect, Near, Far);
	Scene.ViewProjection.Projection[1][1] *= -1;
	Scene.ViewProjection.View = glm::lookAt(glm::vec3(0.0f, 0.0f, 20.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	BMR::BMRDrawTerrainEntity TestDrawTerrainEntity;
	TestDrawTerrainEntity.VertexOffset = BMR::LoadVertices(&TerrainVerticesData[0][0],
		sizeof(TerrainVertex), NumRows * NumCols);
	TestDrawTerrainEntity.IndexOffset = BMR::LoadIndices(TerrainIndices.data(), TerrainIndices.size());
	TestDrawTerrainEntity.IndicesCount = TerrainIndices.size();
	TestDrawTerrainEntity.TextureSet = TerrainMaterial;

	TerrainIndices.clear();
	TerrainIndices.shrink_to_fit();

	Scene.DrawTerrainEntities = &TestDrawTerrainEntity;
	Scene.DrawTerrainEntitiesCount = 1;

	Scene.DrawEntities = DrawEntities.data();
	Scene.DrawEntitiesCount = DrawEntities.size();


	f64 DeltaTime = 0.0f;
	f64 LastTime = 0.0f;

	Camera MainCamera;

	Memory::BmMemoryManagementSystem::FrameDealloc();

	BMR::BMRLightBuffer TestData;
	TestData.PointLight.Position = glm::vec4(0.0f, 0.0f, 10.0f, 1.0f);
	TestData.PointLight.Ambient = glm::vec3(0.01f, 0.01f, 0.01f);
	TestData.PointLight.Diffuse = glm::vec3(0.5f, 0.5f, 0.5f);
	TestData.PointLight.Specular = glm::vec3(1.0f, 1.0f, 1.0f);
	TestData.PointLight.Constant = 1.0f;
	TestData.PointLight.Linear = 0.09;
	TestData.PointLight.Quadratic = 0.032;

	TestData.DirectionLight.Direction = glm::vec3(0.0f, -1.0f, 0.0f);
	TestData.DirectionLight.Ambient = glm::vec3(0.01f, 0.01f, 0.01f);
	TestData.DirectionLight.Diffuse = glm::vec3(0.5f, 0.5f, 0.5f);
	TestData.DirectionLight.Specular = glm::vec3(1.0f, 1.0f, 1.0f);

	TestData.SpotLight.Position = glm::vec4(0.0f, 0.0f, 10.0f, 1.0f);
	TestData.SpotLight.Ambient = glm::vec3(0.01f, 0.01f, 0.01f);
	TestData.SpotLight.Diffuse = glm::vec3(0.5f, 0.5f, 0.5f);
	TestData.SpotLight.Specular = glm::vec3(1.0f, 1.0f, 1.0f);
	TestData.SpotLight.Constant = 1.0f;
	TestData.SpotLight.Linear = 0.09;
	TestData.SpotLight.Quadratic = 0.032;
	TestData.SpotLight.CutOff = glm::cos(glm::radians(12.5f));
	TestData.SpotLight.OuterCutOff = glm::cos(glm::radians(17.5f));

	BMR::BMRMaterial Mat;
	Mat.Shininess = 32.f;
	BMR::UpdateMaterialBuffer(&Mat);

	float near_plane = 0.1f, far_plane = 100.0f;
	float halfSize = 30.0f;
	glm::mat4 lightProjection = glm::ortho(-halfSize, halfSize, -halfSize, halfSize, near_plane, far_plane);

	glm::vec3 eye = glm::vec3(0.0f, 10.0f, 0.0f);
	glm::vec3 up = glm::vec3(0.0f, 0.0f, -1.0f);

	ImguiIntegration::GuiData GuiData;
	GuiData.DirectionLightDirection = &TestData.DirectionLight.Direction;
	GuiData.eye = &eye;

	bool DrawImgui = true;
	std::thread ImguiThread([&]()
	{
		// UB 2 threads to variables
		ImguiIntegration::DrawLoop(DrawImgui, GuiData);
	});

	while (!glfwWindowShouldClose(Window) && !Close)
	{
		glfwPollEvents();

		const f64 CurrentTime = glfwGetTime();
		DeltaTime = CurrentTime - LastTime;
		LastTime = CurrentTime;

		UpdateScene(Scene, DeltaTime);

		MoveCamera(Window, DeltaTime, MainCamera);

		Scene.ViewProjection.View = glm::lookAt(MainCamera.CameraPosition, MainCamera.CameraPosition + MainCamera.CameraFront, MainCamera.CameraUp);

		glm::vec3 center = eye + TestData.DirectionLight.Direction;
		glm::mat4 lightView = glm::lookAt(eye, center, up);

		TestData.DirectionLight.LightSpaceMatrix = lightProjection * lightView;
		TestData.SpotLight.Direction = MainCamera.CameraFront;
		TestData.SpotLight.Position = MainCamera.CameraPosition;
		TestData.SpotLight.Planes = glm::vec2(Near, Far);
		TestData.SpotLight.LightSpaceMatrix = Scene.ViewProjection.Projection * Scene.ViewProjection.View;

		Scene.LightEntity = &TestData;
		Draw(Scene);

		Memory::BmMemoryManagementSystem::FrameDealloc();
	}

	DrawImgui = false;
	ImguiThread.join();





	for (u32 i = 0; i < BMR::GetImageCount(); i++)
	{
		BMR::DestroyUniformBuffer(VpBuffer[i]);
		BMR::DestroyUniformBuffer(EntityLightBuffer[i]);
		BMR::DestroyUniformBuffer(LightSpaceMatrixBuffer[i]);
		BMR::DestroyUniformImage(DeferredInputColorImage[i]);
		BMR::DestroyUniformImage(DeferredInputDepthImage[i]);
		BMR::DestroyUniformImage(ShadowMapArray[i]);

		BMR::DestroyImageInterface(DeferredInputColorImageInterface[i]);
		BMR::DestroyImageInterface(DeferredInputDepthImageInterface[i]);
		BMR::DestroyImageInterface(ShadowMapArrayImageInterface[i]);
		BMR::DestroyImageInterface(ShadowMapElement1ImageInterface[i]);
		BMR::DestroyImageInterface(ShadowMapElement2ImageInterface[i]);
	}

	BMR::DestroyUniformBuffer(MaterialBuffer);

	BMR::DestroyUniformLayout(VpLayout);
	BMR::DestroyUniformLayout(EntityLightLayout);
	BMR::DestroyUniformLayout(LightSpaceMatrixLayout);
	BMR::DestroyUniformLayout(MaterialLayout);
	BMR::DestroyUniformLayout(DeferredInputLayout);
	BMR::DestroyUniformLayout(ShadowMapArrayLayout);
	BMR::DestroyUniformLayout(EntitySamplerLayout);
	BMR::DestroyUniformLayout(TerrainSkyBoxLayout);

	BMR::DestroySampler(ShadowMapSampler);
	BMR::DestroySampler(DiffuseSampler);

	for (auto& Pass : RenderPasses)
	{
		BMR::DestroyRenderPass(&Pass);
	}



	
	BMR::DeInit();

	glfwDestroyWindow(Window);

	glfwTerminate();

	Memory::BmMemoryManagementSystem::DeInit();

	if (Memory::BmMemoryManagementSystem::AllocateCounter != 0)
	{
		Util::Log::Error("AllocateCounter in not equal 0, counter is {}", Memory::BmMemoryManagementSystem::AllocateCounter);
		assert(false);
	}

	return 0;
}