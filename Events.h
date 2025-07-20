#pragma once

struct UpdateEventArgs
{
	double elapseTime = 0.0;
	double totalTime = 0.0;
};

struct RenderEventArgs
{
};

struct KeyEventArgs
{
};

struct MouseMotionEventArgs
{
};

struct MouseButtonEventArgs
{
};

struct MouseWheelEventArgs
{
};

struct ResizeEventArgs
{
	int width = 1;
	int height = 1;
};