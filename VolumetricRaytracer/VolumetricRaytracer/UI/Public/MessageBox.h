/*
	Copyright (c) 2021 Thomas Schöngrundner

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.
*/

#include <string>

namespace VolumeRaytracer
{
	namespace UI
	{
		enum EVMessageBoxType
		{
			Info = 0,
			Warning = 1,
			Error = 2,
		};

		class VMessageBox
		{
		public:
			static void ShowOk(const std::wstring& title, const std::wstring& message, const EVMessageBoxType& type = EVMessageBoxType::Info);
		};
	}
}