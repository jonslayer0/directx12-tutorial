#define WIN32_LEAN_AND_MEAN

#include "Application.h"
#include "Window.h"

int CALLBACK main(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
    APPLICATION* application = APPLICATION::CreateInstance();
    application->ParseCommandLineArguments();
    application->CreateRenderWindow(L"DX12WindowClass", 1280, 720, false);
    application->Run();
    application->Quit();

    return 0;
}