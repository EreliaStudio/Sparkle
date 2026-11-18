#include "model3d/animator.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>

#include "exception.hpp"

namespace spk::Model3D
{
	Animator::Animator(ObjectHeadless &object) noexcept :
		_object(object)
	{
	}

	Animator::~Animator()
	{
		stopAll();
	}

	Animation::Keyframe Animator::_sample(
		const Animation::Channel &channel,
		const StartPose &initial,
		float time)
	{
		const auto &keys = channel.keyframes;
		if (time <= keys.front().time)
		{
			if (keys.front().time == 0.0f)
			{
				return keys.front();
			}
			// A target introduced later starts from the instance's captured pose.
			const float alpha = time / keys.front().time;
			return {
				time,
				Quaternion::slerp(initial.rotation, keys.front().rotation, alpha),
				Vector3::lerp(initial.scale, keys.front().scale, alpha)};
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

	void Animator::_apply(const Running &running, float time)
	{
		const auto &channels = running.animation._channels;
		for (std::size_t index = 0; index < channels.size(); ++index)
		{
			const auto &channel = channels[index];
			if (channel.keyframes.empty())
			{
				continue;
			}
			const auto value = _sample(channel, running.initial[index], time);
			if (channel.target.attachmentName.empty())
			{
				_object.element(channel.target.elementName).setScale(value.scale);
			}
			else
			{
				_object.attachment(channel.target.elementName, channel.target.attachmentName).setRotation(value.rotation);
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
				_object.attachment(pose.target.elementName, pose.target.attachmentName).setRotation(pose.rotation);
			}
		}
	}

	Animator::Handle Animator::play(const Animation &animation, bool loop)
	{
		if (animation._channels.empty())
		{
			throw std::invalid_argument("Animator cannot play an animation without any targets");
		}

		// Reject conflicts before changing the pose or registering the animation.
		for (const auto &channel : animation._channels)
		{
			for (const auto &active : _running)
			{
				for (const auto &held : active.animation._channels)
				{
					if (channel.target == held.target)
					{
						const bool body = channel.target.attachmentName.empty();
						const std::string target = channel.target.elementName +
							(body ? "" : "/" + channel.target.attachmentName);
						throw spk::Exception(
							"Animation conflict: " + std::string(body ? "body " : "articulation ") +
							target + " is already controlled by animation #" + std::to_string(active.handle));
					}
				}
			}
		}

		std::vector<StartPose> initial;
		initial.reserve(animation._channels.size());
		for (const auto &channel : animation._channels)
		{
			StartPose pose{channel.target, Quaternion::identity(), {1.0f, 1.0f, 1.0f}};
			// Validate every target before touching any pose.
			if (channel.target.attachmentName.empty())
			{
				pose.scale = _object.element(channel.target.elementName).scale();
			}
			else
			{
				const auto &joint = _object.attachment(channel.target.elementName, channel.target.attachmentName);
				pose.rotation = joint.rotation();
			}
			initial.push_back(std::move(pose));
		}

		const Handle handle = _nextHandle;
		_running.push_back({handle, animation, 0.0, loop, std::move(initial)});
		++_nextHandle;
		_apply(_running.back(), 0.0f);
		return handle;
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
			// Clip timestamps use float precision; compare at that precision so
			// 0.9f + 0.1f completes exactly like a single 1.0f update.
			if (!active.loop && static_cast<float>(elapsed) >= active.animation.duration())
			{
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
