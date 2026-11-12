#include "light/channel.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <ranges>
#include <utility>

#include "exception.hpp"
#include "rendering/engine/engine.hpp"

namespace spk::Light
{
	Channel::Channel(Engine &engine) noexcept :
		_engine(&engine)
	{
	}

	void Channel::_invalidateLocked()
	{
		if (_generation == std::numeric_limits<std::uint64_t>::max())
		{
			throw spk::Exception("Light::Channel invalidation generation exhausted");
		}
		++_generation;
	}

	std::size_t Channel::limit() const
	{
		std::lock_guard lock(_mutex);
		return _limit;
	}

	void Channel::setLimit(std::size_t limit)
	{
		std::lock_guard lock(_mutex);
		if (_closed)
		{
			throw spk::Exception("Cannot configure a closed Light::Channel");
		}
		if (limit == _limit)
		{
			return;
		}
		_invalidateLocked();
		_limit = limit;
		_resetNext = true;
	}

	float Channel::hysteresis() const
	{
		std::lock_guard lock(_mutex);
		return _hysteresis;
	}

	void Channel::setHysteresis(float hysteresis)
	{
		if (!std::isfinite(hysteresis) || hysteresis < 0.0f)
		{
			throw spk::Exception("Light::Channel hysteresis must be finite and nonnegative");
		}
		std::lock_guard lock(_mutex);
		if (_closed)
		{
			throw spk::Exception("Cannot configure a closed Light::Channel");
		}
		if (hysteresis == _hysteresis)
		{
			return;
		}
		_invalidateLocked();
		_hysteresis = hysteresis;
		_resetNext = true;
	}

	std::uint64_t Channel::generation() const
	{
		std::lock_guard lock(_mutex);
		return _generation;
	}

	std::uint64_t Channel::publishedGeneration() const
	{
		std::lock_guard lock(_mutex);
		return _publishedGeneration;
	}

	std::shared_ptr<const Selection> Channel::current() const
	{
		std::lock_guard lock(_mutex);
		return _closed || !_published || _publishedGeneration != _generation ? nullptr : _published;
	}

	std::shared_ptr<const Selection> Channel::acquire(const Camera3D &camera, std::uint64_t captureOrdinal)
	{
		{
			std::unique_lock lock(_mutex);
			for (;;)
			{
				if (_closed)
				{
					throw spk::Exception("Cannot acquire a closed Light::Channel");
				}
				if (_published && _publishedGeneration == _generation)
				{
					return _published;
				}
				if (!_building)
				{
					_building = true;
					break;
				}
				_ready.wait(lock, [this] {
					return _closed || !_building || (_published && _publishedGeneration == _generation);
				});
			}
		}

		try
		{
			for (;;)
			{
				std::uint64_t observedGeneration;
				std::size_t limit;
				float hysteresis;
				bool resetHistory;
				std::vector<std::uint64_t> history;
				{
					std::lock_guard lock(_mutex);
					if (_closed)
					{
						throw spk::Exception("Cannot acquire a closed Light::Channel");
					}
					observedGeneration = _generation;
					limit = _limit;
					hysteresis = _hysteresis;
					resetHistory = _resetNext;
					history = _history;
				}

				std::vector<Selection::Entry> candidates;
				{
					// Protect the registry/camera copy against Engine's view registration.
					// Source setters retain the project's existing modify-then-notify policy.
					std::lock_guard engineLock(_engine->_viewMutex);
					candidates = _engine->_captureLightCandidates(camera);
				}

				auto fresh = std::make_shared<Selection>();
				fresh->generation = observedGeneration;
				fresh->captureOrdinal = captureOrdinal;
				for (const auto &entry : candidates)
				{
					if (entry.type == Selection::Type::Directional)
					{
						fresh->directional.push_back(entry);
					}
				}
				std::ranges::sort(fresh->directional, {}, &Selection::Entry::registrationOrder);
				fresh->local = Prioritizer::select(std::move(candidates), history, limit, hysteresis, resetHistory);

				{
					// Publication uses the order Engine -> Channel.
					std::lock_guard engineLock(_engine->_viewMutex);
					std::lock_guard channelLock(_mutex);
					if (_closed)
					{
						throw spk::Exception("Cannot publish to a closed Light::Channel");
					}
					if (observedGeneration != _generation)
					{
						continue; // No history, admissions or reset marker consumed.
					}

					std::uint64_t nextAdmission = _lastAdmission;
					const auto restoreAdmissions = [&](std::vector<Selection::Entry> &entries) {
						for (auto &entry : entries)
						{
							const auto findOld = [&](const std::vector<Selection::Entry> &old) {
								return std::ranges::find_if(old, [&entry](const Selection::Entry &candidate) {
									return candidate.registrationOrder == entry.registrationOrder &&
										   candidate.type == entry.type;
								});
							};
							if (_published)
							{
								const auto &old = entry.type == Selection::Type::Directional ? _published->directional : _published->local;
								const auto it = findOld(old);
								if (it != old.end())
								{
									entry.admission = it->admission;
									continue;
								}
							}
							if (nextAdmission == std::numeric_limits<std::uint64_t>::max())
							{
								throw spk::Exception("Light::Channel admission identity exhausted");
							}
							entry.admission = ++nextAdmission;
						}
					};
					restoreAdmissions(fresh->local);
					restoreAdmissions(fresh->directional);

					std::vector<std::uint64_t> newHistory;
					newHistory.reserve(fresh->local.size());
					for (const auto &entry : fresh->local)
					{
						newHistory.push_back(entry.registrationOrder);
					}
					_history = std::move(newHistory);
					_lastAdmission = nextAdmission;
					_published = fresh;
					_publishedGeneration = observedGeneration;
					_resetNext = false;
					_building = false;
				}
				_ready.notify_all();
				return fresh;
			}
		}
		catch (...)
		{
			{
				std::lock_guard lock(_mutex);
				_building = false;
			}
			_ready.notify_all();
			throw;
		}
	}

	void Channel::invalidate()
	{
		std::lock_guard lock(_mutex);
		if (!_closed)
		{
			_invalidateLocked();
		}
	}

	bool Channel::isAdmissionCurrent(std::uint64_t registrationOrder, std::uint64_t admission) const
	{
		std::lock_guard lock(_mutex);
		if (_closed || !_published || _publishedGeneration != _generation)
		{
			return false;
		}
		const auto matches = [=](const Selection::Entry &entry) {
			return entry.registrationOrder == registrationOrder && entry.admission == admission;
		};
		return std::ranges::any_of(_published->local, matches) ||
			   std::ranges::any_of(_published->directional, matches);
	}

	bool Channel::isClosed() const
	{
		std::lock_guard lock(_mutex);
		return _closed;
	}

	void Channel::close() noexcept
	{
		{
			std::lock_guard lock(_mutex);
			_closed = true;
		}
		_ready.notify_all();
	}
}
