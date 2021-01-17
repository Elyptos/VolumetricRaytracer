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
	ToLeaf(VCell(), VIntVector::ZERO);
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
			Children[i]->ToLeaf(VoxelCell, Index);
		}

		Leaf = false;
	}
}

void VolumeRaytracer::Voxel::VCellOctreeNode::ToLeaf(const VCell& cell, const VIntVector& index)
{
	Children.clear();

	VoxelCell = cell;
	Index = index;

	Leaf = true;
}

bool VolumeRaytracer::Voxel::VCellOctreeNode::TryToMergeNodes()
{
	if (IsLeaf())
	{
		return !VoxelCell.HasSurface();
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
			VIntVector firstIndex = Children[0]->GetIndex();

			VCell cell;

			for (int i = 0; i < 8; i++)
			{
				cell.Voxels[i] = Children[i]->GetCell().GetAvgVoxel();
			}

			ToLeaf(cell, firstIndex);

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
	return Leaf;
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
	return VoxelCell;
}

const VolumeRaytracer::VIntVector& VolumeRaytracer::Voxel::VCellOctreeNode::GetIndex() const
{
	return Index;
}

size_t VolumeRaytracer::Voxel::VCellOctreeNode::GetDepth() const
{
	return Depth;
}

void VolumeRaytracer::Voxel::VCellOctreeNode::SetChildren(const std::vector<std::shared_ptr<VCellOctreeNode>>& children)
{
	assert(children.size() == 8);
	
	Leaf = false;

	Children = std::vector<std::shared_ptr<VCellOctreeNode>>(children);
}

VolumeRaytracer::Voxel::VCellOctree::VCellOctree(const uint8_t& maxDepth, const std::vector<VVoxel>& voxelArray)
	:MaxDepth(maxDepth)
{
	GenerateOctreeFromVoxelVolume(GetVoxelCountAlongAxis(), voxelArray);
}

VolumeRaytracer::Voxel::VCellOctree::~VCellOctree()
{}

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

void VolumeRaytracer::Voxel::VCellOctree::GenerateOctreeFromVoxelVolume(const size_t& voxelAxisCount, const std::vector<VVoxel>& voxelArray)
{
	std::vector<std::shared_ptr<VCellOctreeNode>> nodes;

	int cellCountAlongAxis = GetCellCountAlongAxis();
	int totalCellCount = cellCountAlongAxis * cellCountAlongAxis * cellCountAlongAxis;

	nodes.resize(totalCellCount);

	//#pragma omp parallel for 
	for (int i = 0; i < totalCellCount; i++)
	{
		size_t voxelIndex1D = (size_t)i;
		VIntVector cellIndex = VMathHelpers::Index1DTo3D(voxelIndex1D, cellCountAlongAxis, cellCountAlongAxis);

		VCell cell;

		for (int v = 0; v < 8; v++)
		{
			cell.Voxels[v] = voxelArray[VMathHelpers::Index3DTo1D(cellIndex + VCell::VOXEL_COORDS[v], voxelAxisCount, voxelAxisCount)];
		}

		nodes[i] = std::make_shared<VCellOctreeNode>(MaxDepth);
		nodes[i]->ToLeaf(cell, cellIndex);
	}

	for (int depth = MaxDepth - 1; depth >= 0; depth--)
	{
		int internalAxisCount = std::pow(2, MaxDepth - depth);
		int voxelIndexOffset = internalAxisCount / 2;

		//#pragma omp parallel for collapse(3)
		for (int x = 0; x < (cellCountAlongAxis - 1); x += internalAxisCount)
		{
			for (int y = 0; y < (cellCountAlongAxis - 1); y += internalAxisCount)
			{
				for (int z = 0; z < (cellCountAlongAxis - 1); z += internalAxisCount)
				{
					VIntVector cellIndex3D = VIntVector(x, y, z);
					size_t cellIndex1D = VMathHelpers::Index3DTo1D(cellIndex3D, cellCountAlongAxis, cellCountAlongAxis);

					std::shared_ptr<VCellOctreeNode> parentNode = std::make_shared<VCellOctreeNode>(depth);
					std::vector<std::shared_ptr<VCellOctreeNode>> childNodes;
					childNodes.resize(8);

					for (int i = 0; i < 8; i++)
					{
						VIntVector childIndex = cellIndex3D + VCell::VOXEL_COORDS[i] * voxelIndexOffset;

						childNodes[i] = nodes[VMathHelpers::Index3DTo1D(childIndex, cellCountAlongAxis, cellCountAlongAxis)];
					}

					parentNode->SetChildren(childNodes);

					nodes[cellIndex1D] = parentNode;
				}	
			}
		}
	}

	Root = nodes[0];
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

void VolumeRaytracer::Voxel::VCellOctree::GetAllBranchNodes(const std::shared_ptr<VCellOctreeNode>& node, std::vector<std::shared_ptr<VCellOctreeNode>>& nodes) const
{
	if (!node->IsLeaf())
	{
		nodes.push_back(node);

		for (int i = 0; i < 8; i++)
		{
			GetAllBranchNodes(node->GetChild(i), nodes);
		}
	}
}

void VolumeRaytracer::Voxel::VCellOctree::GetGPUNodes(const std::shared_ptr<VCellOctreeNode>& node, const size_t& gpuVolumeSize, const size_t& currentNodeIndex, std::vector<VCellGPUOctreeNode>& outGpuNodes) const
{
	if (node->IsLeaf())
	{
		outGpuNodes[currentNodeIndex].IsLeaf = true;
		outGpuNodes[currentNodeIndex].CellIndex = node->GetIndex();
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
