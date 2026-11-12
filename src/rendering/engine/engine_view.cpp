#include "rendering/engine/engine_view.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

#include "exception.hpp"
#include "rendering/engine/engine.hpp"

namespace spk
{
	std::shared_ptr<EngineView::Output::Targets> EngineView::Output::acquireTargets(Vector2UInt resolution)
	{
		std::lock_guard lock(_mutex);
		for (const auto &target : _targets)
		{
			if (target.use_count() == 1 && target->resolution == resolution)
			{
				return target;
			}
		}
		std::erase_if(_targets, [&resolution](const auto &target) {
			return target.use_count() == 1 && target->resolution != resolution;
		});
		using Attachment = Framebuffer::RenderAttachment;
		const auto hdr = Attachment::create2D(resolution, Texture::Format::RGBA16F);
		const auto depth = Attachment::create2D(resolution, Texture::Format::Depth24Stencil8);
		const auto accumulation = Attachment::create2D(resolution, Texture::Format::RGBA16F);
		const auto revealage = Attachment::create2D(resolution, Texture::Format::GreyLevel);
		Framebuffer opaque(resolution, std::vector<Texture::Handle>{hdr.texture()}, depth.texture());
		Framebuffer transparent(resolution, std::vector<Texture::Handle>{accumulation.texture(), revealage.texture()}, depth.texture());
		auto target = std::make_shared<Targets>(Targets{resolution, hdr.texture(), depth.texture(), accumulation.texture(), revealage.texture(), opaque.handle(), transparent.handle()});
		_targets.push_back(target);
		return target;
	}

	void EngineView::Output::_discard(Window::Surface *surface) noexcept
	{
		std::lock_guard lock(_mutex);
		_frames.erase(surface);
	}

	void EngineView::Output::publish(Window::Surface &surface, Texture::Handle texture)
	{
		if (!texture)
		{
			throw spk::Exception("Cannot publish an empty EngineView output");
		}
		std::lock_guard lock(_mutex);
		auto &frame = _frames[&surface];
		if (!frame.destruction)
		{
			const std::weak_ptr<Output> weak = weak_from_this();
			frame.destruction = surface.subscribeToDestruction([weak, key = &surface] {
				if (const auto output = weak.lock())
				{
					output->_discard(key);
				}
			});
		}
		frame.texture = std::move(texture);
	}

	Texture::Handle EngineView::Output::latest(Window::Surface &surface) const
	{
		std::lock_guard lock(_mutex);
		if (_detached && _policy == DetachedOutputPolicy::Invalid)
		{
			return {};
		}
		if (_detached && _policy == DetachedOutputPolicy::DefaultTexture)
		{
			return _fallback;
		}
		const auto it = _frames.find(&surface);
		if (_detached && _policy == DetachedOutputPolicy::KeepLastFrame && it == _frames.end())
		{
			throw spk::Exception("EngineView: no last valid frame on this Surface");
		}
		return it == _frames.end() ? Texture::Handle{} : it->second.texture;
	}

	Texture::Handle EngineView::Output::latestFrame(Window::Surface &surface) const
	{
		std::lock_guard lock(_mutex);
		const auto it = _frames.find(&surface);
		return it == _frames.end() ? Texture::Handle{} : it->second.texture;
	}

	Texture::Handle EngineView::Source::onSurface(Window::Surface &surface) const
	{
		if (kind == Kind::DefaultTexture)
		{
			if (texture)
			{
				return texture;
			}
			throw spk::Exception("EngineView: default output texture is missing");
		}
		const auto frame = output ? output->latestFrame(surface) : Texture::Handle{};
		if (kind == Kind::KeepLastFrame && frame)
		{
			return frame;
		}
		if (kind == Kind::Produced && frame && frame.identifier() == texture.identifier())
		{
			return texture;
		}
		throw spk::Exception("EngineView: requested output was not produced on this Surface");
	}

	void EngineView::Output::setDetached(bool detached, DetachedOutputPolicy policy, Texture::Handle fallback)
	{
		std::lock_guard lock(_mutex);
		_detached = detached;
		_policy = policy;
		_fallback = std::move(fallback);
	}

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

	EngineView::EngineView(Engine &engine, std::string name, std::uint64_t identity, CameraComponent3D *camera, Vector2UInt resolution, std::optional<Order> order) :
		_engine(&engine),
		_name(std::move(name)),
		_identity(identity),
		_camera(camera),
		_resolution(resolution),
		_order(order)
	{
		_output->setDetached(_camera == nullptr, _detachedOutputPolicy, _fallbackTexture);
	}

