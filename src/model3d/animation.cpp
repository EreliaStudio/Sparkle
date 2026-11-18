#include "model3d/animation.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <stdexcept>
#include <utility>

namespace spk::Model3D
{
	float Animation::duration() const noexcept
	{
		return _duration;
	}

	const std::vector<Animation::Step> &Animation::steps() const noexcept
	{
		return _steps;
	}

	Animation::Builder::Builder() :
		_animation(Animation())
	{
	}

	Animation &Animation::Builder::_active()
	{
		if (!_animation)
		{
			throw std::logic_error("Animation::Builder has already been built");
		}
		return *_animation;
	}

	Animation::Builder &Animation::Builder::_reserve(Animation::Target target)
	{
		Animation &animation = _active();
		if (target.elementName.empty())
		{
			throw std::invalid_argument("Animation target requires an element name");
		}
		const auto existing = std::ranges::find_if(animation._channels, [&target](const Channel &channel) {
			return channel.target == target;
		});
		if (existing == animation._channels.end())
		{
			animation._channels.push_back({std::move(target), {}});
		}
		return *this;
	}

	Animation::Builder &Animation::Builder::reserveArticulation(std::string elementName, std::string attachmentName)
	{
		if (attachmentName.empty())
		{
			throw std::invalid_argument("Animation articulation requires an attachment name");
		}
		return _reserve({std::move(elementName), std::move(attachmentName)});
	}

	Animation::Builder &Animation::Builder::reserveBody(std::string elementName)
	{
		return _reserve({std::move(elementName), {}});
	}

	Animation::Builder &Animation::Builder::addStep(Step step)
	{
		Animation &animation = _active();
		if (!std::isfinite(step.time) || step.time < 0.0f ||
			(!animation._steps.empty() && step.time <= animation._steps.back().time))
		{
			throw std::invalid_argument("Animation step times must be finite, nonnegative and strictly increasing");
		}


		const auto validateInterpolation = [](Interpolation curve) {
			switch (curve)
			{
			case Interpolation::Linear:
			case Interpolation::Quadratic:
			case Interpolation::Cubic:
			case Interpolation::Logarithmic:
				return;
			}
			throw std::invalid_argument("Animation interpolation mode is not supported");
		};

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
			validateInterpolation(joint.interpolation);
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
			if (!std::isfinite(body.translation.x) || !std::isfinite(body.translation.y) || !std::isfinite(body.translation.z))
			{
				throw std::invalid_argument("Animation body translation must be finite");
			}
			validateInterpolation(body.interpolation);
		}

		// Compile the chronological step into private channels, without modifying
		// the existing animation until all validation and allocations succeed.
		auto channels = animation._channels;
		auto insert = [&channels, time = step.time](Target target, Quaternion rotation, Vector3 scale, Vector3 translation, Interpolation interpolation) {
			auto it = std::ranges::find_if(channels, [&target](const Channel &channel) {
				return channel.target == target;
			});
			if (it == channels.end())
			{
				channels.push_back({std::move(target), {}});
				it = std::prev(channels.end());
			}
			it->keyframes.push_back({time, rotation, scale, translation, interpolation});
		};
		for (const Step::Articulation &joint : step.articulations)
		{
			insert({joint.elementName, joint.attachmentName}, joint.rotation, {1.0f, 1.0f, 1.0f}, {}, joint.interpolation);
		}
		for (const Step::Body &body : step.bodies)
		{
			insert({body.elementName, {}}, Quaternion::identity(), body.scale, body.translation, body.interpolation);
		}

		animation._steps.push_back(std::move(step));
		animation._channels = std::move(channels);
		animation._duration = animation._steps.back().time;
		return *this;
	}

	Animation Animation::Builder::build()
	{
		Animation &animation = _active();
		if (animation._steps.empty() || animation._duration <= 0.0f)
		{
			throw std::invalid_argument("Animation requires a final step at a strictly positive time");
		}
		Animation result = std::move(animation);
		_animation.reset();
		return result;
	}
}
