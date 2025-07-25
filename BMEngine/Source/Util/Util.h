#pragma once

#include <string>
#include <iostream>
#include <format>
#include <source_location>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Deprecated/VulkanInterface/VulkanInterface.h"
#include "Engine/Systems/Render/VulkanHelper.h"
#include "Engine/Systems/Render/RenderResources.h"

namespace Util
{
	bool ReadFileFull(FILE* File, std::vector<char>& OutFileData);
	bool OpenAndReadFileFull(const char* FileName, std::vector<char>& OutFileData, const char* Mode);

	enum LogType
	{
		BMRVkLogType_Error,
		BMRVkLogType_Warning,
		BMRVkLogType_Info
	};

	void RenderLog(LogType logType, const char* format, ...);
	void RenderLog(LogType LogType, const char* Format, va_list Args);

	struct Log
	{
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

		template <typename... TArgs>
		static void Info(std::string_view Message, TArgs&&... Args)
		{
			Print("Info", Message, Args...);
		}

		static void GlfwLogError()
		{
			const char* ErrorDescription;
			const int LastErrorCode = glfwGetError(&ErrorDescription);

			const std::string_view ErrorLog = LastErrorCode != GLFW_NO_ERROR ? ErrorDescription : "No GLFW error";
			Error(ErrorLog);
		}

		template <typename... TArgs>
		static void Print(std::string_view Type, std::string_view Message, TArgs&&... Args)
		{
			//std::cout << std::format("{}: {}\n", Type, std::vformat(Message, std::make_format_args(Args...)));
		}
	};

	void LoadPipelineSettings(VulkanHelper::PipelineSettings& settings, const char* filePath);

	
	Yaml::Node& GetSamplers(Yaml::Node& Root);
	void ParseSamplerNode(Yaml::Node& SamplerNode, VkSamplerCreateInfo* OutSamplerCreateInfo);
	
	Yaml::Node& GetShaders(Yaml::Node& Root);
	void ParseShaderNode(Yaml::Node& ShaderNode, std::string* OutShaderPath);
	
	Yaml::Node& GetDescriptorSetLayouts(Yaml::Node& Root);
	Yaml::Node& ParseDescriptorSetLayoutNode(Yaml::Node& Root);
	void ParseDescriptorSetLayoutBindingNode(Yaml::Node& BindingNode, VkDescriptorSetLayoutBinding* OutBinding);
	
	Yaml::Node& GetVertices(Yaml::Node& Root);
	Yaml::Node& GetVertexBindingNode(Yaml::Node& VertexNode);
	Yaml::Node& GetVertexAttributesNode(Yaml::Node& VertexNode);
	Yaml::Node& GetVertexAttributeFormatNode(Yaml::Node& AttributeNode);
	void ParseVertexAttributeNode(Yaml::Node& AttributeNode, VulkanHelper::VertexAttribute* OutAttribute, std::string* OutAttributeName);
	void ParseVertexBindingNode(Yaml::Node& BindingNode, VulkanHelper::VertexBinding* OutBinding);

#ifdef NDEBUG
	static bool EnableValidationLayers = false;
#else
	static bool EnableValidationLayers = true;
#endif

	typedef u8* Model3DData;

	struct Model3DFileHeader
	{
		u64 VertexDataSize;
		u32 MeshCount;
	};

	struct Model3D
	{
		u8* VertexData;
		u64* VerticesCounts;
		u32* IndicesCounts;
		u64* DiffuseTexturesHashes;
		u32 MeshCount;
	};

	void ObjToModel3D(const char* FilePath, const char* OutputPath);

	Model3DData LoadModel3DData(const char* FilePath);
	void ClearModel3DData(Model3DData Data);

	Model3D ParseModel3D(Model3DData Data);

}