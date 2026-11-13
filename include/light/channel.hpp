#pragma once

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

#include "light/prioritizer.hpp"
#include "math/camera_3d.hpp"

namespace spk
{
	class Engine;
}

namespace spk::Light
{
	// One Channel belongs to one EngineView. Its published Selection is never mutated.
	// Source mutations use Sparkle's existing modify-then-notify contract: applications
	// must coordinate concurrent mutations and capture of mutable scene objects.
	class Channel final
	{
	private:
		Engine *_engine;
		mutable std::mutex _mutex;
		std::condition_variable _ready;
		std::size_t _limit = 8;
		float _hysteresis = 0.10f;
		std::uint64_t _generation = 0;
		std::uint64_t _publishedGeneration = 0;
		std::uint64_t _lastAdmission = 0;
		bool _resetNext = true;
		bool _building = false;
		bool _closed = false;
		std::vector<std::uint64_t> _history;
		std::shared_ptr<const Selection> _published;

		void _invalidateLocked();

	public:
		explicit Channel(Engine &engine) noexcept;
		~Channel() = default;
		Channel(const Channel &) = delete;
		Channel &operator=(const Channel &) = delete;

		[[nodiscard]] std::size_t limit() const;
		void setLimit(std::size_t limit);
		[[nodiscard]] float hysteresis() const;
		void setHysteresis(float hysteresis);
		[[nodiscard]] std::uint64_t generation() const;
		[[nodiscard]] std::uint64_t publishedGeneration() const;
		[[nodiscard]] std::shared_ptr<const Selection> current() const;
		[[nodiscard]] std::shared_ptr<const Selection> acquire(const Camera3D &camera, std::uint64_t captureOrdinal);
		[[nodiscard]] bool isAdmissionCurrent(std::uint64_t registrationOrder, std::uint64_t admission) const;
		[[nodiscard]] bool isClosed() const;

		// Called by Engine's existing source-edition notifications; no immediate rebuild.
		void invalidate();
		// Called during Engine::removeView while Engine owns its view mutex.
		void close() noexcept;
	};
}
