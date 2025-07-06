#include <sparkle.hpp>

class TestWidget : public spk::Widget
{
private:	
	spk::ColorRenderer _colorRenderer;
	spk::OpenGL::FrameBufferObject _fbo;

	spk::TextureRenderer _textureRenderer;

	void _onGeometryChange() override
	{
		_colorRenderer.clear();
		_colorRenderer.prepareSquare({{0, 0}, geometry().size}, layer());
		_colorRenderer.validate();

		_fbo.resize(geometry().size);

		_textureRenderer.clear();
		_textureRenderer.setTexture(_fbo.attachment(L"outputColor")->bindedTexture());
		_textureRenderer.prepare(geometry(), {{0.0f, 0.0f}, {1.0f, 1.0f}}, layer());
		_textureRenderer.validate();
	}

	void _onPaintEvent(spk::PaintEvent& p_event)
	{
		_fbo.activate();
		
		_colorRenderer.render();

		spk::SafePointer<const spk::OpenGL::FrameBufferObject::Attachment> attachment = _fbo.attachment(L"outputColor");

		spk::Texture savedTexture = attachment->save();

		savedTexture.saveAsPng("result.png");

		_fbo.deactivate();

		_textureRenderer.render();
	}

public:
	TestWidget(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent)
	{
		_colorRenderer.setColor(spk::Color::red);

		_fbo.addAttachment(L"outputColor", 0, spk::OpenGL::FrameBufferObject::Attachment::Type::Color);
	}
};

int main()
{
	spk::GraphicalApplication app;

	spk::SafePointer<spk::Window> win = app.createWindow(L"Erelia", {{0, 0}, {800, 600}});

	TestWidget testWidget(L"Test Widget", win->widget());
	testWidget.setGeometry(win->geometry());
	testWidget.activate();

	return (app.run());
}