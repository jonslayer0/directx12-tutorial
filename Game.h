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
	GAME(string name, int width, int height, bool vSync);
	virtual ~GAME();

	inline int GetClientWidth() const { return _width; }
	inline int GetClientHeight() const { return _height; }

	bool Intitialize();
	bool LoadContent();
	void UnloadContent();
	void Destroy();

private :
	void OnUpdate();
	void OnRender();
	void OnKeyPressed();
	void OnKeyReleased();
	void OnMouseMoved();
	void OnMouseButtonPressed();
	void OnMouseWheel();
	void OnResize();
	void OnWindowDestroy();

	WINDOW* _window = nullptr;
	string _name;
	int _width = 1;
	int _height = 1;
	bool _vSync = true;
};