#include "voxel/model_element.hpp"

#include <string>
#include <utility>

#include "voxel/mesher.hpp"

namespace spk::Voxel
{
	ModelElement::ModelElement(std::string name, Volume volume) :
		spk::Model3D::Blueprint::Element(std::move(name)),
		_volume(std::move(volume))
	{
	}

	void ModelElement::setVolume(Volume volume)
	{
		_volume = std::move(volume);
		_geometry.reset();
	}

	const Volume &ModelElement::volume() const noexcept
	{
		return _volume;
	}

	const Geometry &ModelElement::geometry() const
	{
		if (!_geometry)
		{
			_geometry.emplace(Mesher{}.bake(_volume));
		}
		return *_geometry;
	}
}
