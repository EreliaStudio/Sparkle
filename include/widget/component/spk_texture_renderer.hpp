#pragma once

#include "structure/spk_safe_pointer.hpp"
#include "structure/graphics/texture/spk_image.hpp"
#include "structure/graphics/spk_pipeline.hpp"

namespace spk::WidgetComponent
{
	class TextureRenderer
	{
	private:
		static std::string _pipelineCode;
		static spk::Pipeline _pipeline;

		spk::Pipeline::Object _object;
		spk::Pipeline::Object::Attribute& _textureInformations;
		spk::Pipeline::Object::Attribute::Element& _layerElement;
		spk::Pipeline::Object::Attribute::Element& _alphaElement;
		spk::Pipeline::Object::Sampler2D& _textureSampler2D;

		bool _needUpdateGPU = false;

		float _layer = 0.0f;
		float _alpha = 1.0f;
		spk::Geometry2D _geometry = {};
		spk::SafePointer<spk::Image> _texture = nullptr;
		spk::Image::Section _section = { {0, 0}, {1, 1} };

		void _updateGPUData();

	public:
		TextureRenderer();

		const float& layer() const;
		const float& alpha() const;
		const spk::Geometry2D& geometry() const;
		const spk::SafePointer<spk::Image>& texture() const;
		const spk::Image::Section& section() const;

		void setLayer(const float& p_layer);
		void setAlpha(const float& p_alpha);
		void setGeometry(const spk::Geometry2D& p_geometry);
		void setTexture(const spk::SafePointer<spk::Image>& p_texture);
		void setSection(const spk::Image::Section& p_section);
	
		void render();
	};
}