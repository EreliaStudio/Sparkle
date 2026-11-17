#include "light/channel.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <ranges>
#include <utility>

#include "exception.hpp"
#include "rendering/engine/engine.hpp"

namespace spk::Light
{
namespace
	{
		using Entry = Selection::Entry;
		using Status = Entry::PointShadowStatus;

		void selectPointShadows(
			std::vector<Entry> &local,
			const std::vector<std::uint64_t> &history,
			std::size_t target,
			std::size_t maximum,
			float tolerance,
			float hysteresis)
		{
			std::vector<Entry *> candidates;
			for (Entry &entry : local)
			{
				if (entry.type != Selection::Type::Point)
				{
					continue;
				}
				entry.pointShadowStatus = entry.castsShadows ? Status::AtCapacity : Status::Disabled;
				if (entry.castsShadows && entry.score > 0.0f)
				{
					candidates.push_back(&entry);
				}
			}
			std::ranges::sort(candidates, [](const Entry *a, const Entry *b) {
				if (a->score != b->score)
				{
					return a->score > b->score;
				}
				return a->registrationOrder < b->registrationOrder;
			});
			if (target == 0 || candidates.empty())
			{
				return;
			}
			const std::size_t required = std::min(target, candidates.size());
			for (std::size_t i = 0; i < required; ++i)
			{
				candidates[i]->pointShadowStatus = Status::Selected;
			}
			if (required < target || maximum <= required)
			{
				return;
			}

			const float threshold = candidates[target - 1]->score * (1.0f - tolerance);
			std::vector<Entry *> extras;
			for (std::size_t i = target; i < candidates.size(); ++i)
			{
				if (candidates[i]->score >= threshold)
				{
					extras.push_back(candidates[i]);
				}
				else
				{
					candidates[i]->pointShadowStatus = Status::OutsideTolerance;
				}
			}
			std::vector<Entry *> retained;
			const std::size_t slots = maximum - required;
			for (Entry *entry : extras)
			{
				if (std::ranges::find(history, entry->registrationOrder) != history.end() &&
					retained.size() < slots)
				{
					retained.push_back(entry);
					entry->pointShadowStatus = Status::Selected;
				}
			}
			for (Entry *candidate : extras)
			{
				if (candidate->pointShadowStatus == Status::Selected)
				{
					continue;
				}
				if (retained.size() < slots)
				{
					retained.push_back(candidate);
					candidate->pointShadowStatus = Status::Selected;
					continue;
				}
				const auto weakest = std::ranges::min_element(retained, [](const Entry *a, const Entry *b) {
					return a->score < b->score || (a->score == b->score && a->registrationOrder > b->registrationOrder);
				});
				if (weakest != retained.end() &&
					static_cast<double>(candidate->score) >
						static_cast<double>((*weakest)->score) * (1.0 + static_cast<double>(hysteresis)))
				{
					(*weakest)->pointShadowStatus = Status::AtCapacity;
					*weakest = candidate;
					candidate->pointShadowStatus = Status::Selected;
				}
				else
				{
					candidate->pointShadowStatus = Status::Hysteresis;
				}
			}
		}
	}

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

	std::size_t Channel::targetShadowCount() const
	{
		std::lock_guard lock(_mutex);
		return _targetShadowCount;
	}

	void Channel::setTargetShadowCount(std::size_t value)
	{
		std::lock_guard lock(_mutex);
		if (_closed || value > _maxShadowCount)
		{
			throw spk::Exception("Light::Channel target shadow count exceeds maximum or Channel is closed");
		}
		if (value != _targetShadowCount)
		{
			_invalidateLocked();
			_targetShadowCount = value;
		}
	}

	std::size_t Channel::maxShadowCount() const
	{
		std::lock_guard lock(_mutex);
		return _maxShadowCount;
	}

	void Channel::setMaxShadowCount(std::size_t value)
	{
		std::lock_guard lock(_mutex);
		if (_closed || value < _targetShadowCount)
		{
			throw spk::Exception("Light::Channel max shadow count is below target or Channel is closed");
		}
		if (value != _maxShadowCount)
		{
			_invalidateLocked();
			_maxShadowCount = value;
		}
	}

	float Channel::shadowPriorityTolerance() const
	{
		std::lock_guard lock(_mutex);
		return _shadowPriorityTolerance;
	}

	void Channel::setShadowPriorityTolerance(float value)
	{
		if (!std::isfinite(value) || value < 0.0f || value > 1.0f)
		{
			throw spk::Exception("Light::Channel shadow tolerance must be finite and in [0,1]");
		}
		std::lock_guard lock(_mutex);
		if (_closed)
		{
			throw spk::Exception("Cannot configure a closed Light::Channel");
		}
		if (value != _shadowPriorityTolerance)
		{
			_invalidateLocked();
			_shadowPriorityTolerance = value;
		}
	}

	float Channel::shadowSelectionHysteresis() const
	{
		std::lock_guard lock(_mutex);
		return _shadowSelectionHysteresis;
	}

