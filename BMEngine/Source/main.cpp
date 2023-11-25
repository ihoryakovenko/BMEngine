//#define GLM_FORCE_RADIANS
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#include <glm/glm.hpp>
//#include <glm/mat4x4.hpp>

#define STB_IMAGE_IMPLEMENTATION

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Util/Util.h"

#include <cassert>
#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>

#include "Core/VulkanCoreTypes.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

struct TestMesh
{
	std::vector<Core::Vertex> vertices;
	std::vector<uint32_t> indices;
	int TextureId;
};

TestMesh LoadMesh(aiMesh* mesh, const aiScene* scene, std::vector<int> matToTex)
{
	TestMesh Mesh;
	

	Mesh.vertices.resize(mesh->mNumVertices);

	// Go through each vertex and copy it across to our vertices
	for (size_t i = 0; i < mesh->mNumVertices; i++)
	{
		Mesh.vertices[i].Position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };

		if (mesh->mTextureCoords[0])
		{
			Mesh.vertices[i].TextureCoords = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
		}
		else
		{
			Mesh.vertices[i].TextureCoords = { 0.0f, 0.0f };
		}

		Mesh.vertices[i].Color = { 1.0f, 1.0f, 1.0f };
	}

	for (size_t i = 0; i < mesh->mNumFaces; i++)
	{
		// Get a face
		aiFace face = mesh->mFaces[i];

		for (size_t j = 0; j < face.mNumIndices; j++)
		{
			Mesh.indices.push_back(face.mIndices[j]);
		}
	}

	Mesh.TextureId = matToTex[mesh->mMaterialIndex];
	return Mesh;
}

std::vector<TestMesh> LoadNode(aiNode* node, const aiScene* scene, std::vector<int> matToTex)
{
	std::vector<TestMesh> meshList;

	// Go through each mesh at this node and create it, then add it to our meshList
	for (size_t i = 0; i < node->mNumMeshes; i++)
	{
		meshList.push_back(LoadMesh(scene->mMeshes[node->mMeshes[i]], scene, matToTex)
		);
	}

	// Go through each node attached to this node and load it, then append their meshes to this node's mesh list
	for (size_t i = 0; i < node->mNumChildren; i++)
	{
		std::vector<TestMesh> newList = LoadNode(node->mChildren[i], scene, matToTex);
		meshList.insert(meshList.end(), newList.begin(), newList.end());
	}

	return meshList;
}

