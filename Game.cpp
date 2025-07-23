#include "Game.h"
#include "Application.h"
#include "Window.h"

GAME::GAME(const wstring& name, int width, int height, bool vSync):
	_name(name),
	_width(width),
	_height(height),
	_vSync(vSync)
{

}

GAME::~GAME() 
{
	Destroy();
}

bool GAME::Initialize()
{
	if (DirectX::XMVerifyCPUSupport() == false)
	{
		MessageBoxA(nullptr, "Failed to verify DirectX Math library support.", "Error", MB_OK | MB_ICONERROR);
		return false;
	}

	_window = APPLICATION::Instance()->CreateRenderWindow(L"DX12WindowClass", 1280, 720, false); 
	_window->RegisterCallbacks(shared_from_this());
	_window->Show();

	return true;
}

void GAME::Destroy()
{
	//APPLICATION::Instance()->DestroyWindow();
	_window = nullptr;
}