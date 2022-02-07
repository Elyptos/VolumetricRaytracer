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

#include "WindowFactory.h"

#ifdef _WIN32
#include "Win32Window.h"
#endif

#include "Window.h"

VolumeRaytracer::VObjectPtr<VolumeRaytracer::UI::VWindow> VolumeRaytracer::UI::VWindowFactory::NewWindow()
{
#ifdef _WIN32
	VObjectPtr<VWindow> res = VolumeRaytracer::VObjectPtr<VWindow>(VObject::CreateObject<Win32::VWin32Window>());
	return res;
#else
	//Not implemented yet
#endif
}
