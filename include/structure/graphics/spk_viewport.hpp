#pragma once

#include "structure/graphics/spk_geometry_2D.hpp"

namespace spk
{
	/**
	 * @class Viewport
	 * @brief Represents a 2D viewport used for rendering within a specific area.
	 * 
	 * The `Viewport` class manages a rectangular area of the screen for rendering.
	 * It provides methods to define the viewport geometry, apply it to the OpenGL context,
	 * and manage transformations through screen constants.
	 * 
	 * Example usage:
	 * @code
	 * spk::Geometry2D geometry({0, 0}, {800, 600}); // Define a viewport at (0,0) with size 800x600
	 * spk::Viewport viewport(geometry);
	 * viewport.apply(); // Apply the viewport for rendering
	 * @endcode
	 */
	class Viewport
	{
	private:
		static inline float _maxLayer = 10000; ///< Maximum depth layer for the viewport.

		Geometry2D _geometry; ///< Geometry defining the viewport's position and size.

	public:
		/**
		 * @brief Default constructor.
		 * Initializes the viewport with a default geometry of size (1, 1) at position (0, 0).
		 */
		Viewport();

		/**
		 * @brief Constructs a viewport with a specified geometry.
		 * @param p_geometry The geometry defining the viewport's position and size.
		 */
		Viewport(const Geometry2D& p_geometry);

		/**
		 * @brief Retrieves the current geometry of the viewport.
		 * @return A constant reference to the `Geometry2D` defining the viewport.
		 */
		const Geometry2D& geometry() const;

		/**
		 * @brief Sets a new geometry for the viewport.
		 * @param p_geometry The new `Geometry2D` defining the viewport's position and size.
		 */
		void setGeometry(const Geometry2D& p_geometry);

		/**
		 * @brief Applies the viewport settings to the OpenGL context.
		 * 
		 * Updates the OpenGL viewport to match the defined geometry and validates screen constants.
		 * @throws std::runtime_error If the viewport has a width or height of zero.
		 */
		void apply() const;
	};
}
