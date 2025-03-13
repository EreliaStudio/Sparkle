#include "structure/engine/spk_material.hpp"

namespace spk
{
	Material::Material() :
		_diffuse(nullptr),
		_specular(nullptr),
		_shininess(0)
	{
	}

	void Material::setDiffuse(spk::SafePointer<Image> p_diffuse)
	{
		_diffuse = p_diffuse;
	}
	void Material::setSpecular(spk::SafePointer<Image> p_specular)
	{
		_specular = p_specular;
	}
	void Material::setShininess(const float &p_shininess)
	{
		_shininess = p_shininess;
	}

	const spk::SafePointer<Image> &Material::diffuse() const
	{
		return (_diffuse);
	}

	const spk::SafePointer<Image> &Material::specular() const
	{
		return (_specular);
	}

	const float &Material::shininess() const
	{
		return (_shininess);
	}

	void Material::validate()
	{
		_onValidationContractProvider.trigger();
	}

	Material::Contract Material::subscribeToValidation(const Material::Job &p_job)
	{
		return (_onValidationContractProvider.subscribe(p_job));
	}
}