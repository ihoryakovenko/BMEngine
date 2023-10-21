#include "File.h"
#include "Log.h"

namespace Util
{
	std::vector<char> File::ReadFileFull(std::string_view FileName)
	{
		FILE* File = nullptr;
		const int Result = fopen_s(&File, FileName.data(), "rb");
		if (Result == 0)
		{
			_fseeki64(File, 0LL, static_cast<int>(SEEK_END));
			const long long FileSize = _ftelli64(File);
			if (FileSize != -1LL)
			{
				rewind(File); // Put file pointer to 0 index

				std::vector<char> Buffer(static_cast<size_t>(FileSize));
				// Need to check fread result if we know the size?
				const size_t ReadResult = fread(Buffer.data(), 1, static_cast<size_t>(FileSize), File);
				if (ReadResult == static_cast<size_t>(FileSize))
				{
					fclose(File);
					return Buffer;
				}

				Util::Log().Error("Failed to read file {}: ", FileName);
				fclose(File);
				return {};
			}
		}

		Util::Log().Error("Cannot open file {}: Result = {}", FileName, Result);
		return {};
	}
}
