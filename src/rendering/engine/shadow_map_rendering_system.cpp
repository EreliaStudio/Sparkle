#include "shadow_map/rendering_system.hpp"

#include <GL/glew.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "core/context/render_context.hpp"
#include "engine/transform3d.hpp"
#include "exception.hpp"
#include "graphics/internal/resource.hpp"
#include "graphics/opengl/cube_map.hpp"
#include "graphics/opengl/framebuffer.hpp"
#include "graphics/opengl/program.hpp"
#include "graphics/opengl/state.hpp"
#include "graphics/opengl/uniform_buffer.hpp"
#include "light/channel.hpp"
#include "rendering/render_command.hpp"
#include "shadow_map/contributor.hpp"
#include "shadow_map/projection.hpp"
#include "shadow_map/point_budget.hpp"

namespace spk::ShadowMap
{
	namespace
	{
		using PointKey = std::tuple<const void *, std::uint64_t, std::uint64_t, std::uint64_t>;

		struct PointImagePlan
		{
			std::set<PointKey> approved;
			std::set<PointKey> consumed;
		};

		struct Caster
		{
			Mesh mesh;
			Matrix4x4 transform;
		};

		[[nodiscard]] std::uint64_t mix(std::uint64_t seed, std::uint64_t value) noexcept
		{
			return seed ^ (value + 0x9e3779b97f4a7c15ull + (seed << 6) + (seed >> 2));
		}

		[[nodiscard]] std::uint64_t signature(const std::vector<Caster> &casters)
		{
			std::uint64_t result = mix(0x46a9c3bbull, casters.size());
			for (const auto &caster : casters)
			{
				const auto &mesh = caster.mesh.layout();
				result = mix(result, mesh.vertexBuffer().identifier());
				result = mix(result, mesh.vertexBuffer().generation());
				result = mix(result, mesh.indexBuffer().identifier());
				result = mix(result, mesh.indexBuffer().generation());
				for (std::size_t column = 0; column < 4; ++column)
				{
					for (std::size_t row = 0; row < 4; ++row)
					{
						result = mix(result, std::bit_cast<std::uint32_t>(caster.transform[column][row]));
					}
				}
			}
			return result;
		}

		class TargetScope final
		{
		private:
			OpenGL::State::Scope _state;
			RenderContext &_context;
			Window::Surface::RenderTarget _previous;

		public:
			explicit TargetScope(RenderContext &context) :
				_state(context),
				_context(context),
				_previous(context.activeRenderTarget)
			{
			}

			~TargetScope()
			{
				_context.activeRenderTarget = _previous;
			}
		};

		[[nodiscard]] Framebuffer newDepthTarget(std::uint32_t resolution)
		{
			return Framebuffer({resolution, resolution}, {}, Texture::Format::Depth32F);
		}

		struct CubeTargets
		{
			Texture::Handle depth;
			std::array<Framebuffer::Handle, 6> faces;
		};

		[[nodiscard]] CubeTargets newCubeTargets(std::uint32_t resolution)
		{
			CubeMap cube(resolution);
			CubeTargets result;
			result.depth = cube.handle();
			for (std::size_t face = 0; face < result.faces.size(); ++face)
			{
				Framebuffer target(
					{resolution, resolution},
					std::vector<Framebuffer::RenderAttachment>{},
					Framebuffer::RenderAttachment(result.depth, static_cast<CubeMap::Face>(face)));
				result.faces[face] = target.handle();
			}
			return result;
		}

		[[nodiscard]] Program &depthProgram()
		{
			static Program program = [] {
				Program value(
					std::string(resources::text("shaders/shadow_depth.vert.glsl")),
					std::string(resources::text("shaders/shadow_depth.frag.glsl")));
				value.bindUniformBlock("ShadowCamera", 4);
				value.bindUniformBlock("ShadowModel", 5);
				return value;
			}();
			return program;
		}

