#pragma once

#include <GL/glew.h>
#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <string_view>

#include "core/context/render_context.hpp"
#include "rendering/engine/engine_view.hpp"
#include "rendering/render_snapshot.hpp"
#include "shadow_map/profile.hpp"

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
		GLuint _query = 0;

	public:
		struct Sample
		{
			double submission = 0;
			double completion = 0;
			double gpuElapsed = 0;
		};

	private:
		Sample _sample;

		void beforePass(std::string_view name) override
		{
			if (name == spk::EngineView::ShadowPassKey.name)
			{
				glFinish();
				if (_query)
				{
					glBeginQuery(GL_TIME_ELAPSED, _query);
				}
				_begin = Clock::now();
			}
		}

		void afterPass(std::string_view name) override
		{
			if (name == spk::EngineView::ShadowPassKey.name)
			{
				const auto submitted = Clock::now();
				if (_query)
				{
					glEndQuery(GL_TIME_ELAPSED);
				}
				glFinish();
				const auto completed = Clock::now();
				_sample.submission += std::chrono::duration<double, std::milli>(submitted - _begin).count();
				_sample.completion += std::chrono::duration<double, std::milli>(completed - _begin).count();
				if (_query)
				{
					GLuint64 elapsed = 0;
					glGetQueryObjectui64v(_query, GL_QUERY_RESULT, &elapsed);
					_sample.gpuElapsed += static_cast<double>(elapsed) / 1000000.0;
				}
				++_samples;
			}
		}

		class Scope final
		{
		private:
			spk::RenderContext &_context;
			spk::RenderContext::PassObserver *_previous;
			spk::ShadowMap::Profile *_previousProfile;

		public:
			Scope(spk::RenderContext &context, ShadowPassTiming &observer, spk::ShadowMap::Profile *profile) :
				_context(context),
				_previous(context.passObserver),
				_previousProfile(context.shadowProfile)
			{
				_context.passObserver = &observer;
				_context.shadowProfile = profile;
			}

			~Scope()
			{
				_context.passObserver = _previous;
				_context.shadowProfile = _previousProfile;
			}
		};

	public:
		~ShadowPassTiming() override
		{
			if (_query)
			{
				glDeleteQueries(1, &_query);
			}
		}

		[[nodiscard]] static Sample measure(const spk::RenderSnapshot &snapshot, spk::RenderContext &context, spk::ShadowMap::Profile *profile = nullptr)
		{
			ShadowPassTiming observer;
			if (profile)
			{
				glGenQueries(1, &observer._query);
			}
			Scope scope(context, observer, profile);
			snapshot.execute(context);
			glFinish();
			EXPECT_EQ(observer._samples, 1u) << "Expected exactly one captured EngineView shadow pass";
			return observer._sample;
		}
	};
}
