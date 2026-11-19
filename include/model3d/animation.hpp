#pragma once

#include <compare>
#include <optional>
#include <memory>
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

			struct Translation
			{
				std::string elementName;
				Vector3 translation{};
				Interpolation interpolation = Interpolation::Linear;
			};
			struct Scaling
			{
				std::string elementName;
				Vector3 scale{1.0f, 1.0f, 1.0f};
				Interpolation interpolation = Interpolation::Linear;
			};

			float time = 0.0f;
			std::vector<Articulation> articulations;
			std::vector<Translation> translations;
			std::vector<Scaling> scalings;
		};

	private:
		friend class Animator;

		// A channel is an internal detail; clients only see Step.
		// An empty channel reserves its target for the whole animation.
		struct Target
		{
			std::string elementName;
			std::string attachmentName;
			enum class Property
			{
				Rotation,
				Translation,
				Scaling
			} property = Property::Rotation;

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

		struct Data { float duration = 0.0f; std::vector<Step> steps; std::vector<Channel> channels; };
		std::shared_ptr<const Data> _data;

		Animation() = default;

	public:
		[[nodiscard]] float duration() const noexcept;
		[[nodiscard]] const std::vector<Step> &steps() const noexcept;
	};

	// Follows Blueprint::Builder: definitions are mutable only while building.
	class Animation::Builder
	{
	private:
		std::optional<Animation::Data> _animation;

		[[nodiscard]] Animation::Data &_active();
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
		Builder &reserveTranslation(std::string elementName);
		Builder &reserveScaling(std::string elementName);

		// Finishes the builder; requires a final step after t=0.
		// For a mask-only animation, use an empty step at its end time.
		// Subsequent operations are rejected.
		[[nodiscard]] Animation build();
	};
}
