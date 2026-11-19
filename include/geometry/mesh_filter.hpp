#pragma once

#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>

#include "design_pattern/contract_provider.hpp"
#include "exception.hpp"
#include "math/matrix.hpp"

namespace spk
{
	// A collection of immutable/shared mesh values and their relative transforms.
	// This class is independent of render passes, materials, and shadow policy.
	template <typename TMesh>
	class MeshFilter final
	{
	public:
		struct Instance
		{
			TMesh mesh;
			Matrix4x4 localTransform = Matrix4x4::identity();
		};

		using Collection = std::vector<Instance>;
		using OnEditionContractProvider = ContractProvider<const MeshFilter &>;
		using OnEditionCallback = OnEditionContractProvider::callback_type;
		using OnEditionContract = OnEditionContractProvider::Contract;

	private:
		Collection _instances;
		Matrix4x4 _baseTransform = Matrix4x4::identity();
		OnEditionContractProvider _onEditionContractProvider;

		static void _validateMatrix(const Matrix4x4 &matrix)
		{
			for (std::size_t column = 0; column < 4; ++column)
			{
				for (std::size_t row = 0; row < 4; ++row)
				{
					if (!std::isfinite(matrix[column][row]))
					{
						throw spk::Exception("MeshFilter transform must have finite entries");
					}
				}
			}
		}

		void _replace(Collection instances, const Matrix4x4 &baseTransform)
		{
			_validateMatrix(baseTransform);
			for (const Instance &instance : instances)
			{
				_validateMatrix(instance.localTransform);
			}

			// Copies and validation complete before either field changes.
			_instances.swap(instances);
			_baseTransform = _instances.empty() ? Matrix4x4::identity() : baseTransform;
			_onEditionContractProvider.trigger(*this);
		}

	public:
		MeshFilter() = default;
		MeshFilter(const MeshFilter &) = delete;
		MeshFilter &operator=(const MeshFilter &) = delete;

		[[nodiscard]] const Collection &instances() const noexcept
		{
			return _instances;
		}

		[[nodiscard]] const Matrix4x4 &baseTransform() const noexcept
		{
			return _baseTransform;
		}

		[[nodiscard]] OnEditionContract subscribeToEdition(OnEditionCallback callback)
		{
			return _onEditionContractProvider.subscribe(std::move(callback));
		}

		void bind(const TMesh &mesh)
		{
			bind(mesh, Matrix4x4::identity());
		}

		void bind(const TMesh &mesh, const Matrix4x4 &baseTransform)
		{
			bind(Collection{Instance{mesh, Matrix4x4::identity()}}, baseTransform);
		}

		void bind(const Collection &instances)
		{
			bind(instances, Matrix4x4::identity());
		}

		void bind(const Collection &instances, const Matrix4x4 &baseTransform)
		{
			_replace(instances, baseTransform);
		}

		void unbind()
		{
			_replace({}, Matrix4x4::identity());
		}
	};
}