	void Channel::setShadowSelectionHysteresis(float value)
	{
		if (!std::isfinite(value) || value < 0.0f)
		{
			throw spk::Exception("Light::Channel shadow hysteresis must be finite and nonnegative");
		}
		std::lock_guard lock(_mutex);
		if (_closed)
		{
			throw spk::Exception("Cannot configure a closed Light::Channel");
		}
		if (value != _shadowSelectionHysteresis)
		{
			_invalidateLocked();
			_shadowSelectionHysteresis = value;
		}
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
				std::vector<std::uint64_t> pointShadowHistory;
				std::size_t targetShadowCount;
				std::size_t maxShadowCount;
				float shadowPriorityTolerance;
				float shadowSelectionHysteresis;
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
					pointShadowHistory = _pointShadowHistory;
					targetShadowCount = _targetShadowCount;
					maxShadowCount = _maxShadowCount;
					shadowPriorityTolerance = _shadowPriorityTolerance;
					shadowSelectionHysteresis = _shadowSelectionHysteresis;
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
				selectPointShadows(
					fresh->local,
					pointShadowHistory,
					targetShadowCount,
					maxShadowCount,
					shadowPriorityTolerance,
					shadowSelectionHysteresis);

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
								if (it != old.end() &&
									(entry.type != Selection::Type::Point ||
									 (entry.pointShadowStatus == Selection::Entry::PointShadowStatus::Selected &&
									  it->pointShadowStatus == Selection::Entry::PointShadowStatus::Selected) ||
									 (entry.pointShadowStatus != Selection::Entry::PointShadowStatus::Selected &&
									  it->pointShadowStatus != Selection::Entry::PointShadowStatus::Selected)))
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
					std::vector<std::uint64_t> nextPointShadowHistory;
					for (const auto &entry : fresh->local)
					{
						if (entry.type == Selection::Type::Point &&
							entry.pointShadowStatus == Selection::Entry::PointShadowStatus::Selected)
						{
							nextPointShadowHistory.push_back(entry.registrationOrder);
						}
					}
					_pointShadowHistory = std::move(nextPointShadowHistory);
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
		} catch (...)
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

	bool Channel::_isShadowAdmissionCurrentLocked(std::uint64_t registrationOrder, std::uint64_t admission) const
	{
		if (_closed || !_published || _publishedGeneration != _generation)
		{
			return false;
		}
		const auto matches = [=](const Selection::Entry &entry) {
			return entry.registrationOrder == registrationOrder && entry.admission == admission &&
				   entry.castsShadows &&
				   (entry.type != Selection::Type::Point ||
					entry.pointShadowStatus == Selection::Entry::PointShadowStatus::Selected);
		};
		return std::ranges::any_of(_published->local, matches) ||
			   std::ranges::any_of(_published->directional, matches);
	}

	const ShadowMap::ResultCollection &Channel::shadowResultCollection() const noexcept
	{
		return _shadowResultCollection;
	}

	bool Channel::publishShadowResult(Window::Surface &surface, ShadowMap::Result::Entry result)
	{
		if (result.registrationOrder == 0 || result.admission == 0 ||
			result.light.registrationOrder != result.registrationOrder ||
			result.light.admission != result.admission || !result.light.castsShadows)
		{
			return false;
		}

		{
			std::lock_guard lock(_mutex);
			if (!_isShadowAdmissionCurrentLocked(result.registrationOrder, result.admission))
			{
				return false;
			}
		}

		// This GPU stream alone produces this Surface's slot. All allocations and
		// the copy/merge take place outside the Channel and dictionary locks.
		const auto entry = _shadowResultCollection._getOrCreate(surface);
		if (entry == nullptr)
		{
			return false;
		}
		const auto old = entry->slot.acquireLatest();
		ShadowMap::Result merged = old != nullptr ? *old : ShadowMap::Result{};
		const std::uint64_t registrationOrder = result.registrationOrder;
		const std::uint64_t admission = result.admission;
		const std::uint64_t generation = result.generation;
		auto previous = merged.entries.find(registrationOrder);
		if (previous != merged.entries.end() &&
			previous->second.admission == admission &&
			(previous->second.generation >= generation ||
			 previous->second.captureOrdinal > result.captureOrdinal))
		{
			return false;
		}
		merged.entries.insert_or_assign(registrationOrder, std::move(result));

		// The short Channel lock makes final admission/revocation and publication
		// indivisible. No GL operation or dictionary lock is taken here.
		std::lock_guard lock(_mutex);
		if (!_isShadowAdmissionCurrentLocked(registrationOrder, admission) ||
			entry->closed.load(std::memory_order_acquire) ||
			surface.lifeCycle() != Window::LifeCycle::Ready)
		{
			return false;
		}
		std::erase_if(merged.entries, [this](const auto &item) {
			return !_isShadowAdmissionCurrentLocked(item.second.registrationOrder, item.second.admission);
		});
		entry->slot.publish(std::move(merged));
		return true;
	}

	std::optional<ShadowMap::Result::Entry> Channel::resolveShadowResult(
		const Window::Surface &surface, const Selection::Entry &capturedLight, std::uint64_t captureOrdinal) const
	{
		if (!capturedLight.castsShadows ||
			(capturedLight.type == Selection::Type::Point &&
			 capturedLight.pointShadowStatus != Selection::Entry::PointShadowStatus::Selected))
		{
			return std::nullopt;
		}
		const auto published = _shadowResultCollection.acquireLatest(surface);
		if (published == nullptr)
		{
			return std::nullopt;
		}
		const auto it = published->entries.find(capturedLight.registrationOrder);
		if (it == published->entries.end())
		{
			return std::nullopt;
		}
		const ShadowMap::Result::Entry &result = it->second;
		if (result.admission != capturedLight.admission ||
			result.captureOrdinal > captureOrdinal ||
			result.light.type != capturedLight.type ||
			!result.depthTexture.valid())
		{
			return std::nullopt;
		}
		std::lock_guard lock(_mutex);
		return _isShadowAdmissionCurrentLocked(result.registrationOrder, result.admission)
				   ? std::optional<ShadowMap::Result::Entry>{result}
				   : std::nullopt;
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
