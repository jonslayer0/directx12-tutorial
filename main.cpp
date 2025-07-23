#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Shlwapi.h>

#include "Application.h"
#include "Tutorial/Tutorial.h"

int CALLBACK main(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
    int retCode = 0;

    // Set the working directory to the path of the executable.
    WCHAR path[MAX_PATH];
    HMODULE hModule = GetModuleHandleW(NULL);
    if (GetModuleFileNameW(hModule, path, MAX_PATH) > 0)
    {
        PathRemoveFileSpecW(path);
        SetCurrentDirectoryW(path);
    }

    APPLICATION* application = APPLICATION::CreateInstance(hInstance);
    {
        std::shared_ptr<TUTORIAL> demo = std::make_shared<TUTORIAL>(L"Learning DirectX 12 - Lesson 2", 1280, 720, false);
        retCode = application->Run(demo);
    }
    application->Quit();
    /*application->ParseCommandLineArguments();
    application->CreateRenderWindow(L"DX12WindowClass", 1280, 720, false);
    application->Run();
    application->Quit();*/

    return 0;
}