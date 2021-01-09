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

#include "Octree.h"
#include <cmath>

namespace VolumeRaytracer
{
	namespace Internal
	{
		const VIntVector OCTREE_NODE_INDICES[8] = {
			VIntVector(0,0,0),
			VIntVector(1,0,0),
			VIntVector(0,1,0),
			VIntVector(1,1,0),
			VIntVector(0,0,1),
			VIntVector(1,0,1),
			VIntVector(0,1,1),
			VIntVector(1,1,1)
		};
	}
}

VolumeRaytracer::Voxel::VCellOctreeNode::VCellOctreeNode(const size_t& depth)
	:Depth(depth)
{
	VoxelCell = std::make_shared<VCell>();
}

VolumeRaytracer::Voxel::VCellOctreeNode::~VCellOctreeNode()
{}

void VolumeRaytracer::Voxel::VCellOctreeNode::ToBranch()
{
	if (IsLeaf())
	{
		Children.clear();

		for (int i = 0; i < 8; i++)
		{
			Children.push_back(std::make_shared<VCellOctreeNode>(GetDepth() + 1));
			Children[i]->ToLeaf(*VoxelCell);
		}

		VoxelCell = nullptr;
	}
}

void VolumeRaytracer::Voxel::VCellOctreeNode::ToLeaf(const VCell& cell)
{
	Children.clear();

	if (!IsLeaf())
	{
		VoxelCell = std::make_shared<VCell>();
	}

	*VoxelCell = cell;
}

bool VolumeRaytracer::Voxel::VCellOctreeNode::CanBeSimplified() const
{
	if (IsLeaf())
	{
		return !VoxelCell->HasSurface();
	}
	else
	{
		bool canBeCollapsed = true;

		for (const auto& child : Children)
		{
			canBeCollapsed &= child->CanBeSimplified();
		}

		return canBeCollapsed;
	}
}

bool VolumeRaytracer::Voxel::VCellOctreeNode::TryToMergeNodes()
{
	if (IsLeaf())
	{
		return !VoxelCell->HasSurface();
	}
	else
	{
		bool mergeSuccess = true;

		for (auto& child : Children)
		{
			mergeSuccess &= child->TryToMergeNodes();
		}

		if (mergeSuccess)
		{
			VCell cell = Children[0]->GetCell();
			bool inSurface = cell.Voxels[0].Density <= 0;

			VVoxel voxel;
			voxel.Density = inSurface ? -VVoxel::DEFAULT_DENSITY : VVoxel::DEFAULT_DENSITY;
			voxel.Material = cell.Voxels[0].Material;

			cell.FillWithVoxel(voxel);

			ToLeaf(cell);

			return true;
		}
		else
		{
			return false;
		}
	}

	return false;
}

bool VolumeRaytracer::Voxel::VCellOctreeNode::IsLeaf() const
{
	return VoxelCell != nullptr;
}

std::shared_ptr<VolumeRaytracer::Voxel::VCellOctreeNode> VolumeRaytracer::Voxel::VCellOctreeNode::GetChild(const uint8_t& childIndex) const
{
	if (childIndex < Children.size())
	{
		return Children[childIndex];
	}
	else
	{
		return nullptr;
	}
}

VolumeRaytracer::Voxel::VCell VolumeRaytracer::Voxel::VCellOctreeNode::GetCell() const
{
	return *VoxelCell;
}

size_t VolumeRaytracer::Voxel::VCellOctreeNode::GetDepth() const
{
	return Depth;
}

VolumeRaytracer::Voxel::VCellOctree::VCellOctree(const uint8_t& maxDepth, const VVoxel& fillerVoxel)
	: MaxDepth(maxDepth)
{
	VCell cell;
	cell.FillWithVoxel(fillerVoxel);

	Root = std::make_shared<VCellOctreeNode>(0);
	Root->ToLeaf(cell);
}

VolumeRaytracer::Voxel::VCellOctree::~VCellOctree()
{}

VolumeRaytracer::Voxel::VVoxel VolumeRaytracer::Voxel::VCellOctree::GetVoxel(const VIntVector& voxelIndex) const
{
	if(!IsValidVoxelIndex(voxelIndex))
		return VVoxel();

	VIntVector cellIndex = voxelIndex.Min(voxelIndex, GetVoxelCountAlongAxis() - 2);

	std::shared_ptr<VCellOctreeNode> node = GetOctreeNode(Root, VIntVector::ZERO, cellIndex);

	VIntVector cellVoxelIndex = voxelIndex - cellIndex;

	uint8_t ind = VCell::GetVoxelIndex(cellVoxelIndex);

	assert(ind < 8);

	return node->GetCell().Voxels[ind];
}

