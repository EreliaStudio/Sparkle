#pragma once

#include "math/spk_vector2.hpp"
#include "graphics/pipeline/spk_pipeline.hpp"

namespace spk
{
	/**
	 * @class Viewport
	 * @brief Manages the viewport area for rendering within an application window.
	 *
	 * The Viewport class defines a specific area of the window where rendering occurs, characterized by an anchor point and size. It supports operations such as setting the viewport geometry, activating a particular viewport for rendering, and converting screen coordinates to GPU-specific positions.
	 *
	 * This class plays a crucial role in managing how content is visually presented in part of or the entire application window. It allows for the creation of multiple viewport areas, potentially enabling split-screen views or rendering different scenes within the same window.
	 *
	 * Usage example:
	 * @code
	 * spk::Viewport myViewport;
	 * myViewport.setGeometry({0, 0}, {800, 600}); // Sets the viewport to cover an 800x600 area
	 * myViewport.activate(); // Activates this viewport for subsequent rendering operations
	 * @endcode
	 *
	 * @note The Viewport class also provides a static method to retrieve the currently active viewport and a mechanism to convert screen coordinates to positions relative to the GPU's coordinate system, useful for UI interactions and object placement.
	 * @note The viewport will automaticaly be pushed to the Pipeline::Constant
	 * 
	 * @see IVector2
	 */
	class Viewport
	{
		friend class Application;
		
	public:
		static inline const Viewport* _mainViewport = nullptr;
	private:
		static inline const Viewport* _activeViewport = nullptr;
		static inline spk::Pipeline::Constant* _screenConstant = nullptr;

		spk::Vector2Int _anchor;
		spk::Vector2UInt _size;

	public:
		static const Viewport* activeViewport() { return (_activeViewport); }

		Viewport();

		void setGeometry(const spk::Vector2Int& p_anchor, const spk::Vector2UInt& p_size);
		void activate() const;

		const spk::Vector2Int& anchor() const;
		const spk::Vector2UInt& size() const;

		static spk::Vector2 convertScreenToGPUPosition(const spk::Vector2Int& p_screenPosition);
	};
}