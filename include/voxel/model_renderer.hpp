#pragma once

#include <optional>
#include <string>

#include "engine/registry.hpp"
#include "model3d/object.hpp"
#include "rendering/engine/rendering_component.hpp"
#include "voxel/model_element_view.hpp"

namespace spk::Voxel
{
	class ModelRenderer final : public spk::RenderingComponent3D,
								public spk::Registry<ModelRenderer, spk::EngineHeadless *>::Object
	{
	public:
		using Model = spk::Model3D::Object<ModelElementView>;

	private:
		std::optional<Model> _model;
		std::optional<spk::Palette> _defaultPalette;

		void _applyPalette(const spk::Model3D::Attachable &node, bool replace);
		void _prepare();

	public:
		explicit ModelRenderer(const std::string &name = "Voxel model renderer", spk::Entity3D *owner = nullptr);

		void bind(spk::Model3D::Blueprint blueprint);
		void bind(spk::Palette palette);
		void clearModel() noexcept;
		[[nodiscard]] Model *model() noexcept;
		[[nodiscard]] const Model *model() const noexcept;

		void collect(
			spk::RenderSnapshot::Builder &builder,
			const spk::Matrix4x4 &modelTransform,
			RenderBatch &batch);
	};
}
