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
		};

		struct Channel
		{
			Target target;
			std::vector<Keyframe> keyframes;
		};

		float _duration;
		std::vector<Step> _steps;
		std::vector<Channel> _channels;

		explicit Animation(float duration);

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
		explicit Builder(float duration);

		Builder(const Builder &) = delete;
		Builder &operator=(const Builder &) = delete;
		Builder(Builder &&) noexcept = default;
		Builder &operator=(Builder &&) noexcept = default;

		// Steps are ordered and each can animate multiple independent targets.
		Builder &addStep(Step step);

		// Reserve a target without providing keyframes for it.
		Builder &reserveArticulation(std::string elementName, std::string attachmentName);
		Builder &reserveBody(std::string elementName);

		// Finishes the builder; subsequent operations are rejected.
		[[nodiscard]] Animation build();
	};
}
