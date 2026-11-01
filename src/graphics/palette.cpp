#include "graphics/palette.hpp"

#include <algorithm>
#include <cctype>
#include <utility>

#include "exception.hpp"

namespace spk
{
	Palette::Palette(Materials materials) :
		_materials(std::make_shared<const Materials>(std::move(materials)))
	{
	}

	Palette::Palette() :
		Palette(Materials{})
	{
	}

	const Palette::Materials &Palette::materials() const noexcept
	{
		return *_materials;
	}

	const Material *Palette::find(std::string_view name) const noexcept
	{
		const auto found = _materials->find(name);
		return found == _materials->end() ? nullptr : &found->second;
	}

	Palette::Builder::Builder(const Palette &palette) :
		_materials(palette.materials())
	{
	}

	void Palette::Builder::setMaterial(std::string name, Material material)
	{
		const bool blank = std::all_of(name.begin(), name.end(), [](unsigned char character) {
			return std::isspace(character) != 0;
		});
		if (blank)
		{
			throw Exception("Palette: material slot name cannot be empty or whitespace");
		}
		_materials.insert_or_assign(std::move(name), std::move(material));
	}

	Palette Palette::Builder::build() const
	{
		return Palette(_materials);
	}
}
