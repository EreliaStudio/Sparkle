#pragma once

#include <cstdint>
#include <stdexcept>

#include "core/window_surface_render_target.hpp"
#include "diagnostics/profiler.hpp"
#include "math/vector2.hpp"

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

		[[nodiscard]] static Window::Surface &_requireSurface(Window::Surface *surface)
		{
			if (surface == nullptr)
			{
				throw std::invalid_argument("RenderContext requires a surface");
			}
			return *surface;
		}

	public:
		Window::Surface *targetSurface;
		Profiler &profiler;
		ActiveProgram _activeProgram{};
		Window::Surface::RenderTarget activeRenderTarget;

		explicit RenderContext(
			Window::Surface &surface,
			Profiler &profiler = Profiler::defaultProfiler()) :
			targetSurface(&surface),
			profiler(profiler),
			activeRenderTarget(surface.defaultRenderTarget())
		{
		}

		explicit RenderContext(
			Window::Surface *surface,
			Profiler &profiler = Profiler::defaultProfiler()) :
			RenderContext(_requireSurface(surface), profiler)
		{
		}

		[[nodiscard]] Vector2UInt renderTargetSize() const
		{
			return activeRenderTarget.size();
		}
	};
}
