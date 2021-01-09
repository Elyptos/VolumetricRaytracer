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

#pragma once

#include "Voxel.h"
#include "Vector.h"
#include <memory>
#include <vector>

namespace VolumeRaytracer
{
	namespace Voxel
	{
		class VCellOctreeNode
		{
		public:
			VCellOctreeNode(const size_t& depth);
			~VCellOctreeNode();

			void ToBranch();
			void ToLeaf(const VCell& cell);

			bool CanBeSimplified() const;
			bool TryToMergeNodes();

			bool IsLeaf() const;
			std::shared_ptr<VCellOctreeNode> GetChild(const uint8_t& childIndex) const;
			VCell GetCell() const;
			size_t GetDepth() const;

		private:
			std::vector<std::shared_ptr<VCellOctreeNode>> Children;
			std::shared_ptr<VCell> VoxelCell = nullptr;
			size_t Depth;
		};

		struct VCellGPUOctreeNode
		{
		public:
			bool IsLeaf;
			VCell Cell;
			std::vector<VIntVector> Children;
		};

		class VCellOctree
		{
		public:
			VCellOctree(const uint8_t& maxDepth, const VVoxel& fillerVoxel);
			~VCellOctree();

			VVoxel GetVoxel(const VIntVector& voxelIndex) const;
			void SetVoxel(const VIntVector& voxelIndex, const VVoxel& voxel);
			bool IsValidVoxelIndex(const VIntVector& voxelIndex) const;
			bool IsValidCellIndex(const VIntVector& cellIndex) const;

			size_t GetMaxDepth() const;
			size_t GetVoxelCountAlongAxis() const;
			size_t GetCellCountAlongAxis() const;
			size_t GetVoxelCount() const;

			void CollapseTree();

			void GetGPUOctreeStructure(std::vector<VCellGPUOctreeNode>& outNodes, size_t& outNodeAxisCount) const;

		private:
			VIntVector CalculateOctreeNodeIndex(const VIntVector& parentIndex, const VIntVector& relativeIndex, const size_t& currentDepth) const;
			std::shared_ptr<VCellOctreeNode> GetOctreeNode(const std::shared_ptr<VCellOctreeNode>& parent, const VIntVector& parentIndex, const VIntVector& cellIndex) const;
			
			std::shared_ptr<VCellOctreeNode> GetOctreeNodeForEditing(std::shared_ptr<VCellOctreeNode> parent, const VIntVector& parentIndex, const VIntVector& nodeIndex, bool subdivide = true);

			void GetNeighbouringVoxelIndices(const VIntVector& cellVoxelIndex, std::vector<VIntVector>& outCellOffsets, std::vector<VIntVector>& outCellVoxelIndex);

			void GetAllNodes(std::shared_ptr<VCellOctreeNode> node, std::vector<std::shared_ptr<VCellOctreeNode>>& nodes) const;
			void GetGPUNodes(const std::shared_ptr<VCellOctreeNode>& node, const size_t& gpuVolumeSize, const size_t& currentNodeIndex, std::vector<VCellGPUOctreeNode>& outGpuNodes) const;

		private:
			size_t MaxDepth;
			std::shared_ptr<VCellOctreeNode> Root;
		};
	}
}