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
}
