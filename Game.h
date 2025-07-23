#pragma once

#include "Helpers.h"

#include <Events.h>

#include <memory.h>
#include <string.h>
using namespace std;

class WINDOW;

class GAME : public std::enable_shared_from_this<GAME>
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

protected :
	friend class WINDOW;

	virtual void OnUpdate(UpdateEventArgs& e) { ; }
	virtual void OnRender(RenderEventArgs& e) { ; }
	virtual void OnKeyPressed(KeyEventArgs& e) { ; }
	virtual void OnKeyReleased(KeyEventArgs& e) { ; }
	virtual void OnMouseMoved(MouseMotionEventArgs& e) { ; }
	virtual void OnMouseButtonPressed(MouseButtonEventArgs& e) { ; }
	virtual void OnMouseButtonReleased(MouseButtonEventArgs& e) { ; }
	virtual void OnMouseWheel(MouseWheelEventArgs& e) { ; }
	virtual void OnResize(ResizeEventArgs& e) { ; }
	virtual void OnWindowDestroy() { ; }

	WINDOW* _window = nullptr;
	wstring _name;
	int _width = 1;
	int _height = 1;
	bool _vSync = true;
};