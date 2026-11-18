#pragma once

#include <compare>
#include <optional>
#include <string>
#include <vector>

#include "math/quaternion.hpp"
#include "math/vector3.hpp"

namespace spk::Model3D
{
	// An object-independent, reusable animation. Construct with Builder,
	// which validates and compiles chronological steps into private channels.
	class Animation
	{
	public:
		class Builder;

		// Easing of the normalized interpolation parameter (0..1), not
		// interpolation of matrix entries. Cubic/Quadratic are ease-in curves.
		enum class Interpolation
		{
			Linear,
			Quadratic,
			Cubic,
			Logarithmic
		};

		struct Step
		{
			struct Articulation
			{
				std::string elementName;
				std::string attachmentName;
				Quaternion rotation{};
				Interpolation interpolation = Interpolation::Linear;
			};

			struct Body
			{
				std::string elementName;
				Vector3 scale{1.0f, 1.0f, 1.0f};
				Vector3 translation{};
				Interpolation interpolation = Interpolation::Linear;
			};

			float time = 0.0f;
			std::vector<Articulation> articulations;
			std::vector<Body> bodies;
		};

	private:
		friend class Animator;

		// A channel is an internal detail; clients only see Step.
		// An empty channel reserves its target for the whole animation.
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
			Vector3 translation;
			Interpolation interpolation;
		};

		struct Channel
		{
			Target target;
			std::vector<Keyframe> keyframes;
		};

		float _duration = 0.0f;
		std::vector<Step> _steps;
		std::vector<Channel> _channels;

		Animation() = default;

	public:
		[[nodiscard]] float duration() const noexcept;
		[[nodiscard]] const std::vector<Step> &steps() const noexcept;
	};

	// Follows Blueprint::Builder: definitions are mutable only while building.
	class Animation::Builder
	{
	private:
		std::optional<Animation> _animation;

		[[nodiscard]] Animation &_active();
		Builder &_reserve(Animation::Target target);

	public:
		Builder();

		Builder(const Builder &) = delete;
		Builder &operator=(const Builder &) = delete;
		Builder(Builder &&) noexcept = default;
		Builder &operator=(Builder &&) noexcept = default;

		// Steps are ordered and each can animate multiple independent targets.
		// The last step defines the duration, which must be positive.
		Builder &addStep(Step step);

		// Reserve a target without providing keyframes for it.
		Builder &reserveArticulation(std::string elementName, std::string attachmentName);
		Builder &reserveBody(std::string elementName);

		// Finishes the builder; requires a final step after t=0.
		// For a mask-only animation, use an empty step at its end time.
		// Subsequent operations are rejected.
		[[nodiscard]] Animation build();
	};
}
