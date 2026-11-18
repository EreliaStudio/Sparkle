#pragma once

#include <cstddef>
#include <cstdint>
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
		using Handle = std::uint64_t;

	private:
		struct StartPose
		{
			Animation::Target target;
			Quaternion rotation;
			Vector3 scale;
		};

		struct Running
		{
			Handle handle;
			Animation animation;
			double time;
			bool loop;
			std::vector<StartPose> initial;
		};

		ObjectHeadless &_object;
		std::vector<Running> _running;
		Handle _nextHandle = 1;

		[[nodiscard]] static Animation::Keyframe _sample(
			const Animation::Channel &channel,
			const StartPose &initial,
			float time);
		void _apply(const Running &running, float time);
		void _restore(const Running &running);

	public:
		explicit Animator(ObjectHeadless &object) noexcept;
		~Animator();

		Animator(const Animator &) = delete;
		Animator &operator=(const Animator &) = delete;
		Animator(Animator &&) = delete;
		Animator &operator=(Animator &&) = delete;

		// Returns a handle on success; throws spk::Exception on a mask conflict,
		// without changing any pose or running clip. Copies the clip definition.
		[[nodiscard]] Handle play(const Animation &animation, bool loop = false);
		void update(float deltaSeconds);
		[[nodiscard]] bool stop(Handle handle);
		void stopAll();
		[[nodiscard]] bool playing(Handle handle) const noexcept;
		[[nodiscard]] std::size_t activeCount() const noexcept;
	};
}