void VolumeRaytracer::Voxel::VCellOctree::SetVoxel(const VIntVector& voxelIndex, const VVoxel& voxel)
{
	if(!IsValidVoxelIndex(voxelIndex))
		return;

	std::vector<VIntVector> cellOffset;
	std::vector<VIntVector> voxelIndices;

	VIntVector cellIndex = voxelIndex.Min(voxelIndex, GetVoxelCountAlongAxis() - 2);
	VIntVector cellVoxelIndex = voxelIndex - cellIndex;

	GetNeighbouringVoxelIndices(cellVoxelIndex, cellOffset, voxelIndices);

	for (int i = 0; i < 8; i++)
	{
		VIntVector currentCellIndex = cellIndex + cellOffset[i];

		if (IsValidCellIndex(currentCellIndex))
		{
			std::shared_ptr<VCellOctreeNode> node = GetOctreeNodeForEditing(Root, VIntVector::ZERO, currentCellIndex, true);

			VCell cell = node->GetCell();

			uint8_t ind = VCell::GetVoxelIndex(voxelIndices[i]);

			assert(ind < 8);

			cell.Voxels[ind] = voxel;

			node->ToLeaf(cell);
		}
	}
}

bool VolumeRaytracer::Voxel::VCellOctree::IsValidVoxelIndex(const VIntVector& voxelIndex) const
{
	size_t voxelAxisCount = GetVoxelCountAlongAxis();

	return	voxelIndex.X >= 0 && voxelIndex.X < voxelAxisCount&&
			voxelIndex.Y >= 0 && voxelIndex.Y < voxelAxisCount&&
			voxelIndex.Z >= 0 && voxelIndex.Z < voxelAxisCount;
}

bool VolumeRaytracer::Voxel::VCellOctree::IsValidCellIndex(const VIntVector& cellIndex) const
{
	size_t cellAxisCount = GetCellCountAlongAxis();

	return  cellIndex.X >= 0 && cellIndex.X < cellAxisCount &&
			cellIndex.Y >= 0 && cellIndex.Y < cellAxisCount &&
			cellIndex.Z >= 0 && cellIndex.Z < cellAxisCount;
}

size_t VolumeRaytracer::Voxel::VCellOctree::GetMaxDepth() const
{
	return MaxDepth;
}

size_t VolumeRaytracer::Voxel::VCellOctree::GetVoxelCountAlongAxis() const
{
	return 2 + (std::pow(2, MaxDepth) - 1);
}

size_t VolumeRaytracer::Voxel::VCellOctree::GetCellCountAlongAxis() const
{
	return std::pow(2, MaxDepth);
}

size_t VolumeRaytracer::Voxel::VCellOctree::GetVoxelCount() const
{
	size_t axisCount = GetVoxelCountAlongAxis();

	return axisCount * axisCount * axisCount;
}

void VolumeRaytracer::Voxel::VCellOctree::CollapseTree()
{
	Root->TryToMergeNodes();
}

void VolumeRaytracer::Voxel::VCellOctree::GetGPUOctreeStructure(std::vector<VCellGPUOctreeNode>& outNodes, size_t& outNodeAxisCount) const
{
	std::vector<std::shared_ptr<VCellOctreeNode>> nodes;
	GetAllNodes(Root, nodes);

	size_t size;
	size = std::ceil(std::cbrtf(nodes.size()));

	outNodeAxisCount = size;

	outNodes.push_back(VCellGPUOctreeNode());

	GetGPUNodes(Root, size, 0, outNodes);
}

VolumeRaytracer::VIntVector VolumeRaytracer::Voxel::VCellOctree::CalculateOctreeNodeIndex(const VIntVector& parentIndex, const VIntVector& relativeIndex, const size_t& currentDepth) const
{
	int nodeCount = std::pow(2, MaxDepth - currentDepth);

	VIntVector nodeCountVec = VIntVector::ONE * nodeCount;

	return parentIndex + nodeCountVec - (nodeCountVec / (relativeIndex + VIntVector::ONE));
}

std::shared_ptr<VolumeRaytracer::Voxel::VCellOctreeNode> VolumeRaytracer::Voxel::VCellOctree::GetOctreeNode(const std::shared_ptr<VCellOctreeNode>& parent, const VIntVector& parentIndex, const VIntVector& cellIndex) const
{
	if (parent->IsLeaf())
	{
		return parent;
	}

	for (int i = 0; i < 8; i++)
	{
		VIntVector relIndex = Internal::OCTREE_NODE_INDICES[i];
		VIntVector minNodeIndex = CalculateOctreeNodeIndex(parentIndex, relIndex, parent->GetDepth());
		VIntVector maxNodeIndex = minNodeIndex + VIntVector::ONE * std::pow(2, MaxDepth - (parent->GetDepth() + 1));

		if (cellIndex >= minNodeIndex && cellIndex < maxNodeIndex)
		{
			return GetOctreeNode(parent->GetChild(i), minNodeIndex, cellIndex);
		}
	}

	return parent;
}

