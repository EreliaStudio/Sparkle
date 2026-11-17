#include "shadow_map/contributor.hpp"

#include <utility>

namespace spk::ShadowMap
{
	Contributor::Contributor(const std::string &name, Entity3D *owner) :
		Component3D(name, owner)
	{
		_meshFilterEditionContract = _meshFilter.subscribeToEdition(
			[this](const MeshFilter &) {
				_notify(Edition::Kind::Binding, _shadowMode);
			});
	}

	void Contributor::_notify(Edition::Kind kind, ShadowMode previousMode)
	{
		const bool changedMode = kind == Edition::Kind::ShadowMode;
		const Edition edition{
			kind,
			previousMode,
			_shadowMode,
			changedMode || _shadowMode == ShadowMode::Static,
			changedMode || _shadowMode != ShadowMode::None};
		_onEditionContractProvider.trigger(*this, edition);
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
		const ShadowMode previous = _shadowMode;
		_shadowMode = mode;
		_notify(Edition::Kind::ShadowMode, previous);
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
		_notify(Edition::Kind::Layers, _shadowMode);
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
