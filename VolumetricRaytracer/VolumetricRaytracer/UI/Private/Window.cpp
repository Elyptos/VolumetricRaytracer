/*
	Copyright (c) 2020 Thomas Schöngrundner

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.
*/

#include "Window.h"

VolumeRaytracer::UI::VWindow::VWindow()
{
}

VolumeRaytracer::UI::VWindow::~VWindow()
{
}

void VolumeRaytracer::UI::VWindow::Tick(const float& deltaSeconds)
{
	
}

void VolumeRaytracer::UI::VWindow::Initialize()
{
	InitializeWindow();
}

void VolumeRaytracer::UI::VWindow::BeginDestroy()
{
	CloseWindow();
}

const bool VolumeRaytracer::UI::VWindow::CanEverTick() const
{
	return true;
}

bool VolumeRaytracer::UI::VWindow::ShouldTick() const
{
	return true;
}

