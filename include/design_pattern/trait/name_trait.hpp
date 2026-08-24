#pragma once

#include <string>

namespace spk
{
	class NameTrait
	{
	public:
		NameTrait(std::string name);

		const std::string &name() const;

	private:
		std::string _name;
	};
}
