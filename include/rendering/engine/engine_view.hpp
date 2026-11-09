#pragma once

#include "rendering/render_snapshot.hpp"

namespace spk
{
	class EngineView
	{
	public:
		struct CaptureContext;

		class Contributor
		{
		public:
			virtual ~Contributor() = default;

			virtual void buildViewRenderSnapshot(
				RenderSnapshot::Builder &,
				const CaptureContext &)
			{
			}
		};
	};
}
