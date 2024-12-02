#pragma once

#include <iostream>
#include "structure/math/spk_vector2.hpp"

namespace spk
{
	/**
	 * @struct Geometry2D
	 * @brief Represents a 2D geometry defined by an anchor point and size.
	 * 
	 * The `Geometry2D` struct encapsulates a rectangular area in a 2D space, 
	 * providing utilities for initialization, comparison, and point containment checks.
	 * 
	 * Example usage:
	 * @code
	 * spk::Geometry2D rect({0, 0}, {100, 50}); // Anchor at (0, 0), size 100x50
	 * spk::Geometry2D rectAlt(10, 20, 200, 150); // Anchor (10, 20), size 200x150
	 * if (rect.contains({50, 25})) {
	 *	 std::wcout << L"Point is inside the rectangle";
	 * }
	 * @endcode
	 */
	struct Geometry2D
	{
		using Point = IVector2<int32_t>; ///< Represents the anchor point of the geometry.
		using Size = IVector2<uint32_t>; ///< Represents the size (width and height) of the geometry.

		union
		{
			Point anchor; ///< Anchor point of the geometry.
			struct
			{
				int32_t x; ///< X-coordinate of the anchor point.
				int32_t y; ///< Y-coordinate of the anchor point.
			};
		};

		union
		{
			Size size; ///< Size of the geometry (width and height).
			struct
			{
				uint32_t width; ///< Width of the geometry.
				uint32_t heigth; ///< Height of the geometry (note: typo should be fixed to "height").
			};
		};

		/**
		 * @brief Default constructor.
		 * Initializes the geometry with an anchor at (0, 0) and size (0, 0).
		 */
		Geometry2D();

		/**
		 * @brief Constructs a geometry with the specified anchor and size.
		 * @param p_anchor The anchor point.
		 * @param p_size The size of the geometry.
		 */
		Geometry2D(const Point& p_anchor, const Size& p_size);

		/**
		 * @brief Constructs a geometry with the specified anchor and size components.
		 * @param p_anchor The anchor point.
		 * @param p_width The width of the geometry.
		 * @param p_heigth The height of the geometry.
		 */
		Geometry2D(const Point& p_anchor, size_t p_width, size_t p_heigth);

		/**
		 * @brief Constructs a geometry with the specified anchor components and size.
		 * @param p_x X-coordinate of the anchor point.
		 * @param p_y Y-coordinate of the anchor point.
		 * @param p_size The size of the geometry.
		 */
		Geometry2D(int p_x, int p_y, const Size& p_size);

		/**
		 * @brief Constructs a geometry with the specified anchor and size components.
		 * @param p_x X-coordinate of the anchor point.
		 * @param p_y Y-coordinate of the anchor point.
		 * @param p_width The width of the geometry.
		 * @param p_heigth The height of the geometry.
		 */
		Geometry2D(int p_x, int p_y, size_t p_width, size_t p_heigth);

		/**
		 * @brief Checks if a point is contained within the geometry.
		 * @param p_point The point to check.
		 * @return `true` if the point is inside the geometry, `false` otherwise.
		 */
		bool contains(const Point& p_point) const;

		/**
		 * @brief Outputs the geometry's anchor and size to a wide output stream.
		 * @param os The output stream.
		 * @param p_geometry The geometry to output.
		 * @return A reference to the output stream.
		 */
		friend std::wostream& operator<<(std::wostream& os, const Geometry2D& p_geometry);

		/**
		 * @brief Compares two geometries for equality.
		 * @param p_other The geometry to compare.
		 * @return `true` if the geometries are equal, `false` otherwise.
		 */
		bool operator==(const Geometry2D& p_other) const;

		/**
		 * @brief Compares two geometries for inequality.
		 * @param p_other The geometry to compare.
		 * @return `true` if the geometries are not equal, `false` otherwise.
		 */
		bool operator!=(const Geometry2D& p_other) const;
	};
}
