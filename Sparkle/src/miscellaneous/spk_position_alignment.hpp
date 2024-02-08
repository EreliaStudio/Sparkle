#pragma once

namespace spk
{
	/**
	 * @enum VerticalAlignment
	 * @brief Specifies the vertical alignment of text within its bounding box.
	 * 
	 * This enumeration controls how text is vertically aligned within its bounding box when rendered. It allows for precise control over the vertical positioning of text, improving layout options in graphical applications.
	 *
	 * - Top: Aligns text to the top of its bounding box.
	 * - Centered: Centers text vertically within its bounding box.
	 * - Down: Aligns text to the bottom of its bounding box.
	 */
	enum class VerticalAlignment
	{
		Top,
		Centered,
		Down
	};

	/**
	 * @enum HorizontalAlignment
	 * @brief Specifies the horizontal alignment of text within its bounding box.
	 * 
	 * This enumeration controls how text is horizontally aligned within its bounding box when rendered. It provides a means to align text to the left, center, or right of its bounding box, which is essential for text layout and design in graphical user interfaces and other applications.
	 *
	 * - Left: Aligns text to the left of its bounding box.
	 * - Centered: Centers text horizontally within its bounding box.
	 * - Right: Aligns text to the right of its bounding box.
	 */
	enum class HorizontalAlignment
	{
		Left,
		Centered,
		Right
	};
}