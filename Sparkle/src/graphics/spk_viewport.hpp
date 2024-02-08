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
		
	private:
		static inline const Viewport* _mainViewport = nullptr;
		static inline const Viewport* _activeViewport = nullptr;
		static inline spk::Pipeline::Constant* _screenConstant = nullptr;

		spk::Vector2Int _anchor;
		spk::Vector2UInt _size;

	public:

        /**
         * @brief Retrieves the currently active viewport.
         * 
         * This static method returns a pointer to the viewport currently set as active. If no viewport has been activated, it may return nullptr.
         * 
         * @return A pointer to the currently active viewport, or nullptr if none is active.
         */
        static const Viewport* activeViewport();

		/**
         * @brief Constructor for the Viewport class.
         * 
         * Initializes a new instance of the Viewport class, setting up default values for the anchor point and size of the viewport. The default viewport covers the entire application window.
         */
        Viewport();

        /**
         * @brief Sets the geometry of the viewport by specifying the anchor point and size.
         * 
         * This method defines the area of the window where rendering will occur. The viewport is characterized by an anchor point (bottom-left corner) and its size (width and height).
         * 
         * @param p_anchor The anchor point of the viewport as a Vector2Int (x, y).
         * @param p_size The size of the viewport as a Vector2UInt (width, height).
         */
        void setGeometry(const spk::Vector2Int& p_anchor, const spk::Vector2UInt& p_size);

        /**
         * @brief Activates this viewport for rendering.
         * 
         * Sets the current viewport as the active one, meaning that subsequent rendering operations will occur within this viewport's defined area. Only one viewport can be active at a time.
         */
        void activate() const;

        /**
         * @brief Gets the anchor point of the viewport.
         * 
         * The anchor point is the bottom-left corner of the viewport within the application window.
         * 
         * @return The anchor point of the viewport as a Vector2Int.
         */
        const spk::Vector2Int& anchor() const;

        /**
         * @brief Gets the size of the viewport.
         * 
         * The size is defined by the width and height of the viewport area where rendering occurs.
         * 
         * @return The size of the viewport as a Vector2UInt.
         */
        const spk::Vector2UInt& size() const;

        /**
         * @brief Converts screen coordinates to GPU-specific positions.
         * 
         * This static method is useful for converting screen coordinates (e.g., mouse position) to the GPU's coordinate system, which is helpful for UI interactions and placing objects in a scene.
         * 
         * @param p_screenPosition The position on the screen to convert, as a Vector2Int.
         * @return The corresponding position in the GPU's coordinate system as a Vector2.
         */
        static spk::Vector2 convertScreenToGPUPosition(const spk::Vector2Int& p_screenPosition);
    };
}