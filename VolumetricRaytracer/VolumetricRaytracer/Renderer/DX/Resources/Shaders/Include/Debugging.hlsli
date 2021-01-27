


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