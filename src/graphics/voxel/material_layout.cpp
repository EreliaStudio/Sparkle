#include "voxel/material_layout.hpp"

#include <algorithm>
#include <utility>

#include "exception.hpp"

namespace spk::Voxel
{
	MaterialLayout::MaterialLayout(std::vector<std::string> names) :
		_names(std::move(names))
	{
		if (!std::is_sorted(_names.begin(), _names.end()) ||
			std::adjacent_find(_names.begin(), _names.end()) != _names.end())
		{
			throw spk::Exception("Voxel MaterialLayout: names must be unique and sorted");
		}
	}

	const std::vector<std::string> &MaterialLayout::names() const noexcept
	{
		return _names;
	}

	bool MaterialLayout::isCompatible(const spk::Palette &palette) const noexcept
	{
		if (_names.size() != palette.materials().size())
		{
			return false;
		}
		auto it = palette.materials().begin();
		for (const auto &name : _names)
		{
			if (it->first != name)
			{
				return false;
			}
			++it;
		}
		return true;
	}
}
