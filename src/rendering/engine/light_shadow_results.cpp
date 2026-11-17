#include "light/shadow_results.hpp"

#include <stdexcept>
#include <utility>

namespace spk::Light
{
	std::shared_ptr<ShadowResults::Entry> ShadowResults::_find(const Window::Surface &surface) const
	{
		std::lock_guard lock(_state->mutex);
		const auto it = _state->entries.find(surface.identity());
		return it == _state->entries.end() ? nullptr : it->second;
	}

	std::shared_ptr<ShadowResults::Entry> ShadowResults::_getOrCreate(Window::Surface &surface)
	{
		if (surface.lifeCycle() != Window::LifeCycle::Ready)
		{
			return nullptr;
		}
		if (auto existing = _find(surface); existing != nullptr)
		{
			return !existing->closed.load(std::memory_order_acquire) &&
						   surface.lifeCycle() == Window::LifeCycle::Ready
					   ? existing
					   : nullptr;
		}

		auto candidate = std::make_shared<Entry>();
		const std::weak_ptr<State> weakState = _state;
		const std::weak_ptr<Entry> weakEntry = candidate;
		const std::uint64_t identity = surface.identity();

		try
		{
			// Surface serializes subscription with its transition to Releasing.
			// This callback owns neither the Channel nor the Entry.
			candidate->destruction = surface.subscribeToDestruction([weakState, weakEntry, identity] {
				auto entry = weakEntry.lock();
				if (entry == nullptr)
				{
					return;
				}
				auto state = weakState.lock();
				if (state == nullptr)
				{
					entry->closed.store(true, std::memory_order_release);
					return;
				}

				std::shared_ptr<Entry> retired;
				{
					std::lock_guard lock(state->mutex);
					entry->closed.store(true, std::memory_order_release);
					const auto it = state->entries.find(identity);
					if (it != state->entries.end() && it->second == entry)
					{
						retired = std::move(it->second);
						state->entries.erase(it);
					}
				}
				// A possibly reentrant RAII resignation occurs only after unlock.
			});
		} catch (const std::logic_error &)
		{
			// Another thread began releasing the Surface before subscription.
			return nullptr;
		}

		std::shared_ptr<Entry> result;
		{
			std::lock_guard lock(_state->mutex);
			if (!candidate->closed.load(std::memory_order_acquire) &&
				surface.lifeCycle() == Window::LifeCycle::Ready)
			{
				auto [it, inserted] = _state->entries.try_emplace(identity, candidate);
				result = it->second;
			}
		}
		// A discarded candidate resigns its contract outside the dictionary lock.
		if (result != nullptr && result->closed.load(std::memory_order_acquire))
		{
			return nullptr;
		}
		return result;
	}

	std::shared_ptr<const ShadowResult> ShadowResults::acquireLatest(const Window::Surface &surface) const
	{
		if (surface.lifeCycle() != Window::LifeCycle::Ready)
		{
			return nullptr;
		}
		const auto entry = _find(surface);
		if (entry == nullptr || entry->closed.load(std::memory_order_acquire))
		{
			return nullptr;
		}
		const auto result = entry->slot.acquireLatest();
		return entry->closed.load(std::memory_order_acquire) ||
				   surface.lifeCycle() != Window::LifeCycle::Ready
				   ? nullptr
				   : result;
	}
}
