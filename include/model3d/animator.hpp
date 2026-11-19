#pragma once

#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "model3d/animation.hpp"
#include "model3d/object_headless.hpp"

namespace spk::Model3D
{
	// An Animator owns its running clips, but borrows its ObjectHeadless.
	// The object must outlive the Animator and must not be moved or reassigned
	// while any clip is running. Use one Animator per ObjectHeadless instance.
	class Animator final
	{
	public:
		class Builder;

	private:
		struct StartPose
		{
			Animation::Target target;
			Quaternion rotation;
			Vector3 scale;
			Vector3 translation;
		};

		struct Running
		{
			std::string name;
			double time;
			std::vector<StartPose> initial;
		};

		ObjectHeadless *_object = nullptr;
		struct Entry { Animation animation; std::optional<std::string> exit; };
		std::map<std::string, Entry> _animations;
		std::optional<std::string> _default;
		std::vector<Running> _running;
		void _start(const std::string &name);
		void _resumeDefault();

		[[nodiscard]] static Animation::Keyframe _sample(
			const Animation::Channel &channel,
			const StartPose &initial,
			float time);
		void _apply(const Running &running, float time);

	public:
		Animator() = default;
		void bind(ObjectHeadless &object);
		void unbind();
		[[nodiscard]] bool bound() const noexcept;
		~Animator();

		Animator(const Animator &) = delete;
		Animator &operator=(const Animator &) = delete;
		Animator(Animator &&) = default;
		Animator &operator=(Animator &&) = default;

		void play(const std::string &name);
		void update(float deltaSeconds);
		[[nodiscard]] bool stop(const std::string &name);
		void stopAll();
		[[nodiscard]] bool playing(const std::string &name) const noexcept;
		[[nodiscard]] std::size_t activeCount() const noexcept;
	};
	class Animator::Builder
	{
		std::map<std::string, Entry> _animations;
		std::optional<std::string> _default;
	public:
		Builder &addAnimation(std::string name, Animation animation);
		Builder &setDefaultAnimation(std::optional<std::string> name);
		Builder &setExitAnimation(const std::string &name, std::string exitName);
		[[nodiscard]] Animator build();
	};
}