std::shared_ptr<VolumeRaytracer::Voxel::VCellOctreeNode> VolumeRaytracer::Voxel::VCellOctree::GetOctreeNodeForEditing(std::shared_ptr<VCellOctreeNode> parent, const VIntVector& parentIndex, const VIntVector& nodeIndex, bool subdivide /*= true*/)
{
	if (parent->IsLeaf())
	{
		if (parent->GetDepth() == MaxDepth || !subdivide)
		{
			return parent;
		}
		else
		{
			parent->ToBranch();
		}
	}

	for (int i = 0; i < 8; i++)
	{
		VIntVector relIndex = Internal::OCTREE_NODE_INDICES[i];
		VIntVector minNodeIndex = CalculateOctreeNodeIndex(parentIndex, relIndex, parent->GetDepth());
		VIntVector maxNodeIndex = minNodeIndex + VIntVector::ONE * std::pow(2, MaxDepth - (parent->GetDepth() + 1));

		if (nodeIndex >= minNodeIndex && nodeIndex < maxNodeIndex)
		{
			std::shared_ptr<VolumeRaytracer::Voxel::VCellOctreeNode> node = parent->GetChild(i);

			return GetOctreeNodeForEditing(node, minNodeIndex, nodeIndex, subdivide);
		}
	}

	return parent->GetChild(0);
}

void VolumeRaytracer::Voxel::VCellOctree::GetNeighbouringVoxelIndices(const VIntVector& cellVoxelIndex, std::vector<VIntVector>& outCellOffsets, std::vector<VIntVector>& outCellVoxelIndex)
{
	if (cellVoxelIndex == VIntVector(0, 0, 0))
	{
		outCellOffsets = {
			VIntVector(0,0,0),
			VIntVector(-1,0,0),
			VIntVector(0,-1,0),
			VIntVector(-1,-1,0),
			VIntVector(0,0,-1),
			VIntVector(-1,0,-1),
			VIntVector(0,-1,-1),
			VIntVector(-1,-1,-1)
		};

		outCellVoxelIndex = {
			VIntVector(0,0,0),
			VIntVector(1,0,0),
			VIntVector(0,1,0),
			VIntVector(1,1,0),
			VIntVector(0,0,1),
			VIntVector(1,0,1),
			VIntVector(0,1,1),
			VIntVector(1,1,1),
		};
	}
	else if (cellVoxelIndex == VIntVector(1, 0, 0))
	{
		outCellOffsets = {
				VIntVector(0,0,0),
				VIntVector(1,0,0),
				VIntVector(0,-1,0),
				VIntVector(1,-1,0),
				VIntVector(0,0,-1),
				VIntVector(1,0,-1),
				VIntVector(0,-1,-1),
				VIntVector(1,-1,-1)
		};

		outCellVoxelIndex = {
			VIntVector(1,0,0),
			VIntVector(0,0,0),
			VIntVector(1,1,0),
			VIntVector(0,1,0),
			VIntVector(1,0,1),
			VIntVector(0,0,1),
			VIntVector(1,1,1),
			VIntVector(0,1,1),
		};
	}
	else if (cellVoxelIndex == VIntVector(0, 1, 0))
	{
		outCellOffsets = {
				VIntVector(0,0,0),
				VIntVector(-1,0,0),
				VIntVector(0,1,0),
				VIntVector(-1,1,0),
				VIntVector(0,0,-1),
				VIntVector(-1,0,-1),
				VIntVector(0,1,-1),
				VIntVector(-1,1,-1)
		};

		outCellVoxelIndex = {
			VIntVector(0,1,0),
			VIntVector(1,1,0),
			VIntVector(0,0,0),
			VIntVector(1,0,0),
			VIntVector(0,1,1),
			VIntVector(1,1,1),
			VIntVector(0,0,1),
			VIntVector(1,0,1),
		};
	}
	else if (cellVoxelIndex == VIntVector(1, 1, 0))
	{
		outCellOffsets = {
				VIntVector(0,0,0),
				VIntVector(1,0,0),
				VIntVector(0,1,0),
				VIntVector(1,1,0),
				VIntVector(0,0,-1),
				VIntVector(1,0,-1),
				VIntVector(0,1,-1),
				VIntVector(1,1,-1)
		};

		outCellVoxelIndex = {
			VIntVector(1,1,0),
			VIntVector(0,1,0),
			VIntVector(1,0,0),
			VIntVector(0,0,0),
			VIntVector(1,1,1),
			VIntVector(0,1,1),
			VIntVector(1,0,1),
			VIntVector(0,0,1),
		};
	}
	else if (cellVoxelIndex == VIntVector(0, 0, 1))
	{
		outCellOffsets = {
				VIntVector(0,0,0),
				VIntVector(-1,0,0),
				VIntVector(0,-1,0),
				VIntVector(-1,-1,0),
				VIntVector(0,0,1),
				VIntVector(-1,0,1),
				VIntVector(0,-1,1),
				VIntVector(-1,-1,1)
		};

		outCellVoxelIndex = {
			VIntVector(0,0,1),
			VIntVector(1,0,1),
			VIntVector(0,1,1),
			VIntVector(1,1,1),
			VIntVector(0,0,0),
			VIntVector(1,0,0),
			VIntVector(0,1,0),
			VIntVector(1,1,0),
		};
	}
	else if (cellVoxelIndex == VIntVector(1, 0, 1))
	{
		outCellOffsets = {
				VIntVector(0,0,0),
				VIntVector(1,0,0),
				VIntVector(0,-1,0),
				VIntVector(1,-1,0),
				VIntVector(0,0,1),
				VIntVector(1,0,1),
				VIntVector(0,-1,1),
				VIntVector(1,-1,1)
		};

		outCellVoxelIndex = {
			VIntVector(1,0,1),
			VIntVector(0,0,1),
			VIntVector(1,1,1),
			VIntVector(0,1,1),
			VIntVector(1,0,0),
			VIntVector(0,0,0),
			VIntVector(1,1,0),
			VIntVector(0,1,0),
		};
	}
	else if (cellVoxelIndex == VIntVector(0, 1, 1))
	{
		outCellOffsets = {
				VIntVector(0,0,0),
				VIntVector(-1,0,0),
				VIntVector(0,1,0),
				VIntVector(-1,1,0),
				VIntVector(0,0,1),
				VIntVector(-1,0,1),
				VIntVector(0,1,1),
				VIntVector(-1,1,1)
		};

		outCellVoxelIndex = {
			VIntVector(0,1,1),
			VIntVector(1,1,1),
			VIntVector(0,0,1),
			VIntVector(1,0,1),
			VIntVector(0,1,0),
			VIntVector(1,1,0),
			VIntVector(0,0,0),
			VIntVector(1,0,0),
		};
	}
	else
	{
		outCellOffsets = {
			VIntVector(0,0,0),
			VIntVector(1,0,0),
			VIntVector(0,1,0),
			VIntVector(1,1,0),
			VIntVector(0,0,1),
			VIntVector(1,0,1),
			VIntVector(0,1,1),
			VIntVector(1,1,1)
		};

		outCellVoxelIndex = {
			VIntVector(1,1,1),
			VIntVector(0,1,1),
			VIntVector(1,0,1),
			VIntVector(0,0,1),
			VIntVector(1,1,0),
			VIntVector(0,1,0),
			VIntVector(1,0,0),
			VIntVector(0,0,0),
		};
	}
}

