#include "model3d/animator.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace spk::Model3D
{
	namespace
	{
		[[nodiscard]] Animation::Keyframe sample(const Animation::Track &track, float time)
		{
			const auto &keys = track.keyframes;
			if (time <= keys.front().time)
			{
				return keys.front();
			}
			if (time >= keys.back().time)
			{
				return keys.back();
			}
			const auto upper = std::upper_bound(keys.begin(), keys.end(), time, [](float value, const Animation::Keyframe &key) {
				return value < key.time;
			});
			const auto lower = std::prev(upper);
			const float alpha = (time - lower->time) / (upper->time - lower->time);
			return {
				time,
				Quaternion::slerp(lower->rotation, upper->rotation, alpha),
				Vector3::lerp(lower->scale, upper->scale, alpha)};
		}
	}

	Animator::Animator(ObjectHeadless &object) noexcept :
		_object(object)
	{
	}

	Animator::~Animator()
	{
		stopAll();
	}

	void Animator::_apply(const Running &running, float time)
	{
		for (const auto &track : running.animation.tracks())
		{
			if (track.keyframes.empty())
			{
				continue;
			}
			const auto value = sample(track, time);
			if (track.target.attachmentName.empty())
			{
				_object.element(track.target.elementName).setScale(value.scale);
			}
			else
			{
				auto &joint = _object.attachment(track.target.elementName, track.target.attachmentName);
				joint.setRotation(value.rotation);
				joint.setScale(value.scale);
			}
		}
	}

	void Animator::_restore(const Running &running)
	{
		for (const auto &pose : running.initial)
		{
			if (pose.target.attachmentName.empty())
			{
				_object.element(pose.target.elementName).setScale(pose.scale);
			}
			else
			{
				auto &joint = _object.attachment(pose.target.elementName, pose.target.attachmentName);
				joint.setRotation(pose.rotation);
				joint.setScale(pose.scale);
			}
		}
	}

	Animator::PlayResult Animator::play(Animation animation, bool loop)
	{
		if (animation.tracks().empty())
		{
			throw std::invalid_argument("Animator cannot play an animation without any tracks");
		}
		std::vector<StartPose> initial;
		initial.reserve(animation.tracks().size());
		for (const auto &track : animation.tracks())
		{
			StartPose pose{track.target, Quaternion::identity(), {1.0f, 1.0f, 1.0f}};
			// Validate every target before changing any joint or reserving a mask.
			if (track.target.attachmentName.empty())
			{
				pose.scale = _object.element(track.target.elementName).scale();
			}
			else
			{
				const auto &joint = _object.attachment(track.target.elementName, track.target.attachmentName);
				pose.rotation = joint.rotation();
				pose.scale = joint.scale();
			}
			initial.push_back(std::move(pose));
		}
		for (const auto &track : animation.tracks())
		{
			for (const auto &active : _running)
			{
				for (const auto &held : active.animation.tracks())
				{
					if (track.target == held.target)
					{
						return {false, 0, Conflict{track.target, active.handle}};
					}
				}
			}
		}
		const Handle handle = _nextHandle++;
		_running.push_back({handle, std::move(animation), 0.0, loop, std::move(initial)});
		_apply(_running.back(), 0.0f);
		return {true, handle, std::nullopt};
	}

	void Animator::update(float deltaSeconds)
	{
		if (!std::isfinite(deltaSeconds) || deltaSeconds < 0.0f)
		{
			throw std::invalid_argument("Animator delta must be finite and nonnegative");
		}
		for (auto it = _running.begin(); it != _running.end();)
		{
			Running &active = *it;
			const double elapsed = active.time + static_cast<double>(deltaSeconds);
			const double duration = static_cast<double>(active.animation.duration());
			if (!active.loop && elapsed >= duration)
			{
				// A completed one-shot releases its entire mask and restores the
				// pose that was present before play(), including scale.
				_restore(active);
				it = _running.erase(it);
				continue;
			}
			active.time = active.loop ? std::fmod(elapsed, duration) : elapsed;
			_apply(active, static_cast<float>(active.time));
			++it;
		}
	}

	bool Animator::stop(Handle handle)
	{
		const auto it = std::ranges::find_if(_running, [handle](const Running &running) {
			return running.handle == handle;
		});
		if (it == _running.end())
		{
			return false;
		}
		_restore(*it);
		_running.erase(it);
		return true;
	}

	void Animator::stopAll()
	{
		for (const auto &running : _running)
		{
			_restore(running);
		}
		_running.clear();
	}

	bool Animator::playing(Handle handle) const noexcept
	{
		return std::ranges::any_of(_running, [handle](const Running &running) {
			return running.handle == handle;
		});
	}

	std::size_t Animator::activeCount() const noexcept
	{
		return _running.size();
	}
}
