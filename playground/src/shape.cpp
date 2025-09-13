#include "shape.hpp"

namespace taag
{
	Shape::Mesh Shape::_makeMesh(const Shape::Type &p_type)
	{
		static const std::unordered_map<Shape::Type, size_t> nbPointCount = {
			{Shape::Type::Triangle, 3},
			{Shape::Type::Square, 4},
			{Shape::Type::Pentagon, 5},
			{Shape::Type::Hexagon, 6},
			{Shape::Type::Octogon, 8},
			{Shape::Type::Circle, 30}};
		Mesh result;

		std::vector<spk::Vector2> vertices;
		size_t nbPoint = nbPointCount.at(p_type);
		const float step = (2.0f * M_PI) / static_cast<float>(nbPoint);
		for (int i = 0; i < nbPoint; ++i)
		{
			float angle = step * static_cast<float>(i);
			vertices.emplace_back(std::cos(angle) / 2.0f, std::sin(angle) / 2.0f);
		}
		result.addShape(vertices);

		return (result);
	}

	Shape::Shape(const std::wstring &p_name, spk::SafePointer<spk::Entity> p_parent) :
		spk::Entity(p_name, p_parent),
		_subscriber(addComponent<Subscriber>(p_name + L"/Subscriber"))
	{
	}

	void Shape::setType(const Type &p_type)
	{
		_subscriber->setType(p_type);
	}

	void Shape::setColor(const spk::Color &p_color)
	{
		_subscriber->setColor(p_color);
	}
}