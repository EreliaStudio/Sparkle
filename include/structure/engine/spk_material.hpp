#pragma once

#include "structure/spk_safe_pointer.hpp"
#include "structure/graphics/texture/spk_image.hpp"

namespace spk
{
	class Material
	{
	private:
		spk::SafePointer<Image> _diffuse;
		spk::SafePointer<Image> _specular;
		float _shininess;

	public:
		Material();

		void setDiffuse(spk::SafePointer<Image> p_diffuse);
		void setSpecular(spk::SafePointer<Image> p_specular);
		void setShininess(const float& p_shininess);

		const spk::SafePointer<Image>& diffuse() const;
		const spk::SafePointer<Image>& specular() const;
		const float& shininess() const;
	};
}