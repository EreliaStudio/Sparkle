#pragma once

#include <iostream>
#include <vector>
#include "structure/container/spk_JSON_object.hpp"

namespace spk
{
	/**
	 * @struct Color
	 * @brief Represents a color using RGBA components.
	 * 
	 * The `Color` struct provides various constructors for creating colors, supports basic arithmetic
	 * operations, and integrates with JSON for serialization and deserialization.
	 * 
	 * Example usage:
	 * @code
	 * spk::Color color1(255, 0, 0); // Red
	 * spk::Color color2 = spk::Color::blue; // Predefined blue color
	 * spk::Color blended = color1 + color2; // Additive blending
	 * @endcode
	 */
	struct Color
	{
		float r; ///< Red component, range [0.0, 1.0].
		float g; ///< Green component, range [0.0, 1.0].
		float b; ///< Blue component, range [0.0, 1.0].
		float a; ///< Alpha (opacity) component, range [0.0, 1.0].

		/**
		 * @brief Default constructor.
		 * Initializes color to black with full opacity.
		 */
		Color();

		/**
		 * @brief Constructs a color from a single integer value.
		 * @param p_value Integer representation of a color (e.g., 0xRRGGBBAA).
		 */
		Color(int p_value);

		/**
		 * @brief Constructs a color from integer RGBA components.
		 * @param p_r Red component, range [0, 255].
		 * @param p_g Green component, range [0, 255].
		 * @param p_b Blue component, range [0, 255].
		 * @param p_a Alpha component, range [0, 255] (default is 255).
		 */
		Color(int p_r, int p_g, int p_b, int p_a = 255);

		/**
		 * @brief Constructs a color from floating-point RGBA components.
		 * @param p_r Red component, range [0.0, 1.0].
		 * @param p_g Green component, range [0.0, 1.0].
		 * @param p_b Blue component, range [0.0, 1.0].
		 * @param p_a Alpha component, range [0.0, 1.0] (default is 1.0).
		 */
		Color(float p_r, float p_g, float p_b, float p_a = 1.0f);

		/**
		 * @brief Constructs a color from a JSON object.
		 * @param p_object A JSON object containing RGBA components.
		 * @throws std::invalid_argument If required fields are missing or invalid.
		 */
		Color(const JSON::Object& p_object);

		/**
		 * @brief Adds two colors component-wise.
		 * @param p_color The color to add.
		 * @return A new `Color` instance with the summed components.
		 */
		Color operator+(const Color& p_color) const;

		/**
		 * @brief Subtracts one color from another component-wise.
		 * @param p_color The color to subtract.
		 * @return A new `Color` instance with the subtracted components.
		 */
		Color operator-(const Color& p_color) const;

		/**
		 * @brief Compares two colors for equality.
		 * @param p_color The color to compare.
		 * @return `true` if all components are equal, `false` otherwise.
		 */
		bool operator==(const Color& p_color) const;

		/**
		 * @brief Streams the color components to an output stream.
		 * @param p_os The output stream.
		 * @param p_color The color to stream.
		 * @return A reference to the output stream.
		 */
		friend std::ostream& operator<<(std::ostream& p_os, const spk::Color& p_color);

		/**
		 * @brief Serializes the color to a JSON object.
		 * @param p_object The JSON object to populate.
		 * @param p_color The color to serialize.
		 * @return A reference to the JSON object.
		 */
		friend spk::JSON::Object& operator<<(spk::JSON::Object& p_object, const spk::Color& p_color);

		/**
		 * @brief Deserializes a color from a JSON object.
		 * @param p_object The JSON object containing the color data.
		 * @param p_color The color to populate.
		 * @return A reference to the JSON object.
		 * @throws std::invalid_argument If required fields are missing or invalid.
		 */
		friend const spk::JSON::Object& operator>>(const spk::JSON::Object& p_object, spk::Color& p_color);

		// Predefined colors
		static const Color red;	///< Predefined red color.
		static const Color blue;   ///< Predefined blue color.
		static const Color green;  ///< Predefined green color.
		static const Color yellow; ///< Predefined yellow color.
		static const Color purple; ///< Predefined purple color.
		static const Color cyan;   ///< Predefined cyan color.
		static const Color orange; ///< Predefined orange color.
		static const Color magenta;///< Predefined magenta color.
		static const Color pink;   ///< Predefined pink color.
		static const Color black;  ///< Predefined black color.
		static const Color white;  ///< Predefined white color.
		static const Color grey;   ///< Predefined grey color.
	};
}
