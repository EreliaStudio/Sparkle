#pragma once

#include <vector>

#include "design_pattern/contract_provider.hpp"
#include "geometry/shadow_mesh.hpp"
#include "math/matrix.hpp"

namespace spk::ShadowMap
{
	struct MeshInstance
	{
		Mesh mesh;
		Matrix4x4 localTransform = Matrix4x4::identity();
	};

	class MeshFilter final
	{
	public:
		using Collection = std::vector<MeshInstance>;
		using OnEditionContractProvider = ContractProvider<const MeshFilter &>;
		using OnEditionCallback = OnEditionContractProvider::callback_type;
		using OnEditionContract = OnEditionContractProvider::Contract;

	private:
		Collection _instances;
		Matrix4x4 _baseTransform = Matrix4x4::identity();
		OnEditionContractProvider _onEditionContractProvider;

		static void _validateMatrix(const Matrix4x4 &matrix);
		void _replace(Collection instances, const Matrix4x4 &baseTransform);

	public:
		MeshFilter() = default;
		MeshFilter(const MeshFilter &) = delete;
		MeshFilter &operator=(const MeshFilter &) = delete;

		[[nodiscard]] const Collection &instances() const noexcept;
		[[nodiscard]] const Matrix4x4 &baseTransform() const noexcept;
		[[nodiscard]] OnEditionContract subscribeToEdition(OnEditionCallback callback);

		void bind(const Mesh &mesh);
		void bind(const Mesh &mesh, const Matrix4x4 &baseTransform);
		void bind(const Collection &instances);
		void bind(const Collection &instances, const Matrix4x4 &baseTransform);
		void unbind();
	};
}
