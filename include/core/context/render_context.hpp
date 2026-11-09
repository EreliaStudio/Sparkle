#pragma once

#include <cstdint>

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

	public:
		Window::Surface *targetSurface;
		Profiler &profiler = Profiler::defaultProfiler();
		ActiveProgram _activeProgram{};

		Window::Surface::RenderTarget activeRenderTarget{};

		[[nodiscard]] Vector2UInt renderTargetSize() const
		{
			return activeRenderTarget.size(*targetSurface);
		}
	};
}
