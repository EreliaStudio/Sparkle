#include "widget/components/spk_colored_box.hpp"

namespace spk::widget::components
{
    const std::string ColoredBox::_renderingPipelineCode = R"(#version 450

		#include <screenConstants>
		
		Input -> Geometry : ivec2 modelPosition;

		AttributeBlock self
		{
			vec4 color;
			float depth;
		};

		void geometryPass()
		{
			pixelPosition = screenConstants.canvasMVP * vec4(modelPosition, 0, 0);
		}
		
		void renderPass()
		{
			pixelColor = self.color;
		})";
    spk::Pipeline ColoredBox::_renderingPipeline = spk::Pipeline(_renderingPipelineCode);

    ColoredBox::ShaderInput::ShaderInput(const spk::Vector2Int& p_position) :
        position(p_position)
    {
    }

    void ColoredBox::_updateGPUData()
    {
        std::vector<ShaderInput> data;
        std::vector<unsigned int> indexes = {0, 1, 2, 2, 1, 3};

        data.push_back(ShaderInput(anchor() + size() * spk::Vector2Int(0, 0)));
        data.push_back(ShaderInput(anchor() + size() * spk::Vector2Int(0, 1)));
        data.push_back(ShaderInput(anchor() + size() * spk::Vector2Int(1, 0)));
        data.push_back(ShaderInput(anchor() + size() * spk::Vector2Int(1, 1)));

        _renderingObject.setVertices(data);
        _renderingObject.setIndexes(indexes);
    }

    ColoredBox::ColoredBox() :
        _renderingObject(_renderingPipeline.createObject()),
        _selfAttribute(_renderingObject.attribute("self")),
        _selfColorElement(_selfAttribute["color"]),
        _selfDepthElement(_selfAttribute["depth"])
    {
    }

    void ColoredBox::render()
    {
        if (_needGPUInputUpdate == true)
        {
            _updateGPUData();
            _needGPUInputUpdate = false;
        }

        _renderingObject.render();
    }

    void ColoredBox::setColor(const spk::Color& p_color)
    {
        _selfColorElement = p_color;
        _selfAttribute.update();
    }

    const Vector2& ColoredBox::anchor() const
    {
        return _anchor;
    }

    const Vector2& ColoredBox::size() const
    {
        return _size;
    }

    void ColoredBox::setDepth(float p_newDepth)
    {
        _selfDepthElement = p_newDepth;
        _selfAttribute.update();
    }

    void ColoredBox::setGeometry(const Vector2 p_anchor, const Vector2 p_newSize)
    {
        _anchor = p_anchor;
        _size = p_newSize;
        _needGPUInputUpdate = true;
    }
}