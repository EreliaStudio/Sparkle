#include "shape.hpp"

namespace taag
{
	spk::Lumina::ShaderObjectFactory::Instanciator Shape::Renderer::Painter::_instanciator;
	spk::Lumina::Shader Shape::Renderer::Painter::_shader = _createShader();
	
	std::unordered_map<Shape::Type, Shape::Renderer::Painter> Shape::Renderer::painters = {
		{Shape::Type::Triangle, Shape::Renderer::Painter(_makeMesh(Shape::Type::Triangle))},
		{Shape::Type::Square, Shape::Renderer::Painter(_makeMesh(Shape::Type::Square))},
		{Shape::Type::Pentagon, Shape::Renderer::Painter(_makeMesh(Shape::Type::Pentagon))},
		{Shape::Type::Hexagon, Shape::Renderer::Painter(_makeMesh(Shape::Type::Hexagon))},
		{Shape::Type::Octogon, Shape::Renderer::Painter(_makeMesh(Shape::Type::Octogon))},
		{Shape::Type::Circle, Shape::Renderer::Painter(_makeMesh(Shape::Type::Circle))}};

	std::unordered_map<Shape::Type, Shape::Renderer::ContainerData> Shape::Renderer::containers = {
		{Shape::Type::Triangle, Shape::Renderer::ContainerData()},
		{Shape::Type::Square, Shape::Renderer::ContainerData()},
		{Shape::Type::Pentagon, Shape::Renderer::ContainerData()},
		{Shape::Type::Hexagon, Shape::Renderer::ContainerData()},
		{Shape::Type::Octogon, Shape::Renderer::ContainerData()},
		{Shape::Type::Circle, Shape::Renderer::ContainerData()}};

	Shape::Renderer::Renderer(const std::wstring &p_name) :
		spk::Component(p_name)
	{
		for (auto &[type, painter] : painters)
		{
			painter.setShapeList(&(containers[type].container));
		}
	}

	void Shape::Renderer::onPaintEvent(spk::PaintEvent &p_event)
	{
		for (auto &[type, painter] : painters)
		{
			if (containers[type].container.empty() == false)
			{
				if (containers[type].needUpdate == true)
				{
					painter.validate();
					containers[type].needUpdate = false;
				}
				painter.render();
			}
		}
	}

	Shape::InfoIterator Shape::Renderer::subscribe(const Shape::Type &p_type)
	{
		auto &data = containers[p_type];

		data.needUpdate = true;

		return data.container.emplace(data.container.end(), Info{});
	}

	void Shape::Renderer::remove(const Shape::Type &p_type, const InfoIterator &p_iterator)
	{
		auto &data = containers[p_type];

		data.container.erase(p_iterator);

		data.needUpdate = true;
	}

	void Shape::Renderer::validate(const Shape::Type &p_type)
	{
		containers[p_type].needUpdate = true;
	}
}