	EngineView::CaptureContext EngineView::_capture() const
	{
		CaptureContext context;
		context.name = _name;
		context.output = _output;
		context.identity = _identity;
		context.active = _camera != nullptr;
		context.resolution = _resolution;
		context.layers = _layers;
		context.backgroundColor = _backgroundColor;
		context.ambientRGB = _ambientLight.contribution();
		context.exposure = _exposure;
		context.targetFPS = _targetFPS;
		context.detachedOutputPolicy = _detachedOutputPolicy;
		context.fallbackTexture = _fallbackTexture;
		if (_camera != nullptr)
		{
			context.projectionMatrix = _camera->projectionMatrix();
			context.viewMatrix = _camera->viewMatrix();
			context.inverseViewMatrix = _camera->inverseViewMatrix();
			context.viewProjectionMatrix = context.projectionMatrix * context.viewMatrix;
		}
		return context;
	}

	const std::string &EngineView::name() const noexcept
	{
		return _name;
	}
	std::uint64_t EngineView::identity() const noexcept
	{
		return _identity;
	}
	std::shared_ptr<EngineView::Output> EngineView::output() const noexcept
	{
		return _output;
	}

	CameraComponent3D *EngineView::camera() const noexcept
	{
		return _camera;
	}
	void EngineView::setCamera(CameraComponent3D *camera) noexcept
	{
		_camera = camera;
		_output->setDetached(_camera == nullptr, _detachedOutputPolicy, _fallbackTexture);
	}
	void EngineView::detachCamera() noexcept
	{
		setCamera(nullptr);
	}
	Vector2UInt EngineView::resolution() const noexcept
	{
		return _resolution;
	}

	void EngineView::setResolution(Vector2UInt resolution)
	{
		if (resolution.x == 0 || resolution.y == 0)
		{
			throw spk::Exception("EngineView [" + _name + "]: resolution must be nonzero");
		}
		_resolution = resolution;
	}

	std::optional<EngineView::Order> EngineView::order() const noexcept
	{
		return _order;
	}

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

	const std::optional<EngineView::Layers> &EngineView::layers() const noexcept
	{
		return _layers;
	}
	EngineView::DetachedOutputPolicy EngineView::detachedOutputPolicy() const noexcept
	{
		return _detachedOutputPolicy;
	}

	void EngineView::setDetachedOutputPolicy(DetachedOutputPolicy policy)
	{
		switch (policy)
		{
		case DetachedOutputPolicy::Invalid:
		case DetachedOutputPolicy::KeepLastFrame:
		case DetachedOutputPolicy::DefaultTexture:
			_detachedOutputPolicy = policy;
			_output->setDetached(_camera == nullptr, _detachedOutputPolicy, _fallbackTexture);
			return;
		}
		throw spk::Exception("EngineView [" + _name + "]: invalid detached output policy");
	}

	void EngineView::setFallbackTexture(Texture::Handle texture) noexcept
	{
		_fallbackTexture = std::move(texture);
		_output->setDetached(_camera == nullptr, _detachedOutputPolicy, _fallbackTexture);
	}
	const Texture::Handle &EngineView::fallbackTexture() const noexcept
	{
		return _fallbackTexture;
	}

	void EngineView::setBackgroundColor(RGB color)
	{
		const auto valid = [](float value) {
			return std::isfinite(value) && value >= 0.0f && value <= 1.0f;
		};
		if (!valid(color.r) || !valid(color.g) || !valid(color.b))
		{
			throw spk::Exception("EngineView [" + _name + "]: invalid background color");
		}
		_backgroundColor = color;
	}

	RGB EngineView::backgroundColor() const noexcept
	{
		return _backgroundColor;
	}

	AmbientLight &EngineView::ambientLight() noexcept
	{
		return _ambientLight;
	}

	const AmbientLight &EngineView::ambientLight() const noexcept
	{
		return _ambientLight;
	}

	void EngineView::setExposure(float exposure)
	{
		if (!std::isfinite(exposure) || exposure <= 0.0f)
		{
			throw spk::Exception("EngineView [" + _name + "]: exposure must be positive and finite");
		}
		_exposure = exposure;
	}

	float EngineView::exposure() const noexcept
	{
		return _exposure;
	}

	void EngineView::setTargetFPS(float fps)
	{
		if (!std::isfinite(fps) || fps <= 0.0f)
		{
			throw spk::Exception("EngineView [" + _name + "]: target FPS must be positive and finite");
		}
		_targetFPS = fps;
	}

	float EngineView::targetFPS() const noexcept
	{
		return _targetFPS;
	}
}
