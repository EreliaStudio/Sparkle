#pragma once

#include <compare>
#include <string>
#include <vector>

#include "math/quaternion.hpp"
#include "math/vector3.hpp"

namespace spk::Model3D
{
	// A reusable, object-independent clip. An empty attachmentName targets the
	// element's own scale (including the root); other targets affect a joint's
	// rotation and scale. Even an empty track reserves its target during playback.
	class Animation
	{
	public:
		struct Target
		{
			std::string elementName;
			std::string attachmentName;

			auto operator<=>(const Target &) const = default;
		};

		struct Keyframe
		{
			float time = 0.0f;
			Quaternion rotation{};
			Vector3 scale{1.0f, 1.0f, 1.0f};
		};

		struct Track
		{
			Target target;
			std::vector<Keyframe> keyframes;
		};

	private:
		float _duration;
		std::vector<Track> _tracks;

	public:
		explicit Animation(float duration);

		[[nodiscard]] float duration() const noexcept;
		[[nodiscard]] const std::vector<Track> &tracks() const noexcept;

		// Timestamps must be finite, strictly increasing and in [0, duration].
		// Rotation is a unit quaternion after insertion. Scale is positive.
		Animation &addTrack(Target target, std::vector<Keyframe> keyframes);
	};
}
