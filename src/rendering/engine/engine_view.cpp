#include "rendering/engine/engine_view.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

#include "exception.hpp"
#include "rendering/engine/engine.hpp"

namespace spk
{
	bool EngineView::CaptureContext::isLayerCompatible(const Layers &candidate) const
	{
		if (!layers)
		{
			return true;
		}
		return std::ranges::any_of(candidate, [this](const std::string &layer) {
			return std::ranges::find(*layers, layer) != layers->end();
		});
	}

	void EngineView::Contributor::setLayers(Layers layers)
	{
		_layers = std::move(layers);
	}

	const EngineView::Layers &EngineView::Contributor::layers() const noexcept
	{
		return _layers;
	}

	bool EngineView::Contributor::canContribute() const
	{
		return _canContribute();
	}

	EngineView::EngineView(Engine &engine, std::string name, std::uint64_t identity,
						   CameraComponent3D *camera, Vector2UInt resolution, std::optional<Order> order) :
		_engine(&engine),
		_name(std::move(name)),
		_identity(identity),
		_camera(camera),
		_resolution(resolution),
		_order(order)
	{
	}

	EngineView::CaptureContext EngineView::_capture() const
	{
		CaptureContext context;
		context.name = _name;
		context.identity = _identity;
		context.resolution = _resolution;
		context.layers = _layers;
		context.backgroundColor = _backgroundColor;
		context.exposure = _exposure;
		context.targetFPS = _targetFPS;
		context.detachedOutputPolicy = _detachedOutputPolicy;
		context.fallbackTexture = _fallbackTexture;
		context.projectionMatrix = _camera->projectionMatrix();
		context.viewMatrix = _camera->viewMatrix();
		context.inverseViewMatrix = _camera->inverseViewMatrix();
		context.viewProjectionMatrix = context.projectionMatrix * context.viewMatrix;
		return context;
	}

	const std::string &EngineView::name() const noexcept { return _name; }
	std::uint64_t EngineView::identity() const noexcept { return _identity; }
	CameraComponent3D *EngineView::camera() const noexcept { return _camera; }
	void EngineView::setCamera(CameraComponent3D *camera) noexcept { _camera = camera; }
	void EngineView::detachCamera() noexcept { _camera = nullptr; }
	Vector2UInt EngineView::resolution() const noexcept { return _resolution; }

	void EngineView::setResolution(Vector2UInt resolution)
	{
		if (resolution.x == 0 || resolution.y == 0)
		{
			throw spk::Exception("EngineView [" + _name + "]: resolution must be nonzero");
		}
		_resolution = resolution;
	}

	std::optional<EngineView::Order> EngineView::order() const noexcept { return _order; }

	void EngineView::setOrder(std::optional<Order> order)
	{
		_engine->setViewOrder(*this, order);
	}

	void EngineView::setLayers(Layers layers)
	{
		_layers = std::move(layers);
	}

	void EngineView::clearLayerFilter() noexcept
	{
		_layers.reset();
	}

	const std::optional<EngineView::Layers> &EngineView::layers() const noexcept { return _layers; }
	EngineView::DetachedOutputPolicy EngineView::detachedOutputPolicy() const noexcept { return _detachedOutputPolicy; }

	void EngineView::setDetachedOutputPolicy(DetachedOutputPolicy policy)
	{
		switch (policy)
		{
		case DetachedOutputPolicy::Invalid:
		case DetachedOutputPolicy::KeepLastFrame:
		case DetachedOutputPolicy::DefaultTexture:
			_detachedOutputPolicy = policy;
			return;
		}
		throw spk::Exception("EngineView [" + _name + "]: invalid detached output policy");
	}

	void EngineView::setFallbackTexture(Texture::Handle texture) noexcept { _fallbackTexture = std::move(texture); }
	const Texture::Handle &EngineView::fallbackTexture() const noexcept { return _fallbackTexture; }

	void EngineView::setBackgroundColor(RGB color)
	{
		const auto valid = [](float value) { return std::isfinite(value) && value >= 0.0f && value <= 1.0f; };
		if (!valid(color.r) || !valid(color.g) || !valid(color.b))
		{
			throw spk::Exception("EngineView [" + _name + "]: invalid background color");
		}
		_backgroundColor = color;
	}

	RGB EngineView::backgroundColor() const noexcept { return _backgroundColor; }

	void EngineView::setExposure(float exposure)
	{
		if (!std::isfinite(exposure) || exposure <= 0.0f)
		{
			throw spk::Exception("EngineView [" + _name + "]: exposure must be positive and finite");
		}
		_exposure = exposure;
	}

	float EngineView::exposure() const noexcept { return _exposure; }

	void EngineView::setTargetFPS(float fps)
	{
		if (!std::isfinite(fps) || fps <= 0.0f)
		{
			throw spk::Exception("EngineView [" + _name + "]: target FPS must be positive and finite");
		}
		_targetFPS = fps;
	}

	float EngineView::targetFPS() const noexcept { return _targetFPS; }
}
