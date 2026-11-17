#pragma once

#include <string>
#include <vector>

#include "engine/component3d.hpp"
#include "engine/registry.hpp"
#include "shadow_map/mesh_filter.hpp"

namespace spk::ShadowMap
{
	enum class ShadowMode
	{
		None,
		Static,
		Dynamic
	};

	class Contributor final : public Component3D,
							  public Registry<Contributor, EngineHeadless *>::Object
	{
	public:
		using Layers = std::vector<std::string>;
		using OnEditionContractProvider = ContractProvider<const Contributor &>;
		using OnEditionCallback = OnEditionContractProvider::callback_type;
		using OnEditionContract = OnEditionContractProvider::Contract;

	private:
		MeshFilter _meshFilter;
		ShadowMode _shadowMode = ShadowMode::None;
		Layers _layers;
		OnEditionContractProvider _onEditionContractProvider;
		MeshFilter::OnEditionContract _meshFilterEditionContract;

		void _notifyEdition();

	public:
		explicit Contributor(
			const std::string &name = "Shadow contributor",
			Entity3D *owner = nullptr);

		[[nodiscard]] MeshFilter &meshFilter() noexcept;
		[[nodiscard]] const MeshFilter &meshFilter() const noexcept;
		[[nodiscard]] ShadowMode shadowMode() const noexcept;
		void setShadowMode(ShadowMode mode);
		[[nodiscard]] const Layers &layers() const noexcept;
		void setLayers(Layers layers);
		[[nodiscard]] OnEditionContract subscribeToEdition(OnEditionCallback callback);

		// Convenience forwarding: every successful call generates one notification.
		void bind(const Mesh &mesh);
		void bind(const Mesh &mesh, const Matrix4x4 &baseTransform);
		void bind(const MeshFilter::Collection &instances);
		void bind(const MeshFilter::Collection &instances, const Matrix4x4 &baseTransform);
		void unbind();
	};
}
