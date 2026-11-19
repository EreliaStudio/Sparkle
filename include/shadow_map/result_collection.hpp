#pragma once

#include <atomic>
#include <map>
#include <memory>
#include <mutex>

#include "container/thread_safe_slot.hpp"
#include "core/window.hpp"
#include "shadow_map/result.hpp"
#include "type/uuid.hpp"

namespace spk::Light
{
	class Channel;
}

namespace spk::ShadowMap
{
	// Logically owned by one Channel, with a distinct immutable slot for each
	// incarnation of a Window::Surface. Does not create or own native GL objects.
	class ResultCollection final
	{
	private:
		friend class spk::Light::Channel;

		struct Entry final
		{
			ThreadSafeSlot<Result> slot;
			std::atomic_bool closed = false;
			Window::Surface::DestructionContract destruction;
		};

		struct State final
		{
			std::mutex mutex;
			std::map<UUID, std::shared_ptr<Entry>> entries;
		};

		std::shared_ptr<State> _state = std::make_shared<State>();

		[[nodiscard]] std::shared_ptr<Entry> _find(const Window::Surface &surface) const;
		[[nodiscard]] std::shared_ptr<Entry> _getOrCreate(Window::Surface &surface);

	public:
		ResultCollection() = default;
		ResultCollection(const ResultCollection &) = delete;
		ResultCollection &operator=(const ResultCollection &) = delete;

		// Read-only: CPU threads must never become producers of a surface slot.
		[[nodiscard]] std::shared_ptr<const Result> acquireLatest(const Window::Surface &surface) const;
	};
}
