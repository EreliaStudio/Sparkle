#pragma once

#include <cstddef>
#include <cstdint>
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
		enum class StateScopeMode
		{
			Full,
			DepthOnly
		};

		enum class DepthSubmissionMode
		{
			Individual,
			Batched,
			BatchedCached,
			BatchedVector,
			BatchedPrecomputed
		};

		void setDepthSubmissionMode(DepthSubmissionMode mode) noexcept;
		[[nodiscard]] DepthSubmissionMode depthSubmissionMode() const noexcept;

		enum class CascadeMode : std::uint8_t
		{
			SinglePass = 1,
			DualPass = 2,
			TriplePass = 3,
			QuadraPass = 4
		};

		RenderingSystem();
		void setDirectionalCascadeMode(CascadeMode mode);
		[[nodiscard]] CascadeMode directionalCascadeMode() const noexcept;
		void setStateScopeMode(StateScopeMode mode) noexcept;
		[[nodiscard]] StateScopeMode stateScopeMode() const noexcept;
		// Single-map mode is retained for diagnostic A/B comparisons.
		void setDirectionalCascadesEnabled(bool enabled) noexcept;
		[[nodiscard]] bool directionalCascadesEnabled() const noexcept;
		[[nodiscard]] std::size_t maxPointShadowRebuildsPerImage() const noexcept;
		void setMaxPointShadowRebuildsPerImage(std::size_t value) noexcept;
		void buildViewRenderSnapshot(
			spk::RenderSnapshot::Builder &builder,
			const spk::EngineView::CaptureContext &context) override;
	};
}
