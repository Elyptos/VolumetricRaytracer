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

#include "LevelObject.h"
#include "Vector.h"
#include "Quat.h"
#include <vector>

namespace VolumeRaytracer
{
	namespace Scene
	{
		class VDensityShape
		{
		public:
			float Evaluate(const VVector& worldPosition) const;

		protected:
			virtual float EvaluateInternal(const VVector& p) const = 0;

		public:
			VVector Position = VVector::ZERO;
			VVector Scale = VVector::ONE;
			VQuat Rotation = VQuat::IDENTITY;
		};

		class VBox : public VDensityShape
		{
		public:
			VVector Extends;
		protected:
			float EvaluateInternal(const VVector& p) const override;

		};

		class VSphere : public VDensityShape
		{
		public:
			float Radius;
		protected:
			float EvaluateInternal(const VVector& p) const override;

		};

		class VCylinder : public VDensityShape
		{
		public:
			float Radius;
			float Height;

		protected:
			float EvaluateInternal(const VVector& p) const override;
		};

		enum class ECombinationType
		{
			ADD = 0,
			SUBTRACT
		};

		class VDensityShapeContainer
		{
		public:
			float Evaluate(const VVector& p) const;
			VDensityShapeContainer& AddChild(std::weak_ptr<VDensityShape> shape, ECombinationType combinationType = ECombinationType::ADD);

		public:
			std::weak_ptr<VDensityShape> Shape;
			ECombinationType CombinationType;
			std::vector<VDensityShapeContainer> Children;
		};

		class VDensityGenerator : public VLevelObject
		{
		public:
			float Evaluate(const VVector& worldPos) const;

			VDensityShapeContainer& GetRootShape();

		private:
			VDensityShapeContainer Root;
		protected:
			void Initialize() override;


			void BeginDestroy() override;

		};
	}
}