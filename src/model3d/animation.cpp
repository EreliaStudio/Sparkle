#include "model3d/animation.hpp"

#include <algorithm>
#include <cmath>
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

	const std::vector<Animation::Track> &Animation::tracks() const noexcept
	{
		return _tracks;
	}

	Animation &Animation::addTrack(Target target, std::vector<Keyframe> keyframes)
	{
		if (target.elementName.empty())
		{
			throw std::invalid_argument("Animation target requires an element name");
		}
		const auto duplicate = std::ranges::find_if(_tracks, [&target](const Track &track) {
			return track.target == target;
		});
		if (duplicate != _tracks.end())
		{
			throw std::invalid_argument("Animation target is already present: " + target.elementName + "/" + target.attachmentName);
		}

		float previous = -1.0f;
		for (Keyframe &keyframe : keyframes)
		{
			if (!std::isfinite(keyframe.time) || keyframe.time < 0.0f ||
				keyframe.time > _duration || keyframe.time <= previous)
			{
				throw std::invalid_argument("Animation keyframe times must be finite, ordered and within the duration");
			}
			previous = keyframe.time;
			const Quaternion &q = keyframe.rotation;
			const float lengthSquared = q.dot(q);
			if (!std::isfinite(lengthSquared) || lengthSquared <= 0.0f)
			{
				throw std::invalid_argument("Animation keyframe rotation must be a finite non-zero quaternion");
			}
			keyframe.rotation = q.normalized();
			const Vector3 &scale = keyframe.scale;
			if (!std::isfinite(scale.x) || !std::isfinite(scale.y) || !std::isfinite(scale.z) ||
				scale.x <= 0.0f || scale.y <= 0.0f || scale.z <= 0.0f)
			{
				throw std::invalid_argument("Animation keyframe scale must be finite and strictly positive");
			}
			if (target.attachmentName.empty() && keyframe.rotation != Quaternion::identity())
			{
				throw std::invalid_argument("Element scale-only animation tracks cannot specify joint rotation");
			}
		}
		_tracks.push_back({std::move(target), std::move(keyframes)});
		return *this;
	}
}
