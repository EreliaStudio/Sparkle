#include "shape.hpp"

namespace taag
{
	void Shape::Subscriber::_bind()
	{
		if (_type.has_value() == false)
		{
			return;
		}
		_iterator = Renderer::subscribe(_type.value());
	}

	void Shape::Subscriber::_unbind()
	{
		if (_type.has_value() == false)
		{
			GENERATE_ERROR("Can't use an Shape without type");
		}
		Renderer::remove(_type.value(), _iterator);
	}

	void Shape::Subscriber::_pushInfo()
	{
		if (_type.has_value() == false)
		{
			GENERATE_ERROR("Can't use a Shape without type");
		}

		_iterator->model = owner()->transform().model();
		_iterator->color = _color;

		Renderer::validate(_type.value());
	}

	Shape::Subscriber::Subscriber(const std::wstring &p_name) :
		spk::Component(p_name)
	{
		_type.reset();
	}

	void Shape::Subscriber::setType(const Shape::Type &p_type)
	{
		if (_type.has_value() == true)
		{
			_unbind();
		}

		_type = p_type;

		_bind();
	}

	void Shape::Subscriber::setColor(const spk::Color &p_color)
	{
		_color = p_color;
		if (_type.has_value() == true)
		{
			_pushInfo();
		}
	}

	void Shape::Subscriber::start()
	{
		_onEditionContract = owner()->transform().addOnEditionCallback(
			[&]()
			{
				if (_type.has_value() == false)
				{
					GENERATE_ERROR("Can't use an Shape without type");
				}

				_pushInfo();
			});
	}

	void Shape::Subscriber::awake()
	{
		_bind();
	}

	void Shape::Subscriber::sleep()
	{
		_unbind();
	}
}