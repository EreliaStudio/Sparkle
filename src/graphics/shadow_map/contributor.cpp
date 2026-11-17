#include "shadow_map/contributor.hpp"

#include <utility>

namespace spk::ShadowMap
{
	Contributor::Contributor(const std::string &name, Entity3D *owner) :
		Component3D(name, owner)
	{
		_meshFilterEditionContract = _meshFilter.subscribeToEdition(
			[this](const MeshFilter &) {
				_notifyEdition();
			});
	}

	void Contributor::_notifyEdition()
	{
		_onEditionContractProvider.trigger(*this);
	}

	MeshFilter &Contributor::meshFilter() noexcept
	{
		return _meshFilter;
	}

	const MeshFilter &Contributor::meshFilter() const noexcept
	{
		return _meshFilter;
	}

	ShadowMode Contributor::shadowMode() const noexcept
	{
		return _shadowMode;
	}

	void Contributor::setShadowMode(ShadowMode mode)
	{
		if (_shadowMode == mode)
		{
			return;
		}
		_shadowMode = mode;
		_notifyEdition();
	}

	const Contributor::Layers &Contributor::layers() const noexcept
	{
		return _layers;
	}

	void Contributor::setLayers(Layers layers)
	{
		if (_layers == layers)
		{
			return;
		}
		_layers.swap(layers);
		_notifyEdition();
	}

	Contributor::OnEditionContract Contributor::subscribeToEdition(OnEditionCallback callback)
	{
		return _onEditionContractProvider.subscribe(std::move(callback));
	}

	void Contributor::bind(const Mesh &mesh)
	{
		_meshFilter.bind(mesh);
	}

	void Contributor::bind(const Mesh &mesh, const Matrix4x4 &baseTransform)
	{
		_meshFilter.bind(mesh, baseTransform);
	}

	void Contributor::bind(const MeshFilter::Collection &instances)
	{
		_meshFilter.bind(instances);
	}

	void Contributor::bind(const MeshFilter::Collection &instances, const Matrix4x4 &baseTransform)
	{
		_meshFilter.bind(instances, baseTransform);
	}

	void Contributor::unbind()
	{
		_meshFilter.unbind();
	}
}
