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

#include "OpenFileDialog.h"

#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <shobjidl.h> 
#endif


bool VolumeRaytracer::UI::VOpenFileDialog::Open(const std::wstring& filter, std::wstring& outPath)
{
#ifdef _WIN32
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED |
		COINIT_DISABLE_OLE1DDE);
	if (SUCCEEDED(hr))
	{
		IFileOpenDialog* pFileOpen;

		// Create the FileOpenDialog object.
		hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
			IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

		bool bSuccess = false;

		if (SUCCEEDED(hr))
		{
			std::vector<std::wstring> filterNames; 
			std::vector<std::wstring> filters;

			SplitFilter(filter, filterNames, filters);

			COMDLG_FILTERSPEC* filterSpecs = nullptr;

			if (filters.size() == filterNames.size() && filters.size() > 0)
			{
				filterSpecs = new COMDLG_FILTERSPEC[filters.size()];

				for (int i = 0; i < filters.size(); i++)
				{
					filterSpecs->pszName = filterNames[i].c_str();
					filterSpecs->pszSpec = filters[i].c_str();
				}

				pFileOpen->SetFileTypes(filters.size(), filterSpecs);
			}

			// Show the Open dialog box.
			hr = pFileOpen->Show(NULL);

			// Get the file name from the dialog box.
			if (SUCCEEDED(hr))
			{
				IShellItem* pItem;
				hr = pFileOpen->GetResult(&pItem);
				if (SUCCEEDED(hr))
				{
					LPWSTR pszFilePath;
					hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

					outPath = std::wstring(pszFilePath);
					bSuccess = true;

					pItem->Release();
				}
			}

			if (filterSpecs != nullptr)
			{
				delete[] filterSpecs;
				filterSpecs = nullptr;
			}

			pFileOpen->Release();
		}
		CoUninitialize();

		return bSuccess;
	}

	return false;
#endif
}

void VolumeRaytracer::UI::VOpenFileDialog::SplitFilter(const std::wstring& filter, std::vector<std::wstring>& outfilterNames, std::vector<std::wstring>& outFilter)
{
	std::vector<std::wstring> stringParts;

	std::wstringstream wss(filter);
	std::wstring temp;

	while(std::getline(wss, temp, L';'))
		stringParts.push_back(temp);

	for (int i = 0; i < stringParts.size(); i++)
	{
		if (i % 2 == 0)
		{
			outfilterNames.push_back(stringParts[i]);
		}
		else
		{
			outFilter.push_back(stringParts[i]);
		}
	}
}
