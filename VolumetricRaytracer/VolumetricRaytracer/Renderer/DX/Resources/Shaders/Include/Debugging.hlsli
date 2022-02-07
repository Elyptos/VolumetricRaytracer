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


inline bool IsPosInsideBox(in float3 pos, in float3 min, in float3 max)
{
	bool3 res = pos >= min && pos <= max;
	
	return res.x && res.y && res.z;
}

bool DoesRayHitOctreeBounds(in float3 pos, in float3 nodePos, in float nodeSize, in float borderThikness)
{
	float3 outerBoxMin = nodePos - borderThikness;
	float3 outerBoxMax = nodePos + nodeSize + borderThikness;
	float3 innerBoxMin = nodePos + borderThikness;
	float3 innerBoxMax = nodePos + nodeSize - borderThikness;
	
	if(IsPosInsideBox(pos, outerBoxMin, outerBoxMax))
	{
		bool3 insideBorder = abs(pos - nodePos) <= borderThikness || abs(pos - (nodePos + nodeSize)) <= borderThikness;
		
		return ((insideBorder.x || insideBorder.y) && insideBorder.z) || 
			   ((insideBorder.y || insideBorder.z) && insideBorder.x);
	}
	
	return false;
}