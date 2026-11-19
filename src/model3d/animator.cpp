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
	namespace
	{
		// The easing curve changes time, not the quaternion or matrix math.
		// All curves map [0,1] into [0,1] and preserve both endpoints.
		[[nodiscard]] float easedTime(float alpha, Animation::Interpolation interpolation)
		{
			const float t = std::clamp(alpha, 0.0f, 1.0f);
			switch (interpolation)
			{
			case Animation::Interpolation::Linear:
				return t;
			case Animation::Interpolation::Quadratic:
				return t * t;
			case Animation::Interpolation::Cubic:
				return t * t * t;
			case Animation::Interpolation::Logarithmic:
				return std::log1p(9.0f * t) / std::log1p(9.0f);
			}
			throw std::invalid_argument("Invalid animation interpolation mode");
		}
	}

	void Animator::bind(ObjectHeadless &object)
	{
		if (_object != nullptr)
		{
			throw spk::Exception("Animator is already bound.");
		}
		_object = &object;
		_resumeDefault();
	}

	void Animator::unbind()
	{
		stopAll();
		_object = nullptr;
	}

	bool Animator::bound() const noexcept
	{
		return _object != nullptr;
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
			// A target introduced later starts from the current pose captured at play().
			const float alpha = easedTime(time / keys.front().time, keys.front().interpolation);
			return {
				time,
				Quaternion::slerp(initial.rotation, keys.front().rotation, alpha),
				Vector3::lerp(initial.scale, keys.front().scale, alpha),
				Vector3::lerp(initial.translation, keys.front().translation, alpha),
				keys.front().interpolation};
		}
		if (time >= keys.back().time)
		{
			return keys.back();
		}
		const auto upper = std::upper_bound(keys.begin(), keys.end(), time, [](float value, const Animation::Keyframe &key) {
			return value < key.time;
		});
		const auto lower = std::prev(upper);
		const float alpha = easedTime((time - lower->time) / (upper->time - lower->time), upper->interpolation);
		return {
			time,
			Quaternion::slerp(lower->rotation, upper->rotation, alpha),
			Vector3::lerp(lower->scale, upper->scale, alpha),
			Vector3::lerp(lower->translation, upper->translation, alpha),
			upper->interpolation};
	}

	void Animator::_apply(const Running &running, float time)
	{
		const auto &channels = _animations.at(running.name).animation._data->channels;
		for (std::size_t index = 0; index < channels.size(); ++index)
		{
			const auto &channel = channels[index];
			if (channel.keyframes.empty())
			{
				continue;
			}
			const auto value = _sample(channel, running.initial[index], time);
			switch (channel.target.property)
			{
			case Animation::Target::Property::Rotation:
				_object->attachment(channel.target.elementName, channel.target.attachmentName).setRotation(value.rotation);
				break;
			case Animation::Target::Property::Translation:
				_object->element(channel.target.elementName).setTranslation(value.translation);
				break;
			case Animation::Target::Property::Scaling:
				_object->element(channel.target.elementName).setScale(value.scale);
				break;
			}
		}
	}

	Animator::Builder &Animator::Builder::addAnimation(std::string name, Animation animation)
	{
		if (name.empty() || !animation._data)
		{
			throw std::invalid_argument("Invalid animation");
		}
		if (!_animations.emplace(std::move(name), Entry{std::move(animation), std::nullopt}).second)
		{
			throw std::invalid_argument("Duplicate animation name");
		}
		return *this;
	}
	Animator::Builder &Animator::Builder::setDefaultAnimation(std::optional<std::string> name)
	{
		_default = std::move(name);
		return *this;
	}
	Animator::Builder &Animator::Builder::setExitAnimation(const std::string &name, std::string exitName)
	{
		const auto it = _animations.find(name);
		if (it == _animations.end())
		{
			throw std::invalid_argument("Unknown source animation");
		}
		it->second.exit = std::move(exitName);
		return *this;
	}
	Animator Animator::Builder::build()
	{
		if (_default && !_animations.contains(*_default))
		{
			throw std::invalid_argument("Unknown default animation");
		}
		for (const auto &[name, entry] : _animations)
		{
			if (entry.exit && !_animations.contains(*entry.exit))
			{
				throw std::invalid_argument("Unknown exit animation: " + name);
			}
		}
		Animator animator;
		animator._animations = std::move(_animations);
		animator._default = std::move(_default);
		return animator;
	}
	void Animator::_start(const std::string &name)
	{
		if (!_object)
		{
			throw spk::Exception("Animator is not bound.");
		}
		const auto &animation = _animations.at(name).animation;
		const auto &channels = animation._data->channels;
		if (channels.empty())
		{
			throw std::invalid_argument("Animation has no targets");
		}
		for (const auto &active : _running)
		{
			if (active.name != name)
			{
				for (const auto &channel : channels)
				{
					for (const auto &held : _animations.at(active.name).animation._data->channels)
					{
						if (channel.target == held.target)
						{
							throw spk::Exception("Animation conflict: " + name + " / " + active.name);
						}
					}
				}
			}
		}
		std::vector<StartPose> initial;
		initial.reserve(channels.size());
		for (const auto &channel : channels)
		{
			StartPose pose{channel.target, Quaternion::identity(), {1, 1, 1}, {}};
			switch (channel.target.property)
			{
			case Animation::Target::Property::Rotation:
				pose.rotation = _object->attachment(channel.target.elementName, channel.target.attachmentName).rotation();
				break;
			case Animation::Target::Property::Translation:
				pose.translation = _object->element(channel.target.elementName).translation();
				break;
			case Animation::Target::Property::Scaling:
				pose.scale = _object->element(channel.target.elementName).scale();
				break;
			}
			initial.push_back(std::move(pose));
		}
		const auto it = std::ranges::find_if(_running, [&name](const Running &running) {
			return running.name == name;
		});
		if (it != _running.end())
		{
			_running.erase(it);
		}
		_running.push_back({name, 0.0, std::move(initial)});
		_apply(_running.back(), 0.0f);
	}
	void Animator::_resumeDefault()
	{
		if (_running.empty() && _default)
		{
			_start(*_default);
		}
	}
	void Animator::play(const std::string &name)
	{
		if (!_animations.contains(name))
		{
			throw std::invalid_argument("Unknown animation: " + name);
		}
		if (_default && name != *_default)
		{
			const auto it = std::ranges::find_if(_running, [this](const Running &r) {
				return r.name == *_default;
			});
			if (it != _running.end())
			{
				_running.erase(it);
			}
		}
		_start(name);
	}
	void Animator::update(float deltaSeconds)
	{
		if (!std::isfinite(deltaSeconds) || deltaSeconds < 0.0f)
		{
			throw std::invalid_argument("Animator delta must be finite and nonnegative");
		}
		if (!_object)
		{
			return;
		}
		_resumeDefault();
		std::vector<std::string> finished;
		for (auto &active : _running)
		{
			const auto &animation = _animations.at(active.name).animation;
			const double elapsed = active.time + static_cast<double>(deltaSeconds);
			const bool looping = _default && active.name == *_default;
			if (!looping && static_cast<float>(elapsed) >= animation.duration())
			{
				_apply(active, animation.duration());
				finished.push_back(active.name);
			}
			else
			{
				active.time = looping ? std::fmod(elapsed, static_cast<double>(animation.duration())) : elapsed;
				_apply(active, static_cast<float>(active.time));
			}
		}
		for (const auto &name : finished)
		{
			const auto it = std::ranges::find_if(_running, [&name](const Running &r) {
				return r.name == name;
			});
			if (it != _running.end())
			{
				_running.erase(it);
			}
			const auto &exit = _animations.at(name).exit;
			if (exit)
			{
				_start(*exit);
			}
		}
		_resumeDefault();
	}
	bool Animator::stop(const std::string &name)
	{
		const auto matchesName = [&name](const Running &running) {
			return running.name == name;
		};
		const auto it = std::ranges::find_if(_running, matchesName);
		if (it == _running.end())
		{
			return false;
		}
		_running.erase(it);
		_resumeDefault();
		return true;
	}
	void Animator::stopAll()
	{
		_running.clear();
	}
	bool Animator::playing(const std::string &name) const noexcept
	{
		const auto matchesName = [&name](const Running &running) {
			return running.name == name;
		};
		return std::ranges::any_of(_running, matchesName);
	}
	std::size_t Animator::activeCount() const noexcept
	{
		return _running.size();
	}
}
