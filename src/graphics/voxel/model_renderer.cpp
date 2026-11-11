#include "voxel/model_renderer.hpp"

#include <string>
#include <utility>

namespace spk::Voxel
{
	ModelRenderer::ModelRenderer(const std::string &name, spk::Entity3D *owner) :
		RenderingComponent3D(name, owner)
	{
	}

	void ModelRenderer::bind(spk::Model3D::Blueprint blueprint)
	{
		_model.emplace(std::move(blueprint));
	}

	void ModelRenderer::bind(spk::Palette palette)
	{
		_defaultPalette = std::move(palette);
		if (_model)
		{
			_model->synchronize();
			_applyPalette(*_model, true);
		}
	}

	void ModelRenderer::clearModel() noexcept
	{
		_model.reset();
	}

	ModelRenderer::Model *ModelRenderer::model() noexcept
	{
		return _model ? &*_model : nullptr;
	}

	const ModelRenderer::Model *ModelRenderer::model() const noexcept
	{
		return _model ? &*_model : nullptr;
	}

	void ModelRenderer::_applyPalette(const spk::Model3D::Attachable &node, bool replace)
	{
		if (const auto *element = dynamic_cast<const Model::ElementNode *>(&node))
		{
			if (auto *graphics = _model->graphics(*element);
				graphics != nullptr && (replace || graphics->palette() == nullptr))
			{
				graphics->bind(*_defaultPalette);
			}
		}
		for (const auto *child : node.children())
		{
			_applyPalette(*child, replace);
		}
	}

	void ModelRenderer::_prepare()
	{
		_model->synchronize();
		if (_defaultPalette)
		{
			_applyPalette(*_model, false);
		}
	}

	void ModelRenderer::collect(
		spk::RenderSnapshot::Builder &builder,
		const spk::Matrix4x4 &modelTransform,
		RenderBatch &batch)
	{
		if (!_model)
		{
			return;
		}
		_prepare();
		_model->buildRenderSnapshot(builder, modelTransform, batch);
	}
}