int main()
{
	if (glfwInit() == GL_FALSE)
	{
		Util::Log::Error("glfwInit result is GL_FALSE");
		return -1;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	GLFWwindow* Window = glfwCreateWindow(1600, 800, "BMEngine", nullptr, nullptr);
	if (Window == nullptr)
	{
		Util::Log::GlfwLogError();
		glfwTerminate();
		return -1;
	}

	Core::MainInstance Instance;
	Core::InitMainInstance(Instance, Util::EnableValidationLayers);

	Core::VulkanRenderInstance RenderInstance;
	Core::InitVulkanRenderInstance(RenderInstance, Instance.VulkanInstance, Window);

	RenderInstance.ViewProjection.Projection = glm::perspective(glm::radians(45.f),
		static_cast<float>(RenderInstance.SwapExtent.width) / static_cast<float>(RenderInstance.SwapExtent.height), 0.1f, 100.0f);

	RenderInstance.ViewProjection.Projection[1][1] *= -1;

	RenderInstance.ViewProjection.View = glm::lookAt(glm::vec3(0.0f, 0.0f, 20.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	// Function LoadTexture
	int Width, Height;
	uint64_t ImageSize; // Todo: DeviceSize?
	int Channels;

	const char* TestTexture = "./Resources/Textures/giraffe.jpg";
	stbi_uc* ImageData = stbi_load(TestTexture, &Width, &Height, &Channels, STBI_rgb_alpha);

	if (ImageData == nullptr)
	{
		return -1;
	}

	ImageSize = Width * Height * 4;
	// Function end LoadTexture

	Core::CreateTexture(RenderInstance, ImageData, Width, Height, ImageSize);


	// TMP shit
	Assimp::Importer AssimpImporter;
	const char* Modelpath = "./Resources/Models/uh60.obj";
	const aiScene* Scene = AssimpImporter.ReadFile(Modelpath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);
	if (!Scene)
	{
		return -1;
	}

	std::vector<std::string> TextureList(Scene->mNumMaterials);
	std::vector<int> MaterialToTexture(TextureList.size());

	for (size_t i = 0; i < Scene->mNumMaterials; i++)
	{
		aiMaterial* Material = Scene->mMaterials[i];

		TextureList[i] = "";

		if (Material->GetTextureCount(aiTextureType_DIFFUSE))
		{
			aiString Path;
			if (Material->GetTexture(aiTextureType_DIFFUSE, 0, &Path) == AI_SUCCESS)
			{
				int idx = std::string(Path.data).rfind("\\");
				std::string fileName = "./Resources/Textures/" + std::string(Path.data).substr(idx + 1);

				TextureList[i] = fileName;
			}
		}
	}

	for (size_t i = 0; i < TextureList.size(); i++)
	{
		if (TextureList[i].empty())
		{
			MaterialToTexture[i] = 0;
		}
		else
		{
			MaterialToTexture[i] = RenderInstance.TextureImagesCount;

			stbi_uc* ImageData = stbi_load(TextureList[i].c_str(), &Width, &Height, &Channels, STBI_rgb_alpha);

			if (ImageData == nullptr)
			{
				return -1;
			}

			ImageSize = Width * Height * 4;

			Core::CreateTexture(RenderInstance, ImageData, Width, Height, ImageSize);

			stbi_image_free(ImageData);
		}
	}

	std::vector<TestMesh> ModelMeshes = LoadNode(Scene->mRootNode, Scene, MaterialToTexture);

	for (int i = 0; i < ModelMeshes.size(); ++i)
	{
		Core::Mesh m;
		m.MeshVertices = ModelMeshes[i].vertices.data();
		m.MeshVerticesCount = ModelMeshes[i].vertices.size();
		m.MeshIndices = ModelMeshes[i].indices.data();
		m.MeshIndicesCount = ModelMeshes[i].indices.size();
		Core::LoadMesh(RenderInstance, m);
		RenderInstance.DrawableObjects[RenderInstance.DrawableObjectsCount - 1].TextureId = ModelMeshes[i].TextureId;
	}

	float Angle = 0.0f;
	double DeltaTime = 0.0f;
	double LastTime = 0.0f;

	float XMove = 0.0f;
	float MoveScale = 4.0f;

	while (!glfwWindowShouldClose(RenderInstance.Window))
	{
		glfwPollEvents();

		const double CurrentTime = glfwGetTime();
		DeltaTime = CurrentTime - LastTime;
		LastTime = static_cast<float>(CurrentTime);

		Angle += 30.0f * static_cast<float>(DeltaTime);
		if (Angle > 360.0f)
		{
			Angle -= 360.0f;
		}

		glm::mat4 TestMat = glm::rotate(glm::mat4(1.0f), glm::radians(Angle), glm::vec3(0.0f, 1.0f, 0.0f));

		for (int i = 0; i < RenderInstance.DrawableObjectsCount; ++i)
		{
			RenderInstance.DrawableObjects[i].Model = TestMat;
		}

		Core::Draw(RenderInstance);
	}

	Core::DeinitVulkanRenderInstance(RenderInstance);
	Core::DeinitMainInstance(Instance);

	glfwDestroyWindow(Window);

	glfwTerminate();

	if (Util::Memory::AllocateCounter != 0)
	{
		Util::Log::Error("AllocateCounter in not equal 0, counter is {}", Util::Memory::AllocateCounter);
		assert(false);
	}

	return 0;
}