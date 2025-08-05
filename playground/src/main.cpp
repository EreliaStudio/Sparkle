#include <sparkle.hpp>

class MeshTestWidget : public spk::Screen
{
private:
	spk::ObjMesh _mesh;
	spk::ObjMeshRenderer _renderer;

public:
	MeshTestWidget(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Screen(p_name, p_parent)
	{
		_mesh.loadFromFile("resources/obj/cube.obj");

		auto &cameraUBO = spk::Lumina::ShaderObjectFactory::instance()->ubo(L"CameraUBO");
		cameraUBO.layout()[L"viewMatrix"] = spk::Matrix4x4::identity();
		cameraUBO.layout()[L"projectionMatrix"] = spk::Matrix4x4::identity();
		cameraUBO.validate();

		auto &transformUBO = spk::Lumina::ShaderObjectFactory::instance()->ubo(L"TransformUBO");
		transformUBO.layout()[L"modelMatrix"] = spk::Matrix4x4::identity();
		transformUBO.validate();

		_renderer.painter().clear();
		_renderer.painter().prepare(_mesh);
		_renderer.painter().validate();
	}

protected:
	void _onPaintEvent(spk::PaintEvent &) override
	{
		_renderer.painter().render();
	}
};

int main()
{
	spk::GraphicalApplication app;
	auto win = app.createWindow(L"Playground", {{0, 0}, {800, 600}});

	MeshTestWidget widget(L"MeshTest", win->widget());
	widget.setGeometry({0, 0}, win->geometry().size);
	widget.activate();

	return app.run();
}