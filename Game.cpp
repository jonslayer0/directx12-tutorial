#include "Game.h"
#include "Application.h"

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

	_window = APPLICATION::Instance()->GetWindow();

	return true;
}

void GAME::Destroy()
{

}