#include "shadow_map/mesh_filter.hpp"

#include <cmath>
#include <utility>

#include "exception.hpp"

namespace spk::ShadowMap
{
	void MeshFilter::_validateMatrix(const Matrix4x4 &matrix)
	{
		for (std::size_t column = 0; column < 4; ++column)
		{
			for (std::size_t row = 0; row < 4; ++row)
			{
				if (!std::isfinite(matrix[column][row]))
				{
					throw spk::Exception("ShadowMap MeshFilter transform must have finite entries");
				}
			}
		}
	}

	void MeshFilter::_replace(Collection instances, const Matrix4x4 &baseTransform)
	{
		_validateMatrix(baseTransform);
		for (const MeshInstance &instance : instances)
		{
			_validateMatrix(instance.localTransform);
		}

		// Everything that can fail (copy and validation) precedes publication.
		_instances.swap(instances);
		_baseTransform = _instances.empty() ? Matrix4x4::identity() : baseTransform;
		_onEditionContractProvider.trigger(*this);
	}

	const MeshFilter::Collection &MeshFilter::instances() const noexcept
	{
		return _instances;
	}

	const Matrix4x4 &MeshFilter::baseTransform() const noexcept
	{
		return _baseTransform;
	}

	MeshFilter::OnEditionContract MeshFilter::subscribeToEdition(OnEditionCallback callback)
	{
		return _onEditionContractProvider.subscribe(std::move(callback));
	}

	void MeshFilter::bind(const Mesh &mesh)
	{
		bind(mesh, Matrix4x4::identity());
	}

	void MeshFilter::bind(const Mesh &mesh, const Matrix4x4 &baseTransform)
	{
		bind(Collection{MeshInstance{mesh, Matrix4x4::identity()}}, baseTransform);
	}

	void MeshFilter::bind(const Collection &instances)
	{
		bind(instances, Matrix4x4::identity());
	}

	void MeshFilter::bind(const Collection &instances, const Matrix4x4 &baseTransform)
	{
		_replace(instances, baseTransform);
	}

	void MeshFilter::unbind()
	{
		_replace({}, Matrix4x4::identity());
	}
}
