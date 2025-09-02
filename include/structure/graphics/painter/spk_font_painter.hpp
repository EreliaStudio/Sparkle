#pragma once

#include <filesystem>
#include <map>
#include <unordered_map>
#include <vector>

#include "structure/graphics/opengl/spk_buffer_set.hpp"
#include "structure/graphics/opengl/spk_program.hpp"
#include "structure/graphics/opengl/spk_sampler_object.hpp"
#include "structure/graphics/opengl/spk_uniform_buffer_object.hpp"
#include "structure/graphics/spk_color.hpp"
#include "structure/graphics/texture/spk_font.hpp"
#include "structure/math/spk_vector2.hpp"
#include "structure/math/spk_vector4.hpp"

namespace spk
{
	class FontPainter
	{
	public:
		using Contract = spk::ContractProvider::Contract;
		using Job = spk::ContractProvider::Job;

		struct Vertex
		{
			spk::Vector2 position;
			float layer;
			spk::Vector2 uv;
		};

	private:
		static inline std::unique_ptr<spk::OpenGL::Program> _program;
		spk::OpenGL::BufferSet _bufferSet;
		spk::OpenGL::SamplerObject _samplerObject;

		// Uniform buffer object for layer, glyphColor, outlineColor and SDF parameters
		spk::OpenGL::UniformBufferObject _textInformationsUniformBufferObject;

		spk::SafePointer<Font> _font = nullptr;
		Font::Size _fontSize = {16, 1};
		spk::SafePointer<Font::Atlas> _atlas = nullptr;
		spk::Font::Atlas::Contract _onAtlasEditionContract;
		spk::ContractProvider _onFontEditionContractProvider;

		float _layer = 0.0f;
		spk::Color _glyphColor = spk::Color(255, 255, 255, 255); // white by default
		spk::Color _outlineColor = spk::Color(0, 0, 0, 255);	 // black by default
		float _outlineThreshold = 0.5f;
		float _sdfSmoothing = 1.0f;

		void _initProgram();
		void _initBuffers();

		void _updateUniformBufferObject();

		void _prepareText(const std::wstring &p_text, const spk::Vector2Int &p_anchor, float p_layer);

	public:
		FontPainter();

		FontPainter::Contract subscribeToFontEdition(const Job &p_job);

		void setFont(const spk::SafePointer<Font> &p_font);
		void setFontSize(const Font::Size &p_fontSize);
		void setGlyphColor(const spk::Color &p_color);
		void setOutlineColor(const spk::Color &p_color);
		void setSdfSmoothing(float p_sdfSmoothing);

		const spk::SafePointer<spk::Font> &font() const;
		const spk::Font::Size &fontSize() const;
		const spk::Color &glyphColor() const;
		const spk::Color &outlineColor() const;
		float sdfSmoothing() const;

		void clear();

		spk::Vector2Int computeTextBaselineOffset(const std::wstring &p_text) const;
		spk::Vector2UInt computeTextSize(const std::wstring &p_text) const;
		spk::Vector2Int computeTextAnchor(
			const spk::Geometry2D &p_geometry,
			const std::wstring &p_string,
			spk::HorizontalAlignment p_horizontalAlignment,
			spk::VerticalAlignment p_verticalAlignment) const;

		void prepare(const std::wstring &p_text, const spk::Vector2Int &p_anchor, float p_layer);

		void validate();

		void render();
	};
}
