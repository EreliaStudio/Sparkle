#pragma once

#include <compare>
#include <string>
#include <vector>

#include "math/quaternion.hpp"
#include "math/vector3.hpp"

namespace spk::Model3D
{
	// A reusable, object-independent animation defined chronologically.
	// A Step can move any number of joints and scale any number of elements.
	class Animation
	{
	public:
		struct Step
		{
			struct Articulation
			{
				std::string elementName;
				std::string attachmentName;
				Quaternion rotation{};
			};

			struct Body
			{
				std::string elementName;
				Vector3 scale{1.0f, 1.0f, 1.0f};
			};

			float time = 0.0f;
			std::vector<Articulation> articulations;
			std::vector<Body> bodies;
		};

	private:
		friend class Animator;

		// Channels are an implementation detail: callers only define Steps.
		// Empty channels reserve a target for the full animation duration.
		struct Target
		{
			std::string elementName;
			std::string attachmentName;

			auto operator<=>(const Target &) const = default;
		};

		struct Keyframe
		{
			float time;
			Quaternion rotation;
			Vector3 scale;
		};

		struct Channel
		{
			Target target;
			std::vector<Keyframe> keyframes;
		};

		float _duration;
		std::vector<Step> _steps;
		std::vector<Channel> _channels;

		Animation &_reserve(Target target);

	public:
		explicit Animation(float duration);

		[[nodiscard]] float duration() const noexcept;
		[[nodiscard]] const std::vector<Step> &steps() const noexcept;

		// Steps must be in strictly increasing time order, within [0, duration].
		// A target may be omitted from intermediate steps; its values are
		// interpolated between its own neighboring keyframes.
		Animation &addStep(Step step);

		// Reserve a target even when the animation has no keyframes for it.
		Animation &reserveArticulation(std::string elementName, std::string attachmentName);
		Animation &reserveBody(std::string elementName);
	};
}
