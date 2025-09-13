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
		using Mesh = spk::TMesh<spk::Vector2>;

		struct Info
		{
			spk::Matrix4x4 model;
			spk::Color color;
		};

		using InfoContainer = std::list<Info>;
		using InfoIterator = InfoContainer::iterator;

		static Mesh _makeMesh(const Shape::Type &p_type);

	public:
		struct Renderer : public spk::Component
		{
		private:
			class Painter
			{
			private:
				static spk::Lumina::ShaderObjectFactory::Instanciator _instanciator;
				static spk::Lumina::Shader _createShader();
				static spk::Lumina::Shader _shader;

				spk::Lumina::Shader::Object _object;
				spk::OpenGL::BufferSet &_bufferSet;
				spk::OpenGL::ShaderStorageBufferObject &_infoSSBO;

				spk::SafePointer<const InfoContainer> _infoContainer;

				void _prepare(const Mesh &p_mesh);

			public:
				Painter(const Mesh &p_mesh);

				void render();

				void setShapeList(const spk::SafePointer<const InfoContainer> p_infoContainer);

				void validate();
			};

			static std::unordered_map<Shape::Type, Painter> painters;

			struct ContainerData
			{
				bool needUpdate = false;
				InfoContainer container;
			};

			static std::unordered_map<Shape::Type, ContainerData> containers;

		public:
			Renderer(const std::wstring &p_name);

			void onPaintEvent(spk::PaintEvent &p_event) override;

			static InfoIterator subscribe(const Shape::Type &p_type);
			static void remove(const Shape::Type &p_type, const InfoIterator &p_iterator);
			static void validate(const Shape::Type &p_type);
		};

	private:
		class Subscriber : public spk::Component
		{
		private:
			std::optional<Shape::Type> _type;
			spk::Color _color = spk::Color::white;
			InfoIterator _iterator;
			spk::Transform::Contract _onEditionContract;

			void _bind();
			void _unbind();

			void _pushInfo();

		public:
			Subscriber(const std::wstring &p_name);

			void setType(const Shape::Type &p_type);
			void setColor(const spk::Color &p_color);

			void start() override;
			void awake() override;
			void sleep() override;
		};

	private:
		spk::SafePointer<Subscriber> _subscriber;

	public:
		Shape(const std::wstring &p_name, spk::SafePointer<spk::Entity> p_parent);

		void setType(const Type &p_type);
		void setColor(const spk::Color &p_color);
	};
}