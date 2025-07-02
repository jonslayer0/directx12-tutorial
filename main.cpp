#define WIN32_LEAN_AND_MEAN

#include "Application.h"
#include "Window.h"

int CALLBACK main(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
    APPLICATION* application = APPLICATION::CreateInstance(hInstance);

    application->Run();
    application->Quit();

    return 0;
}