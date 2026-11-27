#pragma once

#include <cstdint>
#include <memory>
#include <string_view>
#include <unordered_map>

#include "core/window_surface_render_target.hpp"
#include "diagnostics/profiler.hpp"

namespace spk
{
	class RenderContextProgramAccess;
	template <typename TType>
	class Uniform;

	struct RenderContext
	{
	private:
		class ActiveProgram
		{
			friend class RenderContextProgramAccess;
			template <typename TType>
			friend class Uniform;

		private:
			Window::Surface *_surface = nullptr;
			GPUResource::Identifier _identifier = 0;
			GPUResource::Generation _generation = 0;
			std::uint32_t _nativeIdentifier = 0;

		public:
			ActiveProgram() = default;
		};

		friend class RenderContextProgramAccess;
		template <typename TType>
		friend class Uniform;

	public:
		// Optional instrumentation of actual render passes, including nested
		// snapshots. The observer is borrowed for the duration of execution.
		class PassObserver
		{
		public:
			virtual ~PassObserver() = default;
			virtual void beforePass(std::string_view name) = 0;
			virtual void afterPass(std::string_view name) = 0;
		};

		PassObserver *passObserver = nullptr;
		Window::Surface *targetSurface;
		Profiler &profiler;
		ActiveProgram _activeProgram{};
		Window::Surface::RenderTarget activeRenderTarget;
		std::unordered_map<std::uint64_t, std::shared_ptr<void>> imageState;

		explicit RenderContext(
			Window::Surface &surface,
			Profiler &profiler = Profiler::defaultProfiler()) :
			targetSurface(&surface),
			profiler(profiler),
			activeRenderTarget(surface.defaultRenderTarget())
		{
		}
	};
}
