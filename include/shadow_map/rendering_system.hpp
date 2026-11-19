#pragma once

#include <cstddef>
#include <memory>

#include "rendering/engine/rendering_system.hpp"

namespace spk::ShadowMap
{
	class DepthCommand;

	class RenderingSystem final : public spk::RenderingSystem
	{
	private:
		friend class DepthCommand;
		struct State;
		std::shared_ptr<State> _state;

	public:
		RenderingSystem();
		[[nodiscard]] std::size_t maxPointShadowRebuildsPerImage() const noexcept;
		void setMaxPointShadowRebuildsPerImage(std::size_t value) noexcept;
		void buildViewRenderSnapshot(
			spk::RenderSnapshot::Builder &builder,
			const spk::EngineView::CaptureContext &context) override;
	};
}
