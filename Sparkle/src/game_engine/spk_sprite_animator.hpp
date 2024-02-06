#pragma once

#include <chrono>
#include <vector>
#include <map>
#include "design_pattern/spk_notifier.hpp"
#include "math/spk_vector2.hpp"

namespace spk
{
	/**
	 * @brief Represents a sprite animation with duration and frame information.
	 * 
	 * This struct defines a sprite animation, including its duration (in frames), the coordinates of the first frame
	 * of the animation in the sprite sheet, frame offset, and the length of the animation.
	 * It is used to specify how a sprite should be animated, providing information about the sequence of frames
	 * and the timing of the animation.
	 * 
	 * It have its equivalent inside Lumina code, named SpriteAnimation, includable by #include <spriteAnimation>.
	 * 
	 * Usage example:
	 * @code
	 * // Create a sprite animation with a duration of 5 frames
	 * spk::SpriteAnimation animation;
	 * animation.duration = 5;
	 * animation.firstFrame = spk::Vector2Int(0, 0); // Coordinates of the first frame of this animation inside the sprite sheet
	 * animation.frameOffset = spk::Vector2Int(32, 0); // Offset to the next frame in the sprite sheet
	 * animation.length = 8; // Number of frames in the animation
	 * @endcode
	 * 
	 * @see SpriteRenderer
	 */
	struct SpriteAnimation
	{
		int duration;
		spk::Vector2Int firstFrame;
		spk::Vector2Int frameOffset;
		int lenght;
	};
}