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

#include "FileStreamReader.h"
#include <boost/filesystem.hpp>

std::shared_ptr<std::istream> VolumeRaytracer::Voxelizer::VFileStreamReader::GetInputStream(const std::string& filename) const
{
	std::string fullPath = boost::filesystem::absolute(filename, BasePath).string();
	std::shared_ptr<std::ifstream> stream = std::make_shared<std::ifstream>(fullPath, std::ios_base::binary);

	return stream;
}
