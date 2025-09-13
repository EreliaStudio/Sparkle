#pragma once

#include <sparkle.hpp>

namespace taag
{
	class Shape : public spk::Entity
	{
	public:
		enum class Type
		{
			Triangle,
			Square,
			Pentagon,
			Hexagon,
			Octogon,
			Circle
		};

	private:
		using ShapeMesh = spk::TMesh<spk::Vector2>;

		struct Info
		{
			spk::Matrix4x4 model;
			spk::Color color;
		};

		using InfoContainer = std::list<Info>;
		using InfoIterator = InfoContainer::iterator;

		static ShapeMesh _makeMesh(const Shape::Type &p_type)
		{
			static const std::unordered_map<Shape::Type, size_t> nbPointCount = {
				{Shape::Type::Triangle, 3},
				{Shape::Type::Square, 4},
				{Shape::Type::Pentagon, 5},
				{Shape::Type::Hexagon, 6},
				{Shape::Type::Octogon, 8},
				{Shape::Type::Circle, 30}};
			ShapeMesh result;

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

	public:
		struct Renderer : public spk::Component
		{
		private:
			class Painter
			{
			private:
				static inline spk::Lumina::ShaderObjectFactory::Instanciator _instanciator;

				static spk::Lumina::Shader _createShader()
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
				static inline spk::Lumina::Shader _shader = _createShader();

				spk::Lumina::Shader::Object _object;
				spk::OpenGL::BufferSet &_bufferSet;
				spk::OpenGL::ShaderStorageBufferObject &_infoSSBO;

				spk::SafePointer<const InfoContainer> _infoContainer;

				void _prepare(const ShapeMesh &p_mesh)
				{
					const auto &buffer = p_mesh.buffer();

					_bufferSet.layout().clear();
					_bufferSet.indexes().clear();

					_bufferSet.layout() << buffer.vertices;
					_bufferSet.indexes() << buffer.indexes;

					_bufferSet.layout().validate();
					_bufferSet.indexes().validate();
				}

			public:
				Painter(const ShapeMesh &p_mesh) :
					_object(_shader.createObject()),
					_bufferSet(_object.bufferSet()),
					_infoSSBO(_object.ssbo(L"InfoSSBO"))
				{
					_prepare(p_mesh);
				}

				const spk::OpenGL::ShaderStorageBufferObject &infoSSBO() const
				{
					return (_infoSSBO);
				}

				void render()
				{
					size_t nbInstance = _infoSSBO.dynamicArray().nbElement();

					_object.setNbInstance(nbInstance);
					_object.render();
				}

				void setShapeList(const spk::SafePointer<const InfoContainer> p_infoContainer)
				{
					_infoContainer = p_infoContainer;
				}

				void validate()
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
			};

			static inline std::unordered_map<Shape::Type, Painter> painters = {
				{Shape::Type::Triangle, Painter(_makeMesh(Shape::Type::Triangle))},
				{Shape::Type::Square, Painter(_makeMesh(Shape::Type::Square))},
				{Shape::Type::Pentagon, Painter(_makeMesh(Shape::Type::Pentagon))},
				{Shape::Type::Hexagon, Painter(_makeMesh(Shape::Type::Hexagon))},
				{Shape::Type::Octogon, Painter(_makeMesh(Shape::Type::Octogon))},
				{Shape::Type::Circle, Painter(_makeMesh(Shape::Type::Circle))}};

			struct ContainerData
			{
				bool needUpdate;
				InfoContainer container;

				ContainerData() :
					needUpdate(false),
					container()
				{
				}
			};

			static inline std::unordered_map<Shape::Type, ContainerData> containers = {
				{Shape::Type::Triangle, ContainerData()},
				{Shape::Type::Square, ContainerData()},
				{Shape::Type::Pentagon, ContainerData()},
				{Shape::Type::Hexagon, ContainerData()},
				{Shape::Type::Octogon, ContainerData()},
				{Shape::Type::Circle, ContainerData()}};

		public:
			Renderer(const std::wstring &p_name) :
				spk::Component(p_name)
			{
				for (auto &[type, painter] : painters)
				{
					painter.setShapeList(&(containers[type].container));
				}
			}

			void onPaintEvent(spk::PaintEvent &p_event) override
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

			static InfoIterator subscribe(const Shape::Type &p_type)
			{
				auto &data = containers[p_type];

				data.needUpdate = true;

				return data.container.emplace(data.container.end(), Info{});
			}

			static void remove(const Shape::Type &p_type, const InfoIterator &p_iterator)
			{
				auto &data = containers[p_type];

				data.container.erase(p_iterator);

				data.needUpdate = true;
			}

			static void validate(const Shape::Type &p_type)
			{
				containers[p_type].needUpdate = true;
			}
		};

	private:
		class Subscriber : public spk::Component
		{
		private:
			std::optional<Shape::Type> _type;
			spk::Color _color = spk::Color::white;
			InfoIterator _iterator;
			spk::Transform::Contract _onEditionContract;

			void _bind()
			{
				if (_type.has_value() == false)
				{
					return;
				}
				_iterator = Renderer::subscribe(_type.value());
			}

			void _unbind()
			{
				if (_type.has_value() == false)
				{
					GENERATE_ERROR("Can't use an Shape without type");
				}
				Renderer::remove(_type.value(), _iterator);
			}

			void _pushInfo()
			{
				if (_type.has_value() == false)
				{
					GENERATE_ERROR("Can't use a Shape without type");
				}

				_iterator->model = owner()->transform().model();
				_iterator->color = _color;

				Renderer::validate(_type.value());
			}

		public:
			Subscriber(const std::wstring &p_name) :
				spk::Component(p_name)
			{
				_type.reset();
			}

			void setType(const Shape::Type &p_type)
			{
				if (_type.has_value() == true)
				{
					_unbind();
				}

				_type = p_type;

				_bind();
			}

			void setColor(const spk::Color &p_color)
			{
				_color = p_color;
				if (_type.has_value() == true)
				{
					_pushInfo();
				}
			}

			void start() override
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

			void awake() override
			{
				_bind();
			}

			void sleep() override
			{
				_unbind();
			}
		};

	private:
		spk::SafePointer<Subscriber> _subscriber;

	public:
		Shape(const std::wstring &p_name, spk::SafePointer<spk::Entity> p_parent) :
			spk::Entity(p_name, p_parent),
			_subscriber(addComponent<Subscriber>(p_name + L"/Subscriber"))
		{
		}

		void setType(const Type &p_type)
		{
			_subscriber->setType(p_type);
		}

		void setColor(const spk::Color &p_color)
		{
			_subscriber->setColor(p_color);
		}
	};
}