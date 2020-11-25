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

#include "Texture.h"
#include <boost/signals2.hpp>

namespace VolumeRaytracer
{
	namespace Renderer
	{
		class VRenderTarget : public VTexture
		{
			typedef boost::signals2::signal<void(VRenderTarget* renderTarget)> GenericRenderTargetDelegate;

		public:
			VRenderTarget() = default;
			virtual ~VRenderTarget();

			virtual void Release();

			unsigned int GetBufferCount() const;
			unsigned int GetBufferIndex() const;

			void SetBufferIndex(unsigned int& bufferIndex);

			boost::signals2::connection OnRenderTargetReleased_Bind(const GenericRenderTargetDelegate::slot_type& del);

		protected:
			unsigned int BufferIndex = 0;
			unsigned int BufferCount = 0;

			void Initialize() override;
			void BeginDestroy() override;

		private:
			GenericRenderTargetDelegate OnRenderTargetReleased;
		};
	}
}