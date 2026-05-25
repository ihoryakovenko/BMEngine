#pragma once

#include <ShortTypes.h>
#include <slang.h>
#include <slang-com-ptr.h>
#include <iostream>

inline slang::ISession* InitalizeSlangSession(slang::IGlobalSession* GlobalSession, const char** IncludePaths, u32 IncludePathsCount)
{
	slang::CompilerOptionEntry options[] =
	{
		slang::CompilerOptionName::EmitSpirvDirectly, {slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr},
		slang::CompilerOptionName::MatrixLayoutColumn, {slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr},
	};

	slang::TargetDesc targetDesc = {};
	targetDesc.format = SLANG_SPIRV;
	targetDesc.profile = GlobalSession->findProfile("spirv_1_5");

	slang::SessionDesc sessionDesc = {};
	sessionDesc.targets = &targetDesc;
	sessionDesc.targetCount = 1;
	sessionDesc.searchPaths = IncludePaths;
	sessionDesc.searchPathCount = IncludePathsCount;
	sessionDesc.compilerOptionEntries = options;
	sessionDesc.compilerOptionEntryCount = sizeof(options) / sizeof(options[0]);

	slang::ISession* Session;
	GlobalSession->createSession(sessionDesc, &Session);
	return Session;
}

inline bool CheckDiagnostics(Slang::ComPtr<slang::IBlob> diagnostics)
{
	if (diagnostics)
	{
		std::cout << (const char*)diagnostics->getBufferPointer() << std::endl;
		return false;
	}

	return true;
}
