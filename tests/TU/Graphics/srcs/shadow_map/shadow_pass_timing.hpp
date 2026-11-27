#pragma once

#include <GL/glew.h>
#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <string_view>

#include "core/context/render_context.hpp"
#include "rendering/engine/engine_view.hpp"
#include "rendering/render_snapshot.hpp"

namespace sparkle_test
{
	// Observe the real nested shadow pass without bypassing EngineView setup.
	// Drain earlier GPU work before measuring; synchronize at the end of the
	// pass, before any skybox, opaque or composition commands are submitted.
	class ShadowPassTiming final : public spk::RenderContext::PassObserver
	{
	private:
		using Clock = std::chrono::steady_clock;
		Clock::time_point _begin{};
		std::size_t _samples = 0;

	public:
		struct Sample
		{
			double submission = 0;
			double completion = 0;
		};

	private:
		Sample _sample;

		void beforePass(std::string_view name) override
		{
			if (name == spk::EngineView::ShadowPassKey.name)
			{
				glFinish();
				_begin = Clock::now();
			}
		}

		void afterPass(std::string_view name) override
		{
			if (name == spk::EngineView::ShadowPassKey.name)
			{
				const auto submitted = Clock::now();
				glFinish();
				const auto completed = Clock::now();
				_sample.submission += std::chrono::duration<double, std::milli>(submitted - _begin).count();
				_sample.completion += std::chrono::duration<double, std::milli>(completed - _begin).count();
				++_samples;
			}
		}

		class Scope final
		{
		private:
			spk::RenderContext &_context;
			spk::RenderContext::PassObserver *_previous;

		public:
			Scope(spk::RenderContext &context, ShadowPassTiming &observer) :
				_context(context),
				_previous(context.passObserver)
			{
				_context.passObserver = &observer;
			}

			~Scope()
			{
				_context.passObserver = _previous;
			}
		};

	public:
		[[nodiscard]] static Sample measure(const spk::RenderSnapshot &snapshot, spk::RenderContext &context)
		{
			ShadowPassTiming observer;
			Scope scope(context, observer);
			snapshot.execute(context);
			glFinish();
			EXPECT_EQ(observer._samples, 1u) << "Expected exactly one captured EngineView shadow pass";
			return observer._sample;
		}
	};
}
