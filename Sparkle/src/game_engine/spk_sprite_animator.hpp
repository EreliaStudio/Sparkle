#pragma once

#include <chrono>
#include <vector>
#include <map>
#include "design_pattern/spk_notifier.hpp"
#include "math/spk_vector2.hpp"

namespace spk
{
	/**
	 * @class SpriteAnimation
	 * @brief Represents a sprite animation with duration and frame information.
	 * 
	 * This struct defines a sprite animation, including its duration (in frames), the coordinates of the first frame
	 * of the animation in the sprite sheet, frame offset, and the length of the animation.
	 * It is used to specify how a sprite should be animated, providing information about the sequence of frames
	 * and the timing of the animation.
	 * 
	 * It have its equivalent inside Lumina code, named SpriteAnimation, includable by including <spriteAnimation>.
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
		/**
		 * @brief The duration of each frame in the animation.
		 * 
		 * Specifies how long (in frames) each frame of the animation should be displayed before moving to the next one.
		 * This allows for control over the speed of the animation.
		 */
		int duration;

		/**
		 * @brief The coordinates of the first frame of the animation in the sprite sheet.
		 * 
		 * Defines the starting point of the animation within the sprite sheet. This vector specifies the row and column
		 * of the first frame's top-left corner, allowing the animation to begin at any point within the sprite sheet.
		 */
		spk::Vector2Int firstFrame;

		/**
		 * @brief The offset to the next frame in the sprite sheet.
		 * 
		 * Specifies the distance (in pixels) to move from one frame to the next within the sprite sheet. This can be
		 * used to navigate frames that are arranged horizontally or vertically (or both) in the sprite sheet.
		 */
		spk::Vector2Int frameOffset;

		/**
		 * @brief The total number of frames in the animation.
		 * 
		 * Indicates how many frames are included in the animation sequence. This determines when the animation should loop
		 * or end, based on the number of frames that have been displayed.
		 */
		int lenght;
	};
}