		void renderDepth(
			RenderContext &context,
			const Framebuffer::Handle &framebuffer,
			const Matrix4x4 &projection,
			const std::vector<Caster> &casters,
			bool clear)
		{
			TargetScope scope(context);
			const Window::Surface::RenderTarget target(*context.targetSurface, framebuffer);
			target.activate(context);
			context.activeRenderTarget = target;
			const auto size = target.size();
			OpenGL::State::Viewport{{0, 0, static_cast<int>(size.x), static_cast<int>(size.y)}}.apply(context);
			OpenGL::State::Scissor{.enabled = false}.apply(context);
			OpenGL::State::ColorOutput::writable(context, false).apply(context);
			OpenGL::State::DepthTest{}.apply(context);
			OpenGL::State::Blend::disabled(context).apply(context);
			auto rasterization = OpenGL::State::Rasterization{};
			rasterization.polygonOffsetEnabled = true;
			rasterization.polygonOffsetFactor = 1.5f;
			rasterization.polygonOffsetUnits = 2.0f;
			rasterization.apply(context);
			if (clear)
			{
				OpenGL::State::Clear::depth(context);
			}
			if (casters.empty())
			{
				if (const GLenum error = glGetError(); error != GL_NO_ERROR)
				{
					throw spk::Exception("Shadow depth rendering failed with OpenGL error " + std::to_string(error));
				}
				return;
			}
			UniformBuffer cameraBuffer(4, sizeof(Matrix4x4));
			cameraBuffer.setData(projection);
			cameraBuffer.validate();
			cameraBuffer.activate(context);
			Program &program = depthProgram();
			program.activate(context);
			for (const Caster &caster : casters)
			{
				UniformBuffer modelBuffer(5, sizeof(Matrix4x4));
				modelBuffer.setData(caster.transform);
				modelBuffer.validate();
				modelBuffer.activate(context);
				caster.mesh.layout().activate(context);
				program.render(Program::Primitive::Triangles, caster.mesh.indexType(), 0, caster.mesh.indexCount());
			}
			if (const GLenum error = glGetError(); error != GL_NO_ERROR)
			{
				throw spk::Exception("Shadow depth rendering failed with OpenGL error " + std::to_string(error));
			}
		}

