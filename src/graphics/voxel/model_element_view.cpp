#include "voxel/model_element_view.hpp"

#include <cstdint>
#include <utility>
#include <vector>

#include "exception.hpp"
#include "voxel/mesher.hpp"
#include "voxel/renderer.hpp"

namespace spk::Voxel
{
	ModelElementView::ModelElementView(const ModelElement &element) :
		_element(&element)
	{
	}

	const Geometry &ModelElementView::_geometry() const
	{
		return _replacementGeometry ? *_replacementGeometry : _element->geometry();
	}

	void ModelElementView::bind(Volume volume)
	{
		Geometry geometry = Mesher{}.bake(volume);
		std::vector<std::uint32_t> remap;
		if (_palette)
		{
			remap = Renderer::resolveMaterialRemap(geometry, *_palette);
		}
		_replacement = std::move(volume);
		_replacementGeometry = std::move(geometry);
		_materialRemap = std::move(remap);
	}

	void ModelElementView::bind(spk::Palette palette)
	{
		auto remap = Renderer::resolveMaterialRemap(_geometry(), palette);
		_palette = std::move(palette);
		_materialRemap = std::move(remap);
	}

	void ModelElementView::clearVolumeOverride()
	{
		std::vector<std::uint32_t> remap;
		if (_palette)
		{
			remap = Renderer::resolveMaterialRemap(_element->geometry(), *_palette);
		}
		_replacement.reset();
		_replacementGeometry.reset();
		_materialRemap = std::move(remap);
	}

	void ModelElementView::clearPalette() noexcept
	{
		_palette.reset();
		_materialRemap.clear();
	}

	const Geometry &ModelElementView::geometry() const
	{
		return _geometry();
	}

	const spk::Palette *ModelElementView::palette() const noexcept
	{
		return _palette ? &*_palette : nullptr;
	}

	const std::vector<std::uint32_t> &ModelElementView::materialRemap() const noexcept
	{
		return _materialRemap;
	}

	void ModelElementView::buildRenderSnapshot(
		spk::RenderSnapshot::Builder &builder,
		const spk::Matrix4x4 &model)
	{
		(void)builder;
		(void)model;
		throw spk::Exception("Voxel ModelElementView requires an EngineView capture batch");
	}

	void ModelElementView::buildRenderSnapshot(
		spk::RenderSnapshot::Builder &builder,
		const spk::Matrix4x4 &model,
		RenderBatch &batch)
	{
		(void)builder;
		if (_palette)
		{
			batch.append(_geometry(), *_palette, _materialRemap, model);
		}
	}
}
