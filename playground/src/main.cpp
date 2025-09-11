#include <sparkle.hpp>

class DebugOverlayManager : public spk::Widget
{
private:
	spk::Profiler::Instanciator _profilerInstanciator;
	spk::DebugOverlay _debugOverlay;

	void _updateOutlineFromLayout()
	{
		const uint32_t labelHeight = _debugOverlay.labelHeight();
		const uint32_t outlineSize = std::max<uint32_t>(1u, labelHeight / 6u);
		_debugOverlay.setFontOutlineSize(outlineSize);
	}

	void _onGeometryChange() override
	{
		spk::Vector2UInt s = geometry().size;
		s.y = std::min(s.y, _debugOverlay.computeMaxHeightPixels());
		_debugOverlay.setGeometry({{0, 0}, s});
		_updateOutlineFromLayout();
	}

	void _onUpdateEvent(spk::UpdateEvent &p_event) override
	{
		size_t fps = 0;
		double fpsMs = 0.0;
		double fpsMinMs = 0.0;
		double fpsMaxMs = 0.0;
		size_t fpsMin = 0;
		size_t fpsMax = 0;

		size_t ups = 0;
		double upsMs = 0.0;
		double upsMinMs = 0.0;
		double upsMaxMs = 0.0;
		size_t upsMin = 0;
		size_t upsMax = 0;

		if (p_event.window != nullptr)
		{
			spk::SafePointer<spk::Window> wnd = p_event.window;
			if (wnd != nullptr)
			{
				fps = wnd->fps();
				fpsMs = wnd->realFpsDuration();
				fpsMinMs = wnd->minFpsDuration();
				fpsMaxMs = wnd->maxFpsDuration();
				fpsMin = wnd->minFps();
				fpsMax = wnd->maxFps();

				ups = wnd->ups();
				upsMs = wnd->realUpsDuration();
				upsMinMs = wnd->minUpsDuration();
				upsMaxMs = wnd->maxUpsDuration();
				upsMin = wnd->minUps();
				upsMax = wnd->maxUps();
			}
		}

		_debugOverlay.setText(0, 0, L"FPS Counter : " + std::to_wstring(fps));
		_debugOverlay.setText(1, 0, L"FPS Duration : " + std::to_wstring(fpsMs) + L" ms");
		_debugOverlay.setText(2, 0, L"Min duration : " + std::to_wstring(fpsMinMs) + L" ms");
		_debugOverlay.setText(3, 0, L"Max duration : " + std::to_wstring(fpsMaxMs) + L" ms");
		_debugOverlay.setText(4, 0, L"Min FPS : " + std::to_wstring(fpsMin));
		_debugOverlay.setText(5, 0, L"Max FPS : " + std::to_wstring(fpsMax));
		_debugOverlay.setText(6, 0, L"");
		_debugOverlay.setText(7, 0, L"UPS Counter : " + std::to_wstring(ups));
		_debugOverlay.setText(8, 0, L"UPS Duration : " + std::to_wstring(upsMs) + L" ms");
		_debugOverlay.setText(9, 0, L"Min duration : " + std::to_wstring(upsMinMs) + L" ms");
		_debugOverlay.setText(10, 0, L"Max duration : " + std::to_wstring(upsMaxMs) + L" ms");
		_debugOverlay.setText(11, 0, L"Min UPS : " + std::to_wstring(upsMin));
		_debugOverlay.setText(12, 0, L"Max UPS : " + std::to_wstring(upsMax));

		p_event.requestPaint();
	}

public:
	DebugOverlayManager(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent),
		_debugOverlay(p_name + L"/Overlay", this)
	{
		_debugOverlay.setMaxGlyphSize(30);
		_debugOverlay.setFontOutlineSize(4);
		_debugOverlay.setFontOutlineSharpness(1);
		_debugOverlay.setFontColor(spk::Color::white, spk::Color::black);

		_debugOverlay.configureRows(20, 1);
		_debugOverlay.activate();
	}
};

class ShaderDebugWidget : public spk::Widget
{
public:
	ShaderDebugWidget(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent)
	{
		spk::OpenGL::SamplerObject sampler;
		spk::OpenGL::UniformBufferObject ubo;
		spk::OpenGL::ShaderStorageBufferObject ssbo;

		_shader.addSampler(L"sampler", sampler, spk::Lumina::Shader::Mode::Constant);
		_shader.addUBO(L"ubo", ubo, spk::Lumina::Shader::Mode::Constant);
		_shader.addSSBO(L"ssbo", ssbo, spk::Lumina::Shader::Mode::Constant);

		auto &samplerRef = _shader.sampler(L"sampler");
		auto &uboRef = _shader.ubo(L"ubo");
		auto &ssboRef = _shader.ssbo(L"ssbo");

		(void)samplerRef;
		(void)uboRef;
		(void)ssboRef;

		spk::JSON::Object desc;
		desc.setAsObject();
		auto &array = desc[L"Sampler"];
		array.setAsArray();
		auto &samplerObj = array.append();
		samplerObj[L"name"].set(std::wstring(L"factorySampler"));
		samplerObj[L"data"].setAsObject();

		spk::Lumina::ShaderObjectFactory::instance().add(desc);
		auto &factorySampler = spk::Lumina::ShaderObjectFactory::instance().sampler(L"factorySampler");
		(void)factorySampler;
	}

private:
	spk::Lumina::Shader _shader;
};

int main()
{
	spk::GraphicalApplication app;
	auto window = app.createWindow(L"Playground", {{0, 0}, {800, 600}});
	window->setUpdateTimer(0);

	DebugOverlayManager debugOverlay(L"DebugOverlay", window->widget());
	debugOverlay.setGeometry({0, 0}, window->geometry().size);
	debugOverlay.setLayer(100);
	debugOverlay.activate();

	ShaderDebugWidget shaderWidget(L"ShaderWidget", window->widget());
	shaderWidget.activate();

	return app.run();
}