		void copyDepth(RenderContext &context, const Framebuffer::Handle &source, const Framebuffer::Handle &destination)
		{
			TargetScope scope(context);
			source.activate(context);
			GLint read = 0;
			glGetIntegerv(GL_FRAMEBUFFER_BINDING, &read);
			destination.activate(context);
			GLint draw = 0;
			glGetIntegerv(GL_FRAMEBUFFER_BINDING, &draw);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(read));
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(draw));
			const GLsizei size = static_cast<GLsizei>(destination->size().x);
			glBlitFramebuffer(0, 0, size, size, 0, 0, size, size, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
			if (const GLenum error = glGetError(); error != GL_NO_ERROR)
			{
				throw spk::Exception("Shadow depth copy failed with OpenGL error " + std::to_string(error));
			}
		}
	}

	void PointBudget::add(Request request)
	{
		if (request.system == nullptr || request.registrationOrder == 0 ||
			request.admission == 0 || !request.limit || !request.needsRebuild)
		{
			throw spk::Exception("Invalid PointLight shadow budget request");
		}
		_requests.push_back(std::move(request));
	}

	void PointBudget::prepare(RenderContext &context) const
	{
		if (context.imageState.contains(this))
		{
			return;
		}
		auto plan = std::make_shared<PointImagePlan>();
		std::vector<const Request *> pending;
		for (const Request &request : _requests)
		{
			if (request.needsRebuild(context))
			{
				pending.push_back(&request);
			}
		}
		std::ranges::sort(pending, [](const Request *a, const Request *b) {
			if (a->score != b->score)
			{
				return a->score > b->score;
			}
			if (a->viewIdentity != b->viewIdentity)
			{
				return a->viewIdentity < b->viewIdentity;
			}
			if (a->registrationOrder != b->registrationOrder)
			{
				return a->registrationOrder < b->registrationOrder;
			}
			return std::less<const void *>{}(a->system, b->system);
		});
		std::map<const void *, std::size_t, std::less<const void *>> used;
		std::set<PointKey> seen;
		for (const Request *request : pending)
		{
			const PointKey key{request->system, request->viewIdentity, request->registrationOrder, request->admission};
			if (!seen.insert(key).second)
			{
				continue;
			}
			auto &count = used[request->system];
			if (count < request->limit())
			{
				plan->approved.insert(key);
				++count;
			}
		}
		context.imageState.emplace(this, std::move(plan));
	}

	bool PointBudget::consume(
		RenderContext &context,
		const void *system,
		std::uint64_t viewIdentity,
		std::uint64_t registrationOrder,
		std::uint64_t admission) const
	{
		const auto it = context.imageState.find(this);
		if (it == context.imageState.end())
		{
			return false;
		}
		const auto plan = std::static_pointer_cast<PointImagePlan>(it->second);
		const PointKey key{system, viewIdentity, registrationOrder, admission};
		return plan->approved.contains(key) && plan->consumed.insert(key).second;
	}

	struct RenderingSystem::State final
	{
		struct LightCache
		{
			std::uint32_t resolution = 0;
			Matrix4x4 projection{};
			std::uint64_t staticSignature = 0;
			std::uint64_t dynamicSignature = 0;
			std::uint64_t generation = 0;
			std::uint64_t publishedAdmission = 0;
			std::uint64_t sourceCaptureOrdinal = 0;
			Framebuffer::Handle staticTarget;
			Framebuffer::Handle finalTarget;
			bool point = false;
			std::array<Matrix4x4, 6> pointProjections{};
			std::array<Framebuffer::Handle, 6> pointStaticFaces;
			std::array<Framebuffer::Handle, 6> pointFinalFaces;
			Texture::Handle finalTexture;
		};

		struct SurfaceCache final
		{
			std::mutex mutex;
			std::map<std::pair<std::uint64_t, std::uint64_t>, LightCache> lights;
			Window::Surface::DestructionContract destruction;
		};

		std::mutex mutex;
		std::map<UUID, std::shared_ptr<SurfaceCache>> surfaces;
		std::atomic<std::size_t> maxPointRebuilds{1};
	};

	class DepthCommand final : public RenderCommand
	{
	private:
		std::shared_ptr<RenderingSystem::State> _state;
		std::shared_ptr<Light::Channel> _channel;
		std::weak_ptr<PointBudget> _budget;
		std::uint64_t _viewIdentity;
		std::uint64_t _captureOrdinal;
		Camera3D _camera;
		std::vector<Light::Selection::Entry> _lights;
		std::vector<Caster> _staticCasters;
		std::vector<Caster> _dynamicCasters;
		std::uint64_t _staticSignature;
		std::uint64_t _dynamicSignature;

		[[nodiscard]] std::shared_ptr<RenderingSystem::State::SurfaceCache> _surface(Window::Surface &surface) const
		{
			const UUID identity = surface.identity();
			std::lock_guard lock(_state->mutex);
			const auto it = _state->surfaces.find(identity);
			if (it != _state->surfaces.end())
			{
				return it->second;
			}
			auto cache = std::make_shared<RenderingSystem::State::SurfaceCache>();
			const std::weak_ptr<RenderingSystem::State> weak = _state;
			cache->destruction = surface.subscribeToDestruction([weak, identity] {
				if (const auto state = weak.lock())
				{
					std::lock_guard guard(state->mutex);
					state->surfaces.erase(identity);
				}
			});
			_state->surfaces.emplace(identity, cache);
			return cache;
		}

		void _renderPoint(
			RenderContext &context,
			RenderingSystem::State::LightCache &cache,
			const Light::Selection::Entry &light) const
		{
			if (cache.sourceCaptureOrdinal > _captureOrdinal)
			{
				return;
			}
			const auto projections = Projection::point(light);
			std::array<Matrix4x4, 6> matrices{};
			for (std::size_t face = 0; face < matrices.size(); ++face)
			{
				matrices[face] = projections[face].viewProjection;
			}
			const bool staticDirty = !cache.point || !cache.pointStaticFaces[0] || cache.resolution != light.shadowResolution || cache.pointProjections != matrices || cache.staticSignature != _staticSignature || cache.publishedAdmission != light.admission;
			const bool dynamicDirty = staticDirty || !cache.pointFinalFaces[0] || cache.dynamicSignature != _dynamicSignature;
			std::array<Framebuffer::Handle, 6> staticFaces = cache.pointStaticFaces;
			if (staticDirty)
			{
				auto targets = newCubeTargets(light.shadowResolution);
				staticFaces = std::move(targets.faces);
				for (std::size_t face = 0; face < staticFaces.size(); ++face)
				{
					renderDepth(context, staticFaces[face], matrices[face], _staticCasters, true);
				}
			}
			std::array<Framebuffer::Handle, 6> finalFaces = cache.pointFinalFaces;
			Texture::Handle finalTexture = cache.finalTexture;
			if (dynamicDirty)
			{
				auto targets = newCubeTargets(light.shadowResolution);
				finalTexture = std::move(targets.depth);
				finalFaces = std::move(targets.faces);
				for (std::size_t face = 0; face < finalFaces.size(); ++face)
				{
					copyDepth(context, staticFaces[face], finalFaces[face]);
					renderDepth(context, finalFaces[face], matrices[face], _dynamicCasters, false);
				}
			}
			// Commit only after every face was generated successfully. The previous
			// depth texture remains untouched and valid for historical readers.
			if (!dynamicDirty && cache.publishedAdmission == light.admission)
			{
				return;
			}
			cache.pointStaticFaces = std::move(staticFaces);
			cache.pointFinalFaces = std::move(finalFaces);
			cache.finalTexture = std::move(finalTexture);
			cache.pointProjections = matrices;
			cache.point = true;
			cache.resolution = light.shadowResolution;
			cache.staticSignature = _staticSignature;
			cache.dynamicSignature = _dynamicSignature;
			cache.sourceCaptureOrdinal = _captureOrdinal;

			ShadowMap::Result::Entry entry;
			entry.registrationOrder = light.registrationOrder;
			entry.admission = light.admission;
			entry.generation = ++cache.generation;
			entry.captureOrdinal = _captureOrdinal;
			entry.light = light;
			entry.depthTexture = cache.finalTexture;
			entry.viewProjection = matrices;
			entry.faceCount = 6;
			entry.nearPlane = projections[0].nearPlane;
			entry.farPlane = projections[0].farPlane;
			entry.resolution = light.shadowResolution;
			entry.depthBias = 0.0005f;
			entry.normalBias = 0.025f;
			if (_channel->publishShadowResult(*context.targetSurface, std::move(entry)))
			{
				cache.publishedAdmission = light.admission;
			}
		}

		void _renderLight(
			RenderContext &context,
			RenderingSystem::State::LightCache &cache,
			const Light::Selection::Entry &light) const
		{
			if (light.type == Light::Selection::Type::Point)
			{
				_renderPoint(context, cache, light);
				return;
			}
			if (cache.sourceCaptureOrdinal > _captureOrdinal)
			{
				return;
			}
			const Projection projection = light.type == Light::Selection::Type::Spot
											  ? Projection::spot(light)
											  : Projection::directional(light, _camera);
			const bool staticDirty = cache.point || !cache.staticTarget || cache.resolution != light.shadowResolution ||
									 cache.projection != projection.viewProjection ||
									 cache.staticSignature != _staticSignature;
			const bool dynamicDirty = staticDirty || !cache.finalTarget ||
									  cache.dynamicSignature != _dynamicSignature;
			Framebuffer::Handle staticTarget = cache.staticTarget;
			if (staticDirty)
			{
				staticTarget = newDepthTarget(light.shadowResolution).handle();
				renderDepth(context, staticTarget, projection.viewProjection, _staticCasters, true);
			}
			Framebuffer::Handle finalTarget = cache.finalTarget;
			Texture::Handle finalTexture = cache.finalTexture;
			if (dynamicDirty)
			{
				Framebuffer finalFrame = newDepthTarget(light.shadowResolution);
				finalTexture = *finalFrame.depthStencilAttachmentHandle();
				finalTarget = finalFrame.handle();
				copyDepth(context, staticTarget, finalTarget);
				renderDepth(context, finalTarget, projection.viewProjection, _dynamicCasters, false);
			}
			if (!dynamicDirty && cache.publishedAdmission == light.admission)
			{
				return;
			}
			cache.staticTarget = std::move(staticTarget);
			cache.finalTarget = std::move(finalTarget);
			cache.finalTexture = std::move(finalTexture);
			cache.staticSignature = _staticSignature;
			cache.dynamicSignature = _dynamicSignature;
			cache.projection = projection.viewProjection;
			cache.resolution = light.shadowResolution;
			cache.point = false;
			cache.sourceCaptureOrdinal = _captureOrdinal;
			ShadowMap::Result::Entry entry;
			entry.registrationOrder = light.registrationOrder;
			entry.admission = light.admission;
			entry.generation = ++cache.generation;
			entry.captureOrdinal = _captureOrdinal;
			entry.light = light;
			entry.depthTexture = cache.finalTexture;
			entry.viewProjection[0] = projection.viewProjection;
			entry.nearPlane = projection.nearPlane;
			entry.farPlane = projection.farPlane;
			entry.resolution = light.shadowResolution;
			entry.depthBias = 0.0005f;
			entry.normalBias = 0.025f;
			if (_channel->publishShadowResult(*context.targetSurface, std::move(entry)))
			{
				cache.publishedAdmission = light.admission;
			}
		}

	public:
		DepthCommand(
			std::shared_ptr<RenderingSystem::State> state,
			std::shared_ptr<Light::Channel> channel,
			const EngineView::CaptureContext &view,
			std::vector<Light::Selection::Entry> lights,
			std::vector<Caster> staticCasters,
			std::vector<Caster> dynamicCasters) :
			_state(std::move(state)),
			_channel(std::move(channel)),
			_budget(view.pointShadowBudget),
			_viewIdentity(view.identity),
			_captureOrdinal(view.captureOrdinal),
			_camera(view.cameraSnapshot),
			_lights(std::move(lights)),
			_staticCasters(std::move(staticCasters)),
			_dynamicCasters(std::move(dynamicCasters)),
			_staticSignature(signature(_staticCasters)),
			_dynamicSignature(signature(_dynamicCasters))
		{
		}

		[[nodiscard]] bool pointNeedsRebuild(
			RenderContext &context,
			const Light::Selection::Entry &light) const
		{
			if (context.targetSurface == nullptr ||
				context.targetSurface->lifeCycle() != Window::LifeCycle::Ready ||
				!_channel->isAdmissionCurrent(light.registrationOrder, light.admission) ||
				light.pointShadowStatus != Light::Selection::Entry::PointShadowStatus::Selected)
			{
				return false;
			}
			auto surface = _surface(*context.targetSurface);
			std::lock_guard lock(surface->mutex);
			const auto it = surface->lights.find({_viewIdentity, light.registrationOrder});
			if (it == surface->lights.end())
			{
				return true;
			}
			const auto &cache = it->second;
			if (cache.sourceCaptureOrdinal > _captureOrdinal)
			{
				return false;
			}
			if (!cache.point || !cache.pointStaticFaces[0] || !cache.pointFinalFaces[0] ||
				cache.resolution != light.shadowResolution ||
				cache.staticSignature != _staticSignature ||
				cache.dynamicSignature != _dynamicSignature ||
				cache.publishedAdmission != light.admission)
			{
				return true;
			}
			const auto projections = Projection::point(light);
			for (std::size_t face = 0; face < projections.size(); ++face)
			{
				if (cache.pointProjections[face] != projections[face].viewProjection)
				{
					return true;
				}
			}
			return false;
		}

		void execute(RenderContext &context) const override
		{
			if (context.targetSurface == nullptr || context.targetSurface->lifeCycle() != Window::LifeCycle::Ready ||
				_channel->isClosed())
			{
				return;
			}
			auto surface = _surface(*context.targetSurface);
			std::lock_guard lock(surface->mutex);
			for (const auto &light : _lights)
			{
				if (!_channel->isAdmissionCurrent(light.registrationOrder, light.admission))
				{
					continue;
				}
				if (light.type == Light::Selection::Type::Point)
				{
					auto budget = _budget.lock();
					if (!budget || !budget->consume(
							context, _state.get(), _viewIdentity, light.registrationOrder, light.admission))
					{
						continue;
					}
				}
				auto &cache = surface->lights[{_viewIdentity, light.registrationOrder}];
				_renderLight(context, cache, light);
			}
		}
	};

	class SharedDepthCommand final : public RenderCommand
	{
	private:
		std::shared_ptr<const DepthCommand> _command;

	public:
		explicit SharedDepthCommand(std::shared_ptr<const DepthCommand> command) :
			_command(std::move(command))
		{
		}

		void execute(RenderContext &context) const override
		{
			_command->execute(context);
		}
	};

	std::size_t RenderingSystem::maxPointShadowRebuildsPerImage() const noexcept
	{
		return _state->maxPointRebuilds.load(std::memory_order_relaxed);
	}

	void RenderingSystem::setMaxPointShadowRebuildsPerImage(std::size_t value) noexcept
	{
		_state->maxPointRebuilds.store(value, std::memory_order_relaxed);
	}

	RenderingSystem::RenderingSystem() :
		spk::RenderingSystem("Shadow map rendering"),
		_state(std::make_shared<State>())
	{
	}

	void RenderingSystem::buildViewRenderSnapshot(
		spk::RenderSnapshot::Builder &builder,
		const spk::EngineView::CaptureContext &context)
	{
		if (!context.lightSelection || !context.lightChannel)
		{
			return;
		}
		std::vector<Light::Selection::Entry> lights;
		const auto append = [&lights](const Light::Selection::Entry &light) {
			if (light.castsShadows &&
				(light.type != Light::Selection::Type::Point ||
				 light.pointShadowStatus == Light::Selection::Entry::PointShadowStatus::Selected))
			{
				lights.push_back(light);
			}
		};
		for (const auto &light : context.lightSelection->local)
		{
			append(light);
		}
		for (const auto &light : context.lightSelection->directional)
		{
			append(light);
		}
				{
			std::set<std::uint64_t> selected;
			for (const auto &light : context.lightSelection->local)
			{
				if (light.type == Light::Selection::Type::Point &&
					light.castsShadows &&
					light.pointShadowStatus == Light::Selection::Entry::PointShadowStatus::Selected)
				{
					selected.insert(light.registrationOrder);
				}
			}
			std::lock_guard stateLock(_state->mutex);
			for (const auto &[identity, surface] : _state->surfaces)
			{
				std::lock_guard surfaceLock(surface->mutex);
				std::erase_if(surface->lights, [&selected, &context](const auto &item) {
					return item.first.first == context.identity && item.second.point &&
						   !selected.contains(item.first.second);
				});
			}
		}

		if (lights.empty())
		{
			return;
		}
		std::vector<Caster> staticCasters;
		std::vector<Caster> dynamicCasters;
		for (const ShadowMap::Contributor *contributor : Registry<ShadowMap::Contributor, EngineHeadless *>::instance().elements(engine()))
		{
			if (contributor == nullptr || !contributor->isEffectivelyActive() || contributor->owner() == nullptr ||
				contributor->shadowMode() == Mode::None || !context.isLayerCompatible(contributor->layers()))
			{
				continue;
			}
			auto &destination = contributor->shadowMode() == Mode::Static ? staticCasters : dynamicCasters;
			const Matrix4x4 base = contributor->owner()->transform().modelMatrix() *
								   contributor->meshFilter().baseTransform();
			for (const auto &instance : contributor->meshFilter().instances())
			{
				if (!instance.mesh.empty())
				{
					destination.push_back({instance.mesh, base * instance.localTransform});
				}
			}
		}

		const auto command = std::make_shared<DepthCommand>(
			_state, context.lightChannel, context, lights, std::move(staticCasters), std::move(dynamicCasters));
		if (context.pointShadowBudget)
		{
			for (const auto &light : lights)
			{
				if (light.type != Light::Selection::Type::Point)
				{
					continue;
				}
				context.pointShadowBudget->add({
					.system = _state.get(),
					.viewIdentity = context.identity,
					.registrationOrder = light.registrationOrder,
					.admission = light.admission,
					.score = light.score,
					.limit = [state = _state] {
						return state->maxPointRebuilds.load(std::memory_order_relaxed);
					},
					.needsRebuild = [command, light](RenderContext &renderContext) {
						return command->pointNeedsRebuild(renderContext, light);
					}});
			}
		}
		builder.renderPass(spk::EngineView::ShadowPassKey).emplace<SharedDepthCommand>(command);
	}
}
