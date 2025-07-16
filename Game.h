#pragma once

#include "Helpers.h"

#include <Events.h>

#include <memory.h>
#include <string.h>
using namespace std;

class WINDOW;

class GAME
{
public:
	GAME(const wstring& name, int width, int height, bool vSync);
	virtual ~GAME();

	inline int GetClientWidth() const { return _width; }
	inline int GetClientHeight() const { return _height; }

	virtual bool Initialize();
	virtual bool LoadContent() = 0;
	virtual void UnloadContent() = 0;
	virtual void Destroy();

private :
	virtual void OnUpdate(UpdateEventArgs& e) = 0;;
	virtual void OnRender(RenderEventArgs& e) = 0;;
	virtual void OnKeyPressed(KeyEventArgs& e) = 0;;
	virtual void OnKeyReleased(KeyEventArgs& e) = 0;;
	virtual void OnMouseMoved(MouseMotionEventArgs& e) = 0;;
	virtual void OnMouseButtonPressed(MouseButtonEventArgs& e) = 0;;
	virtual void OnMouseWheel(MouseWheelEventArgs& e) = 0;;
	virtual void OnResize(ResizeEventArgs& e) = 0;;
	virtual void OnWindowDestroy() = 0;;

	WINDOW* _window = nullptr;
	wstring _name;
	int _width = 1;
	int _height = 1;
	bool _vSync = true;
};