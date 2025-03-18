#include "playground.hpp"

class TestWidget : public spk::Widget
{
private:
	spk::OpenGL::Program _program;
	spk::OpenGL::BufferSet _bufferSet;
	spk::OpenGL::ShaderStorageBufferObject _ssbo;

	void _initProgram()
	{
		const std::string vertexShaderSource = R"(#version 450 core

			layout(location = 0) in vec2 position;
		
			layout (std430, binding = 0) buffer ssboType
			{
				vec2 offset[];
			} ssbo;

			void main()
			{
				gl_Position = vec4(position + ssbo.offset[gl_InstanceID], 0.0, 1.0);
			}
		)";

		const std::string fragmentShaderSource = R"(#version 450 core

			out vec4 color;

			void main()
			{
				color = vec4(1.0, 0.0, 0.0, 1.0);
			}
		)";

		_program = spk::OpenGL::Program(vertexShaderSource, fragmentShaderSource);

		_bufferSet = spk::OpenGL::BufferSet({
			{0, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector2}
		});

		_bufferSet.clear();
		_bufferSet.layout() << spk::Vector2(-0.5f, -0.5f);
		_bufferSet.layout() << spk::Vector2( 0.5f, -0.5f);
		_bufferSet.layout() << spk::Vector2( 0.0f,  0.5f);
		_bufferSet.indexes() << 0 << 1 << 2;
		_bufferSet.validate();

		_ssbo = spk::OpenGL::ShaderStorageBufferObject(L"ssboType", 0, 0, 0, sizeof(spk::Vector2), 0);
	
		_ssbo.dynamicArray().addElement(L"offset", 0, sizeof(spk::Vector2));
		_ssbo.dynamicArray().resize(4);
		_ssbo.dynamicArray() = {
			spk::Vector2(-0.5f, -0.5f), 
			spk::Vector2( 0.5f, -0.5f), 
			spk::Vector2(-0.5f,  0.5f), 
			spk::Vector2( 0.5f,  0.5f)
		};
		_ssbo.validate();
	}

	void _onGeometryChange() override
	{

	}

	void _onPaintEvent(spk::PaintEvent& p_event) override
	{
		_program.activate();

		_bufferSet.activate();
		_ssbo.activate();

		_program.render(_bufferSet.indexes().nbIndexes(), _ssbo.dynamicArray().nbElement());

		_ssbo.deactivate();
		_bufferSet.deactivate();

		_program.deactivate();
	}

public:
	TestWidget(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		Widget(p_name, p_parent)
	{
		_initProgram();
	}
};

int main()
{
	spk::GraphicalApplication app = spk::GraphicalApplication();

	spk::SafePointer<spk::Window> win = app.createWindow(L"Playground", {800, 800});
	
	TestWidget testWidget = TestWidget(L"Debug overlay", win->widget());
	testWidget.setGeometry(0, win->geometry().size);
	testWidget.activate();

	return (app.run());
}