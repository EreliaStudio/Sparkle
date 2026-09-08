#include "design_pattern/trait/name_trait.hpp"

#include <utility>

namespace spk
{
	NameTrait::NameTrait(std::string name) :
		_name(std::move(name))
	{
	}

	NameTrait::NameTrait(const NameTrait &other) :
		_name(other._name)
	{
	}

	NameTrait::NameTrait(NameTrait &&other) noexcept :
		_name(std::move(other._name))
	{
	}

	NameTrait &NameTrait::operator=(const NameTrait &other)
	{
		if (this != &other)
		{
			setName(other._name);
		}
		return *this;
	}

	NameTrait &NameTrait::operator=(NameTrait &&other)
	{
		if (this != &other)
		{
			_name = std::move(other._name);
			_nameEditionProvider.trigger(_name);
		}
		return *this;
	}

	void NameTrait::setName(std::string name)
	{
		if (_name == name)
		{
			return;
		}
		_name = std::move(name);
		_nameEditionProvider.trigger(_name);
	}

	const std::string &NameTrait::name() const noexcept
	{
		return _name;
	}

	NameTrait::NameEditionContract NameTrait::subscribeToNameEdition(NameEditionCallback callback)
	{
		return _nameEditionProvider.subscribe(std::move(callback));
	}
}
