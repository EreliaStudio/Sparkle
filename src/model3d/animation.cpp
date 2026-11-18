#include "model3d/animation.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <stdexcept>
#include <utility>

namespace spk::Model3D
{
	Animation::Animation(float duration) :
		_duration(duration)
	{
		if (!std::isfinite(duration) || duration <= 0.0f)
		{
			throw std::invalid_argument("Animation duration must be finite and strictly positive");
		}
	}

	float Animation::duration() const noexcept
	{
		return _duration;
	}

	const std::vector<Animation::Step> &Animation::steps() const noexcept
	{
		return _steps;
	}

	Animation &Animation::_reserve(Target target)
	{
		if (target.elementName.empty())
		{
			throw std::invalid_argument("Animation target requires an element name");
		}
		const auto existing = std::ranges::find_if(_channels, [&target](const Channel &channel) {
			return channel.target == target;
		});
		if (existing == _channels.end())
		{
			_channels.push_back({std::move(target), {}});
		}
		return *this;
	}

	Animation &Animation::reserveArticulation(std::string elementName, std::string attachmentName)
	{
		if (attachmentName.empty())
		{
			throw std::invalid_argument("Animation articulation requires an attachment name");
		}
		return _reserve({std::move(elementName), std::move(attachmentName)});
	}

	Animation &Animation::reserveBody(std::string elementName)
	{
		return _reserve({std::move(elementName), {}});
	}

	Animation &Animation::addStep(Step step)
	{
		if (!std::isfinite(step.time) || step.time < 0.0f || step.time > _duration ||
			(!_steps.empty() && step.time <= _steps.back().time))
		{
			throw std::invalid_argument("Animation steps must be finite, strictly increasing and within the duration");
		}

		std::vector<Target> seen;
		seen.reserve(step.articulations.size() + step.bodies.size());
		auto validateTarget = [&seen](const Target &target) {
			if (target.elementName.empty() ||
				std::ranges::find(seen, target) != seen.end())
			{
				throw std::invalid_argument("Animation step has an empty element name or a duplicate target");
			}
			seen.push_back(target);
		};
		for (Step::Articulation &joint : step.articulations)
		{
			if (joint.attachmentName.empty())
			{
				throw std::invalid_argument("Animation articulation requires an attachment name");
			}
			validateTarget({joint.elementName, joint.attachmentName});
			const float lengthSquared = joint.rotation.dot(joint.rotation);
			if (!std::isfinite(lengthSquared) || lengthSquared <= 0.0f)
			{
				throw std::invalid_argument("Animation articulation requires a finite non-zero quaternion");
			}
			joint.rotation = joint.rotation.normalized();
		}
		for (const Step::Body &body : step.bodies)
		{
			validateTarget({body.elementName, {}});
			if (!std::isfinite(body.scale.x) || !std::isfinite(body.scale.y) || !std::isfinite(body.scale.z) ||
				body.scale.x <= 0.0f || body.scale.y <= 0.0f || body.scale.z <= 0.0f)
			{
				throw std::invalid_argument("Animation body scale must be finite and strictly positive");
			}
		}

		// Compile the chronological step into private channels, without modifying
		// the existing animation until all validation and allocations succeed.
		auto channels = _channels;
		auto insert = [&channels, time = step.time](Target target, Quaternion rotation, Vector3 scale) {
			auto it = std::ranges::find_if(channels, [&target](const Channel &channel) {
				return channel.target == target;
			});
			if (it == channels.end())
			{
				channels.push_back({std::move(target), {}});
				it = std::prev(channels.end());
			}
			it->keyframes.push_back({time, rotation, scale});
		};
		for (const Step::Articulation &joint : step.articulations)
		{
			insert({joint.elementName, joint.attachmentName}, joint.rotation, {1.0f, 1.0f, 1.0f});
		}
		for (const Step::Body &body : step.bodies)
		{
			insert({body.elementName, {}}, Quaternion::identity(), body.scale);
		}

		_steps.push_back(std::move(step));
		_channels = std::move(channels);
		return *this;
	}
}
