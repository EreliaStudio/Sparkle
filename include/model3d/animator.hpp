#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
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

		struct Conflict
		{
			Animation::Target target;
			Handle owner;
		};

		struct PlayResult
		{
			bool started = false;
			Handle handle = 0;
			std::optional<Conflict> conflict;
		};

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

		void _apply(const Running &running, float time);
		void _restore(const Running &running);

	public:
		explicit Animator(ObjectHeadless &object) noexcept;
		~Animator();

		Animator(const Animator &) = delete;
		Animator &operator=(const Animator &) = delete;
		Animator(Animator &&) = delete;
		Animator &operator=(Animator &&) = delete;

		// Conflict rejection does not mutate pose or active clips.
		// Clip definitions are copied and can safely outlive their caller.
		[[nodiscard]] PlayResult play(Animation animation, bool loop = false);
		void update(float deltaSeconds);
		[[nodiscard]] bool stop(Handle handle);
		void stopAll();
		[[nodiscard]] bool playing(Handle handle) const noexcept;
		[[nodiscard]] std::size_t activeCount() const noexcept;
	};
}
