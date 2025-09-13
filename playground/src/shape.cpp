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

	spk::Lumina::ShaderObjectFactory::Instanciator Shape::Renderer::Painter::_instanciator;

	spk::Lumina::Shader Shape::Renderer::Painter::_createShader()
	{
		spk::Lumina::ShaderObjectFactory::instance()->add(spk::JSON::Object::fromString(LR"({
	"UBO": [
		{
			"name": "CameraUBO",
			"data": {
				"BlockName": "CameraUBO",
				"BindingPoint": 0,
				"Size": 128,
				"Elements": [
					{
						"Name": "viewMatrix",
						"Offset": 0,
						"Size": 64
					},
					{
						"Name": "projectionMatrix",
						"Offset": 64,
						"Size": 64
					}
				]
			}
		}
	],
	"SSBO": [
	]
})"));
		const char *vertexShaderSrc = R"(#version 450

layout(location = 0) in vec2 inPosition;

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform CameraUBO
{
    mat4 viewMatrix;
    mat4 projectionMatrix;
} cameraUBO;

struct Info
{
    mat4 model;
    vec4 color;
};

layout(std430, binding = 3) buffer InfoSSBO
{
    Info object[];
};

void main()
{
    gl_Position = cameraUBO.projectionMatrix * cameraUBO.viewMatrix * object[gl_InstanceID].model * vec4(inPosition, 0.0, 1.0);

    fragColor = object[gl_InstanceID].color;
})";

		const char *fragmentShaderSrc = R"(#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 0) out vec4 outColor;

void main()
{
	outColor = fragColor;
})";

		spk::Lumina::Shader shader(vertexShaderSrc, fragmentShaderSrc);

		shader.addAttribute({0, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector2});

		shader.addUBO(L"CameraUBO", spk::Lumina::ShaderObjectFactory::instance()->ubo(L"CameraUBO"), spk::Lumina::Shader::Mode::Constant);

		spk::OpenGL::ShaderStorageBufferObject infoSSBO = spk::OpenGL::ShaderStorageBufferObject(L"InfoSSBO", 3, 0, 0, 80, 0);
		infoSSBO.dynamicArray().addElement(L"model", 0, 64);
		infoSSBO.dynamicArray().addElement(L"color", 64, 16);

		shader.addSSBO(L"InfoSSBO", infoSSBO, spk::Lumina::Shader::Mode::Attribute);

		return (shader);
	}
	spk::Lumina::Shader Shape::Renderer::Painter::_shader = _createShader();

	void Shape::Renderer::Painter::_prepare(const Mesh &p_mesh)
	{
		const auto &buffer = p_mesh.buffer();

		_bufferSet.layout().clear();
		_bufferSet.indexes().clear();

		_bufferSet.layout() << buffer.vertices;
		_bufferSet.indexes() << buffer.indexes;

		_bufferSet.layout().validate();
		_bufferSet.indexes().validate();
	}

	Shape::Renderer::Painter::Painter(const Mesh &p_mesh) :
		_object(_shader.createObject()),
		_bufferSet(_object.bufferSet()),
		_infoSSBO(_object.ssbo(L"InfoSSBO"))
	{
		_prepare(p_mesh);
	}

	void Shape::Renderer::Painter::render()
	{
		size_t nbInstance = _infoSSBO.dynamicArray().nbElement();

		_object.setNbInstance(nbInstance);
		_object.render();
	}

	void Shape::Renderer::Painter::setShapeList(const spk::SafePointer<const InfoContainer> p_infoContainer)
	{
		_infoContainer = p_infoContainer;
	}

	void Shape::Renderer::Painter::validate()
	{
		if (_infoContainer == nullptr)
		{
			return;
		}

		auto &array = _infoSSBO.dynamicArray();

		array.resize(_infoContainer->size());

		size_t index = 0;
		for (const auto &info : *_infoContainer)
		{
			array[index] = info;
			index++;
		}

		_infoSSBO.validate();
	}

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