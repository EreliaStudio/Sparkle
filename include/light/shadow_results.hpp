#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "container/thread_safe_slot.hpp"
#include "core/window.hpp"
#include "light/shadow_result.hpp"

namespace spk::Light
{
	class Channel;

	// Logically owned by one Channel, with a distinct immutable slot for each
	// incarnation of a Window::Surface. Does not create or own native GL objects.
	class ShadowResults final
	{
	private:
		friend class Channel;

		struct Entry final
		{
			ThreadSafeSlot<ShadowResult> slot;
			std::atomic_bool closed = false;
			Window::Surface::DestructionContract destruction;
		};

		struct State final
		{
			std::mutex mutex;
			std::unordered_map<std::uint64_t, std::shared_ptr<Entry>> entries;
		};

		std::shared_ptr<State> _state = std::make_shared<State>();

		[[nodiscard]] std::shared_ptr<Entry> _find(const Window::Surface &surface) const;
		[[nodiscard]] std::shared_ptr<Entry> _getOrCreate(Window::Surface &surface);

	public:
		ShadowResults() = default;
		ShadowResults(const ShadowResults &) = delete;
		ShadowResults &operator=(const ShadowResults &) = delete;

		// Read-only: CPU threads must never become producers of a surface slot.
		[[nodiscard]] std::shared_ptr<const ShadowResult> acquireLatest(const Window::Surface &surface) const;
	};
}
