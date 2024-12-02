#pragma once

#include <string>
#include <stdexcept>
#include <GL/glew.h>
#include "structure/spk_safe_pointer.hpp"
#include "spk_texture_object.hpp"

namespace spk::OpenGL
{
	/**
	 * @class SamplerObject
	 * @brief Represents an OpenGL sampler object used to bind and activate textures in a shader.
	 * 
	 * The SamplerObject class allows managing OpenGL textures, binding them to texture units,
	 * and activating them for rendering.
	 * 
	 * Example usage:
	 * @code
	 * spk::OpenGL::SamplerObject sampler;
	 * spk::SafePointer<spk::OpenGL::TextureObject> texture = myTexture;
	 * sampler.bind(texture);
	 * sampler.activate();
	 * @endcode
	 */
	class SamplerObject
	{
	public:
		/**
		 * @class Factory
		 * @brief Provides a builder for constructing SamplerObject instances.
		 * 
		 * The Factory class simplifies the creation of SamplerObject by providing
		 * a step-by-step configuration interface.
		 */
		class Factory
		{
		private:
			std::string _name; ///< Name of the sampler object.
			std::string _designator; ///< Designator string associated with the sampler.
			size_t _index; ///< Index of the sampler object.

		public:
			/**
			 * @brief Default constructor.
			 */
			Factory() = default;

			/**
			 * @brief Sets the name of the sampler object.
			 * @param name The name to set.
			 */
			void setName(const std::string& name);

			/**
			 * @brief Sets the designator string for the sampler object.
			 * @param designator The designator string to set.
			 */
			void setDesignator(const std::string& designator);

			/**
			 * @brief Sets the index of the sampler object.
			 * @param index The index to set.
			 */
			void setIndex(size_t index);

			/**
			 * @brief Constructs a new SamplerObject instance based on the configured values.
			 * @return A new SamplerObject.
			 */
			SamplerObject construct() const;
		};

	private:
		std::string _name; ///< Name of the sampler object.
		std::string _designator; ///< Designator string associated with the sampler.
		GLint _index; ///< OpenGL index for the sampler object.
		spk::SafePointer<TextureObject> _texture = nullptr; ///< Bound texture object.
		GLint _uniformDestination = -1; ///< Destination uniform index in the shader.

		/**
		 * @brief Private constructor used by the Factory class.
		 * @param p_name The name of the sampler object.
		 * @param p_designator The designator string.
		 * @param p_index The index of the sampler object.
		 */
		SamplerObject(const std::string& p_name, const std::string& p_designator, size_t p_index);

	public:
		/**
		 * @brief Default constructor.
		 */
		SamplerObject();

		/**
		 * @brief Copy constructor.
		 * @param p_other The SamplerObject to copy from.
		 */
		SamplerObject(const SamplerObject& p_other);

		/**
		 * @brief Copy assignment operator.
		 * @param p_other The SamplerObject to assign from.
		 * @return A reference to the assigned SamplerObject.
		 */
		SamplerObject& operator=(const SamplerObject& p_other);

		/**
		 * @brief Move constructor.
		 * @param p_other The SamplerObject to move from.
		 */
		SamplerObject(SamplerObject&& p_other) noexcept;

		/**
		 * @brief Move assignment operator.
		 * @param p_other The SamplerObject to assign from.
		 * @return A reference to the assigned SamplerObject.
		 */
		SamplerObject& operator=(SamplerObject&& p_other) noexcept;

		/**
		 * @brief Binds a texture to the sampler object.
		 * @param p_texture A safe pointer to the TextureObject to bind.
		 */
		void bind(const spk::SafePointer<TextureObject>& p_texture);

		/**
		 * @brief Activates the sampler object, making it ready for use in a shader.
		 */
		void activate();
	};
}
