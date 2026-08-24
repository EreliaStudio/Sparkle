#include "design_pattern/trait/name_trait.hpp"

#include <utility>

namespace spk
{
	NameTrait::NameTrait(std::string name) :
		_name(std::move(name))
	{
	}

	const std::string &NameTrait::name() const
	{
		return _name;
	}
}
