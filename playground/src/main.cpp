#include "playground.hpp"

class TestComponent : public spk::Component
{
private:
	spk::OpenGL::Program _program = spk::OpenGL::Program(
R"(#version 450

layout(location = 0) in vec3 modelPosition;

layout(std140, binding = 0) uniform modelInformations {
    vec3 offsets[3];
	vec4 color;
};

void main()
{
	vec3 instanceOffset = offsets[gl_InstanceID];
    gl_Position = vec4(modelPosition + instanceOffset, 1.0);
})",
R"(#version 450

layout(location = 0) out vec4 outputColor;

layout(std140, binding = 0) uniform modelInformations {
    vec3 offsets[3];
	vec4 color;
};

void main()
{
	outputColor = color;
})");

	spk::OpenGL::BufferSet _bufferSet;

	spk::OpenGL::UniformBufferObject _modelInformations;

	void onPaintEvent(spk::PaintEvent& p_event)
	{
		struct Vertex
		{
			spk::Vector3 position;
			spk::Color color;
		};

		_bufferSet.layout().clear();
		_bufferSet.indexes().clear();

		_bufferSet.layout() << Vertex{.position = spk::Vector3( 0.0f,  0.5f, 0.0f), .color = spk::Color::red};		
		_bufferSet.layout() << Vertex{.position = spk::Vector3(-0.5f, -0.5f, 0.0f), .color = spk::Color::green};	
		_bufferSet.layout() << Vertex{.position = spk::Vector3( 0.5f, -0.5f, 0.0f), .color = spk::Color::blue};

		_bufferSet.indexes() << std::array<unsigned int, 3>{
			0u, 1u, 2u
		};

		_modelInformations["offsets"][0] = spk::Vector3(0, 0.5f, 0);
		_modelInformations["offsets"][1] = spk::Vector3(-0.5f, -0.5f, 0);
		_modelInformations["offsets"][2] = spk::Vector3(0.5f, -0.5f, 0);
		_modelInformations["color"] = spk::Color::green;
		_modelInformations.validate();

		_bufferSet.layout().validate();
		_bufferSet.indexes().validate();

		_program.activate();

		_bufferSet.activate();

		_modelInformations.activate();

		size_t nbInstance = 3;

		_program.render(_bufferSet.indexes().nbIndexes(), nbInstance);

		_modelInformations.deactivate();

		_bufferSet.deactivate();

		_program.deactivate();
	}

public:
	TestComponent() :
		spk::Component(L"TestComponent"),
		_modelInformations("modelInformations", 0, 64)
	{
		_bufferSet.layout().addAttribute(0, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector3);
		_bufferSet.layout().addAttribute(1, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector4);

		_modelInformations.addElement("offsets", 0, sizeof(spk::Vector3), 3);
		_modelInformations.addElement("color", 48, sizeof(spk::Color));
	}
};

int main()
{
	spk::GraphicalApplication app = spk::GraphicalApplication();

	spk::SafePointer<spk::Window> win = app.createWindow(L"Playground", {{0, 0}, {840, 680}});

	spk::GameEngine engine;

	spk::Entity player(L"Player");
	player.addComponent<TestComponent>();
	
	spk::Entity camera(L"Camera", &player);

	spk::Entity cube(L"Cube");

	engine.addEntity(&player);
	engine.addEntity(&cube);

	spk::GameEngineWidget gameEngineWidget = spk::GameEngineWidget(L"Engine widget", win->widget());
	gameEngineWidget.setGeometry(win->geometry());
	gameEngineWidget.setGameEngine(&engine);
	gameEngineWidget.activate();

	return (app.run());
}