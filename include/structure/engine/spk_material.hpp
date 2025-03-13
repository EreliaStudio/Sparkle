#pragma once

#include "structure/graphics/texture/spk_image.hpp"
#include "structure/spk_safe_pointer.hpp"

#include "structure/design_pattern/spk_contract_provider.hpp"

namespace spk
{
	class Material
	{
	public:
		using Contract = ContractProvider::Contract;
		using Job = ContractProvider::Job;

	private:
		spk::ContractProvider _onValidationContractProvider;

		spk::SafePointer<Image> _diffuse;
		spk::SafePointer<Image> _specular;
		float _shininess;

	public:
		Material();

		void setDiffuse(spk::SafePointer<Image> p_diffuse);
		void setSpecular(spk::SafePointer<Image> p_specular);
		void setShininess(const float &p_shininess);

		const spk::SafePointer<Image> &diffuse() const;
		const spk::SafePointer<Image> &specular() const;
		const float &shininess() const;

		void validate();
		Contract subscribeToValidation(const Job &p_job);
	};
}