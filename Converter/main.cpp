#include "Util/EngineTypes.h"
#include "Util/Util.h"

#include <windows.h>
#include <shlwapi.h>
#include <stdio.h>


void CreateDirectoryRecursively(const char* Path)
{
	char FolderName[MAX_PATH];

	for (u32 i = 0; Path[i]; ++i)
	{
		FolderName[i] = Path[i];

		if (Path[i] == '\\' || Path[i] == '/')
		{
			FolderName[i] = '\0';

			if (GetFileAttributes(FolderName) == INVALID_FILE_ATTRIBUTES)
			{
				CreateDirectory(FolderName, NULL);
			}

			FolderName[i] = '\\';
		}
	}
}

int main(u32 argc, const char* argv[])
{
	if (argc < 2)
	{
		argc = 3;
		argv[0] = "-m";
		argv[1] = "D:\\Code\\BMEngine\\BMEngine\\Resources\\Models/";
		argv[2] = "D:\\Code\\BMEngine\\BMEngine\\Resources\\Models\\cube.obj";
	}

	for (u32 i = 0; i < argc; i++)
	{
		const char* Command = argv[i];
		if (strcmp(Command, "-m") == 0)
		{
			const char* OutputPath = argv[1];
			CreateDirectoryRecursively(OutputPath);

			printf("Output path: %s\n", OutputPath);

			for (u32 j = 2; j < argc; j++)
			{
				const char* File = argv[j];
				printf("  %s\n", File);

				if (GetFileAttributesA(File) != INVALID_FILE_ATTRIBUTES)
				{
					Util::ObjToModel3D(File, OutputPath);
				}
				else
				{
					printf("  -> File does not exist: %s\n", argv[j]);
				}
			}
			break;
		}
	}

	return 0;
}