void VolumeRaytracer::Voxel::VCellOctree::GetAllNodes(std::shared_ptr<VCellOctreeNode> node, std::vector<std::shared_ptr<VCellOctreeNode>>& nodes) const
{
	nodes.push_back(node);

	if (!node->IsLeaf())
	{
		for (int i = 0; i < 8; i++)
		{
			GetAllNodes(node->GetChild(i), nodes);
		}
	}
}

void VolumeRaytracer::Voxel::VCellOctree::GetGPUNodes(const std::shared_ptr<VCellOctreeNode>& node, const size_t& gpuVolumeSize, const size_t& currentNodeIndex, std::vector<VCellGPUOctreeNode>& outGpuNodes) const
{
	if (node->IsLeaf())
	{
		VCellGPUOctreeNode& currentNode = outGpuNodes[currentNodeIndex];

		currentNode.IsLeaf = true;
		currentNode.Cell = node->GetCell();
	}
	else
	{
		int firstChildIndex = outGpuNodes.size();

		outGpuNodes.push_back(VCellGPUOctreeNode());
		outGpuNodes.push_back(VCellGPUOctreeNode());
		outGpuNodes.push_back(VCellGPUOctreeNode());
		outGpuNodes.push_back(VCellGPUOctreeNode());
		outGpuNodes.push_back(VCellGPUOctreeNode());
		outGpuNodes.push_back(VCellGPUOctreeNode());
		outGpuNodes.push_back(VCellGPUOctreeNode());
		outGpuNodes.push_back(VCellGPUOctreeNode());

		outGpuNodes[currentNodeIndex].IsLeaf = false;

		for (int i = 0; i < 8; i++)
		{
			int index = firstChildIndex + i;
			VIntVector index3D = VMathHelpers::Index1DTo3D(index, gpuVolumeSize, gpuVolumeSize);
			
			outGpuNodes[currentNodeIndex].Children.push_back(index3D);

			GetGPUNodes(node->GetChild(i), gpuVolumeSize, index, outGpuNodes);
		}
	}
}
