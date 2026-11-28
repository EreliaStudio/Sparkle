#include "shadow_map/rendering_system.hpp"

#include <GL/glew.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
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
#include "graphics/opengl/mesh_layout.hpp"
#include "graphics/opengl/program.hpp"
#include "graphics/opengl/state.hpp"
#include "light/channel.hpp"
#include "lighting/system.hpp"
#include "math/vector4.hpp"
#include "rendering/render_command.hpp"
#include "shadow_map/contributor.hpp"
#include "shadow_map/point_budget.hpp"
#include "shadow_map/projection.hpp"

namespace spk::ShadowMap
{
	namespace
	{
		using PointKey = std::tuple<const void *, std::uint64_t, std::uint64_t, std::uint64_t>;

		std::atomic<std::uint64_t> nextPointBudgetIdentity{1};

		struct PointImagePlan
		{
			std::set<PointKey> approved;
			std::set<PointKey> consumed;
		};

		struct Caster
		{
			Mesh mesh;
			Matrix4x4 transform;
			CasterBounds bounds;
			std::uint64_t signature = 0;
		};

		[[nodiscard]] CasterBounds worldBounds(const Mesh &mesh, const Matrix4x4 &transform)
		{
			const auto &local = mesh.bounds();
			const auto &lo = local.minimum;
			const auto &hi = local.maximum;
			const auto first = transform * Vector4(lo, 1.0f);
			CasterBounds bounds{{first.x, first.y, first.z}, {first.x, first.y, first.z}};
			for (std::size_t i = 1; i < 8; ++i)
			{
				const auto p = transform * Vector4{(i & 1) ? hi.x : lo.x, (i & 2) ? hi.y : lo.y, (i & 4) ? hi.z : lo.z, 1};
				bounds.minimum = {std::min(bounds.minimum.x, p.x), std::min(bounds.minimum.y, p.y), std::min(bounds.minimum.z, p.z)};
				bounds.maximum = {std::max(bounds.maximum.x, p.x), std::max(bounds.maximum.y, p.y), std::max(bounds.maximum.z, p.z)};
			}
			return bounds;
		}

		[[nodiscard]] std::uint64_t mix(std::uint64_t seed, std::uint64_t value) noexcept
		{
			return seed ^ (value + 0x9e3779b97f4a7c15ull + (seed << 6) + (seed >> 2));
		}

		[[nodiscard]] std::uint64_t casterSignature(const Caster &caster)
		{
			std::uint64_t result = caster.mesh.identifier();
			for (std::size_t column = 0; column < 4; ++column)
			{
				for (std::size_t row = 0; row < 4; ++row)
				{
					result = mix(result, std::bit_cast<std::uint32_t>(caster.transform[column][row]));
				}
			}
			return result;
		}

		[[nodiscard]] std::uint64_t signature(const std::vector<Caster> &casters)
		{
			std::uint64_t result = mix(0x46a9c3bbull, casters.size());
			for (const auto &caster : casters)
			{
				result = mix(result, caster.signature);
			}
			return result;
		}

		[[nodiscard]] std::uint64_t signature(const std::vector<const Caster *> &casters)
		{
			std::uint64_t result = mix(0x46a9c3bbull, casters.size());
			for (const auto *caster : casters)
			{
				result = mix(result, caster->signature);
			}
			return result;
		}

		// Retain the original path for same-runner performance comparisons.
		// Depth-only FBOs have no color or stencil attachments. Preserve only
		// the states touched by the pass and lazy resource realization.
		class TargetScope final
		{
		private:
			struct Bindings
			{
				GLint drawFramebuffer = 0, readFramebuffer = 0, program = 0, vertexArray = 0;
				GLint arrayBuffer = 0, texture2D = 0, cubeMap = 0;

				Bindings()
				{
					glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFramebuffer);
					glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFramebuffer);
					glGetIntegerv(GL_CURRENT_PROGRAM, &program);
					glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vertexArray);
					glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &arrayBuffer);
					// Resource activation binds on the current unit; it does not
					// change the active unit or bind any sampler.
					glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture2D);
					glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &cubeMap);
				}

				void restore() const noexcept
				{
					glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(drawFramebuffer));
					glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(readFramebuffer));
					glUseProgram(static_cast<GLuint>(program));
					glBindVertexArray(static_cast<GLuint>(vertexArray));
					glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(arrayBuffer));
					glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(texture2D));
					glBindTexture(GL_TEXTURE_CUBE_MAP, static_cast<GLuint>(cubeMap));
				}
			};

			RenderContext &_context;
			Window::Surface::RenderTarget _previous;
			decltype(RenderContext::_activeProgram) _program;
			Bindings _bindings;
			OpenGL::State::DepthTest _depth;
			OpenGL::State::Rasterization _rasterization;
			OpenGL::State::Viewport _viewport;
			bool _scissorEnabled;

		public:
			explicit TargetScope(RenderContext &context) :
				_context(context),
				_previous(context.activeRenderTarget),
				_program(context._activeProgram),
				_depth(OpenGL::State::DepthTest::capture(context)),
				_rasterization(OpenGL::State::Rasterization::capture(context)),
				_viewport(OpenGL::State::Viewport::capture(context)),
				_scissorEnabled(glIsEnabled(GL_SCISSOR_TEST) == GL_TRUE)
			{
			}

			~TargetScope()
			{
				_bindings.restore();
				_depth.apply(_context);
				_rasterization.apply(_context);
				_viewport.apply(_context);
				if (_scissorEnabled)
				{
					glEnable(GL_SCISSOR_TEST);
				}
				_context._activeProgram = _program;
				_context.activeRenderTarget = _previous;
			}
		};

		[[nodiscard]] Framebuffer newDepthTarget(std::uint32_t resolution)
		{
			return Framebuffer({resolution, resolution}, {}, Texture::Format::Depth32F);
		}

		// Retired depth targets remain immutable until all publication readers
		// release both the framebuffer and its depth texture. The pool is
		// surface-local and bounded; it never waits on CPU readers.
		class DepthTargetPool final
		{
		private:
			static constexpr std::size_t MaxBytes = 128u * 1024u * 1024u;
			std::vector<Framebuffer::Handle> _retired;
			std::size_t _bytes = 0;

			[[nodiscard]] static std::size_t bytes(const Framebuffer::Handle &target)
			{
				return static_cast<std::size_t>(target->size().x) * target->size().y * sizeof(float);
			}

		public:
			[[nodiscard]] Framebuffer::Handle acquire(std::uint32_t resolution)
			{
				for (auto it = _retired.begin(); it != _retired.end(); ++it)
				{
					if ((*it)->size().x == resolution && it->hasUniqueOwner() &&
						(*it)->depthStencilAttachment()->texture().hasUniqueOwner())
					{
						auto result = std::move(*it);
						_bytes -= bytes(result);
						_retired.erase(it);
						return result;
					}
				}
				return newDepthTarget(resolution).handle();
			}

			void retire(const Framebuffer::Handle &target)
			{
				if (!target || bytes(target) > MaxBytes ||
					std::any_of(_retired.begin(), _retired.end(), [&](const auto &other) {
						return other.identifier() == target.identifier();
					}))
				{
					return;
				}
				while (!_retired.empty() && _bytes > MaxBytes - bytes(target))
				{
					_bytes -= bytes(_retired.front());
					_retired.erase(_retired.begin());
				}
				_retired.push_back(target);
				_bytes += bytes(target);
			}
		};

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

		// Geometry is packed only when the set of mesh resource versions changes.
		// Model transforms and draw lists are independent of the shared geometry.
		class DepthBatch final
		{
		private:
			using Key = std::uint64_t;
			struct Range
			{
				GLsizei count;
				std::uintptr_t offset;
				GLint baseVertex;
			};
			using SourceCollection = std::map<Key, const Mesh *>;
			std::map<Key, Range> _ranges;
			Mesh _mesh;
			std::vector<GLsizei> _counts;
			std::vector<const void *> _offsets;
			std::vector<GLint> _baseVertices;
			std::vector<Matrix4x4> _models;

			[[nodiscard]] static Key _key(const Mesh &mesh)
			{
				return mesh.identifier();
			}

			[[nodiscard]] bool _matches(const SourceCollection &sources) const
			{
				if (sources.size() != _ranges.size())
				{
					return false;
				}
				auto range = _ranges.begin();
				for (const auto &source : sources)
				{
					if (source.first != range->first)
					{
						return false;
					}
					++range;
				}
				return true;
			}

			[[nodiscard]] static bool _fits(const SourceCollection &sources)
			{
				std::size_t vertices = 0;
				std::size_t indices = 0;
				for (const auto &[key, mesh] : sources)
				{
					(void)key;
					if (mesh->vertexCount() > static_cast<std::size_t>(std::numeric_limits<GLint>::max()) - vertices ||
						mesh->indexCount() > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max()) ||
						mesh->indexCount() > std::numeric_limits<GLuint>::max() - indices)
					{
						return false;
					}
					vertices += mesh->vertexCount();
					indices += mesh->indexCount();
				}
				return true;
			}

			void _pack(const SourceCollection &sources)
			{
				Mesh::Builder builder;
				std::map<Key, Range> ranges;
				std::size_t indexOffset = 0;
				for (const auto &[key, mesh] : sources)
				{
					const auto base = builder.addVertices(mesh->vertices());
					const auto indices = mesh->indices();
					builder.addIndices(indices);
					ranges.emplace(key, Range{static_cast<GLsizei>(indices.size()), indexOffset * sizeof(std::uint32_t), static_cast<GLint>(base)});
					indexOffset += indices.size();
				}
				_mesh = std::move(builder).build();
				_ranges = std::move(ranges);
			}

		public:
			[[nodiscard]] bool prepare(const std::vector<Caster> &staticCasters, const std::vector<Caster> &dynamicCasters)
			{
				SourceCollection sources;
				for (const auto *casters : {&staticCasters, &dynamicCasters})
				{
					for (const auto &caster : *casters)
					{
						sources.try_emplace(_key(caster.mesh), &caster.mesh);
					}
				}
				if (_matches(sources))
				{
					return true;
				}
				if (!_fits(sources))
				{
					return false;
				}
				_pack(sources);
				return true;
			}

			void begin()
			{
				_counts.clear();
				_offsets.clear();
				_baseVertices.clear();
				_models.clear();
			}

			void append(const Caster &caster)
			{
				const auto &range = _ranges.at(_key(caster.mesh));
				_counts.push_back(range.count);
				_offsets.push_back(reinterpret_cast<const void *>(range.offset));
				_baseVertices.push_back(range.baseVertex);
				_models.push_back(caster.transform);
			}

			void draw(RenderContext &context, const Matrix4x4 &projection)
			{
				for (auto &model : _models)
				{
					model = projection * model;
				}
				MeshLayout::obtain(_mesh).activate(context);
				for (std::size_t i = 0; i < _models.size(); ++i)
				{
					glUniformMatrix4fv(0, 1, GL_FALSE, &_models[i][0][0]);
					glDrawElementsBaseVertex(GL_TRIANGLES, _counts[i], GL_UNSIGNED_INT, _offsets[i], _baseVertices[i]);
				}
			}
		};

		// All shadow maps share a depth-only pass. No UBO allocation per
		// caster, no repeated capture/restore of all GL state per face.
		class DepthRenderer final
		{
		private:
			RenderContext &_context;
			std::optional<TargetScope> _scope;
			Vector2UInt _size{0, 0};
			DepthBatch &_batch;
			const std::vector<Caster> &_staticCasters;
			const std::vector<Caster> &_dynamicCasters;
			bool _batchEnabled = true;
			bool _batchPrepared = false;

			[[nodiscard]] static Program &program()
			{
				static Program value(
					R"(#version 460 core
layout(location = 0) in vec3 inPosition;
layout(location = 0) uniform mat4 uMatrix;
void main()
{
	gl_Position = uMatrix * vec4(inPosition, 1.0);
}
)",
					std::string(resources::text("shaders/shadow_depth.frag.glsl")));
				return value;
			}

			void _begin()
			{
				if (_scope.has_value())
				{
					return;
				}
				_scope.emplace(_context);
				glDisable(GL_SCISSOR_TEST);
				OpenGL::State::DepthTest{}.apply(_context);
				auto rasterization = OpenGL::State::Rasterization{};
				rasterization.polygonOffsetEnabled = true;
				rasterization.polygonOffsetFactor = 1.5f;
				rasterization.polygonOffsetUnits = 2.0f;
				rasterization.apply(_context);
				program().activate(_context);
			}

		public:
			DepthRenderer(
				RenderContext &context,
				DepthBatch &batch,
				const std::vector<Caster> &staticCasters,
				const std::vector<Caster> &dynamicCasters) :
				_context(context),
				_batch(batch),
				_staticCasters(staticCasters),
				_dynamicCasters(dynamicCasters)
			{
			}

			static const Caster &casterRef(const Caster &caster)
			{
				return caster;
			}
			static const Caster &casterRef(const Caster *caster)
			{
				return *caster;
			}

			template <typename TCaster>
			void render(const Framebuffer::Handle &framebuffer, const Matrix4x4 &projection, const std::vector<TCaster> &casters, bool clear)
			{
				_begin();
				const Window::Surface::RenderTarget target(*_context.targetSurface, framebuffer);
				target.activate(_context);
				_context.activeRenderTarget = target;
				if (_size != target.size())
				{
					_size = target.size();
					OpenGL::State::Viewport{{0, 0, static_cast<int>(_size.x), static_cast<int>(_size.y)}}.apply(_context);
				}
				if (clear)
				{
					OpenGL::State::Clear::depth(_context);
				}
				if (casters.empty())
				{
					return;
				}
				if (_batchEnabled && !_batchPrepared)
				{
					_batchEnabled = _batch.prepare(_staticCasters, _dynamicCasters);
					_batchPrepared = true;
				}
				if (_batchEnabled)
				{
					_batch.begin();
					for (const auto &item : casters)
					{
						_batch.append(casterRef(item));
					}
					_batch.draw(_context, projection);
					return;
				}
				// Oversized geometry cannot be represented in a shared index buffer.
				// Preserve correctness by drawing the original meshes individually.
				for (const auto &item : casters)
				{
					const Caster &caster = casterRef(item);
					const auto mvp = projection * caster.transform;
					glUniformMatrix4fv(0, 1, GL_FALSE, &mvp[0][0]);
					MeshLayout::obtain(caster.mesh).activate(_context);
					program().render(Program::Primitive::Triangles, IndexBuffer::Type::UnsignedInt, 0, caster.mesh.indexCount());
				}
			}

			void copy(const Framebuffer::Handle &source, const Framebuffer::Handle &destination)
			{
				_begin();
				source.activate(_context);
				GLint read = 0;
				glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &read);
				destination.activate(_context);
				GLint draw = 0;
				glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw);
				const auto size = destination->size();
				glBlitNamedFramebuffer(static_cast<GLuint>(read), static_cast<GLuint>(draw), 0, 0, static_cast<GLsizei>(size.x), static_cast<GLsizei>(size.y), 0, 0, static_cast<GLsizei>(size.x), static_cast<GLsizei>(size.y), GL_DEPTH_BUFFER_BIT, GL_NEAREST);
			}

			void finish() const
			{
				if (_scope.has_value())
				{
					if (const GLenum error = glGetError(); error != GL_NO_ERROR)
					{
						throw spk::Exception("Shadow depth rendering failed with OpenGL error " + std::to_string(error));
					}
				}
			}
		};

	}

	PointBudget::PointBudget() :
		_identity(nextPointBudgetIdentity.fetch_add(1, std::memory_order_relaxed))
	{
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
		if (context.imageState.contains(_identity))
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
		context.imageState.emplace(_identity, std::move(plan));
	}

	bool PointBudget::consume(
		RenderContext &context,
		const void *system,
		std::uint64_t viewIdentity,
		std::uint64_t registrationOrder,
		std::uint64_t admission) const
	{
		const auto it = context.imageState.find(_identity);
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
		struct CascadeCache
		{
			Matrix4x4 projection{};
			std::uint64_t staticSignature = 0;
			std::uint64_t dynamicSignature = 0;
			Framebuffer::Handle staticTarget;
			Texture::Handle staticTexture;
			Result::Entry::CascadeStatistics statistics;
			Framebuffer::Handle finalTarget;
			Texture::Handle texture;
		};

		struct LightCache
		{
			DepthTargetPool depthTargets;
			std::array<CascadeCache, Projection::CascadeCount> cascades{};
			bool cascaded = false;
			bool separateDynamic = false;
			std::size_t cascadeCount = 0;
			Matrix4x4 cascadeCameraView{};
			Matrix4x4 cascadeCameraProjection{};
			Vector3 cascadeDirection{};
			float cascadeDistance = 0.0f;
			std::array<float, Projection::CascadeCount> cascadeSplits{};
			std::array<Projection, Projection::CascadeCount> cascadeProjections{};
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
			DepthBatch depthBatch;
			Window::Surface::DestructionContract destruction;
		};

		std::mutex mutex;
		std::map<UUID, std::shared_ptr<SurfaceCache>> surfaces;
		std::atomic<std::size_t> maxPointRebuilds{1};
		std::atomic<bool> directionalCascades{true};
		std::atomic<RenderingSystem::CascadeMode> directionalCascadeMode{RenderingSystem::CascadeMode::QuadraPass};
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
		bool _directionalCascades;
		std::size_t _directionalCascadeCount;
		std::vector<CasterBounds> _bounds;
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
			DepthBatch &batch,
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
			DepthRenderer renderer(context, batch, _staticCasters, _dynamicCasters);
			std::array<Framebuffer::Handle, 6> staticFaces = cache.pointStaticFaces;
			if (staticDirty)
			{
				auto targets = newCubeTargets(light.shadowResolution);
				staticFaces = std::move(targets.faces);
				for (std::size_t face = 0; face < staticFaces.size(); ++face)
				{
					renderer.render(staticFaces[face], matrices[face], _staticCasters, true);
				}
			}
			std::array<Framebuffer::Handle, 6> finalFaces = cache.pointFinalFaces;
			Texture::Handle finalTexture = cache.finalTexture;
			if (dynamicDirty)
			{
				if (_dynamicCasters.empty())
				{
					finalFaces = staticFaces;
					finalTexture = staticFaces.front()->depthStencilAttachment()->texture();
				}
				else
				{
					auto targets = newCubeTargets(light.shadowResolution);
					finalTexture = std::move(targets.depth);
					finalFaces = std::move(targets.faces);
					for (std::size_t face = 0; face < finalFaces.size(); ++face)
					{
						renderer.copy(staticFaces[face], finalFaces[face]);
						renderer.render(finalFaces[face], matrices[face], _dynamicCasters, false);
					}
				}
			}
			renderer.finish();
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

		void _renderDirectional(
			RenderContext &context, DepthBatch &batch, RenderingSystem::State::LightCache &cache, const Light::Selection::Entry &light, std::size_t count, bool separateDynamic) const
		{
			if (cache.sourceCaptureOrdinal > _captureOrdinal)
			{
				return;
			}
			const auto cameraView = _camera.viewMatrix();
			const auto cameraProjection = _camera.projectionMatrix();
			if (cache.cascaded && cache.cascadeCount == count && cache.separateDynamic == separateDynamic && cache.resolution == light.shadowResolution &&
				cache.publishedAdmission == light.admission && cache.cascadeCameraView == cameraView &&
				cache.cascadeCameraProjection == cameraProjection && cache.cascadeDirection == light.direction &&
				cache.cascadeDistance == light.shadowDistance && cache.staticSignature == _staticSignature &&
				cache.dynamicSignature == _dynamicSignature)
			{
				return;
			}
			const bool canReuseProjection = cache.cascaded && cache.cascadeCount == count &&
											cache.resolution == light.shadowResolution && cache.cascadeDirection == light.direction;
			const auto previousProjections = canReuseProjection ? std::span<const Projection>(cache.cascadeProjections.data(), count) : std::span<const Projection>{};
			const auto projections = Projection::directionalCascades(light, _camera, _bounds, count, previousProjections);
			DepthRenderer renderer(context, batch, _staticCasters, _dynamicCasters);
			auto cascades = cache.cascades;
			for (std::size_t i = count; i < cascades.size(); ++i)
			{
				cascades[i] = {};
			}
			std::vector<const Caster *> staticCasters, dynamicCasters;
			staticCasters.reserve(_staticCasters.size());
			dynamicCasters.reserve(_dynamicCasters.size());
			for (std::size_t i = 0; i < count; ++i)
			{
				const auto &projection = projections[i];
				auto &cascade = cascades[i];
				const bool staticTargetDirty = !cache.cascaded || cache.cascadeCount != count || !cascade.staticTarget ||
											   cache.resolution != light.shadowResolution || cascade.projection != projection.viewProjection;
				const bool reuseStaticSelection = !staticTargetDirty && cache.staticSignature == _staticSignature;
				auto staticSignature = cascade.staticSignature;
				if (!reuseStaticSelection)
				{
					staticCasters.clear();
					for (const auto &caster : _staticCasters)
					{
						if (projection.intersects(caster.bounds))
						{
							staticCasters.push_back(&caster);
						}
					}
					staticSignature = signature(staticCasters);
					cascade.statistics.staticCasters = staticCasters.size();
				}
				dynamicCasters.clear();
				for (const auto &caster : _dynamicCasters)
				{
					if (projection.intersects(caster.bounds))
					{
						dynamicCasters.push_back(&caster);
					}
				}
				const auto dynamicSignature = signature(dynamicCasters);
				const bool staticDirty = staticTargetDirty || cascade.staticSignature != staticSignature;
				// An independent dynamic map does not depend on static depth.
				// Combined maps and static-only aliases still require rebuilding.
				const bool staticAffectsDynamic = staticDirty && (!separateDynamic || dynamicCasters.empty());
				const bool dynamicDirty = staticTargetDirty || staticAffectsDynamic || !cascade.finalTarget ||
										  cache.separateDynamic != separateDynamic || cascade.dynamicSignature != dynamicSignature;
				if (staticDirty)
				{
					cascade.staticTarget = cache.depthTargets.acquire(light.shadowResolution);
					cascade.staticTexture = cascade.staticTarget->depthStencilAttachment()->texture();
					renderer.render(cascade.staticTarget, projection.viewProjection, staticCasters, true);
					++cascade.statistics.staticBuilds;
				}
				if (dynamicDirty)
				{
					// Static-only scenes need no copy or second depth allocation.
					if (dynamicCasters.empty())
					{
						cascade.finalTarget = cascade.staticTarget;
						cascade.texture = cascade.staticTexture;
					}
					else
					{
						cascade.finalTarget = cache.depthTargets.acquire(light.shadowResolution);
						cascade.texture = cascade.finalTarget->depthStencilAttachment()->texture();
						if (!separateDynamic)
						{
							renderer.copy(cascade.staticTarget, cascade.finalTarget);
						}
						renderer.render(cascade.finalTarget, projection.viewProjection, dynamicCasters, separateDynamic);
						++cascade.statistics.dynamicBuilds;
					}
				}
				cascade.projection = projection.viewProjection;
				cascade.staticSignature = staticSignature;
				cascade.dynamicSignature = dynamicSignature;
				cascade.statistics.dynamicCasters = dynamicCasters.size();
			}
			renderer.finish();
			for (std::size_t i = 0; i < cascades.size(); ++i)
			{
				const auto &previous = cache.cascades[i];
				const auto &next = cascades[i];
				for (const auto &target : {previous.staticTarget, previous.finalTarget})
				{
					if (target.identifier() != next.staticTarget.identifier() &&
						target.identifier() != next.finalTarget.identifier())
					{
						cache.depthTargets.retire(target);
					}
				}
			}
			std::array<float, Projection::CascadeCount> splits{};
			for (std::size_t i = 0; i < count; ++i)
			{
				splits[i] = projections[i].splitFar;
			}
			// Publish only if the maps or their selection metadata changed.
			// Camera-space split selection still follows camera movement when
			// stabilized maps are reusable.
			cache.staticTarget = {};
			cache.finalTarget = {};
			cache.finalTexture = {};
			cache.cascades = std::move(cascades);
			cache.cascaded = true;
			cache.cascadeCount = count;
			cache.separateDynamic = separateDynamic;
			cache.point = false;
			cache.resolution = light.shadowResolution;
			cache.sourceCaptureOrdinal = _captureOrdinal;
			cache.cascadeCameraView = cameraView;
			cache.cascadeCameraProjection = cameraProjection;
			cache.cascadeDirection = light.direction;
			cache.cascadeDistance = light.shadowDistance;
			cache.staticSignature = _staticSignature;
			cache.dynamicSignature = _dynamicSignature;
			cache.cascadeSplits = splits;
			cache.cascadeProjections = projections;
			ShadowMap::Result::Entry entry;
			entry.registrationOrder = light.registrationOrder;
			entry.admission = light.admission;
			entry.generation = ++cache.generation;
			entry.captureOrdinal = _captureOrdinal;
			entry.light = light;
			entry.resolution = light.shadowResolution;
			entry.depthTexture = separateDynamic ? cache.cascades.front().staticTexture : cache.cascades.front().texture;
			entry.nearPlane = projections.front().nearPlane;
			entry.farPlane = projections.front().farPlane;
			entry.cascadeCameraView = cameraView;
			entry.cascadeSplits = splits;
			entry.cascadeCount = static_cast<std::uint8_t>(count);
			entry.cascadeNear = projections.front().splitNear;
			for (std::size_t i = 0; i < count; ++i)
			{
				const auto &projection = projections[i];
				const auto &cascade = cache.cascades[i];
				entry.cascadeTextures[i] = separateDynamic ? cascade.staticTexture : cascade.texture;
				if (separateDynamic && cascade.statistics.dynamicCasters != 0)
				{
					entry.cascadeDynamicTextures[i] = cascade.texture;
				}
				entry.cascadeStatistics[i] = cache.cascades[i].statistics;
				entry.viewProjection[i] = projection.viewProjection;
				entry.cascadeDepthBias[i] = std::max(0.000001f, 0.35f * projection.worldPerTexel / (projection.farPlane - projection.nearPlane));
				entry.cascadeNormalBias[i] = 0.3f * projection.worldPerTexel;
			}
			entry.depthBias = entry.cascadeDepthBias[0];
			entry.normalBias = entry.cascadeNormalBias[0];
			if (_channel->publishShadowResult(*context.targetSurface, std::move(entry)))
			{
				cache.publishedAdmission = light.admission;
			}
		}

		void _renderLight(
			RenderContext &context,
			DepthBatch &batch,
			RenderingSystem::State::LightCache &cache,
			const Light::Selection::Entry &light,
			std::size_t cascadeCount,
			bool separateDynamic) const
		{
			if (light.type == Light::Selection::Type::Point)
			{
				_renderPoint(context, batch, cache, light);
				return;
			}
			if (cache.sourceCaptureOrdinal > _captureOrdinal)
			{
				return;
			}
			if (light.type == Light::Selection::Type::Directional && _directionalCascades)
			{
				_renderDirectional(context, batch, cache, light, cascadeCount, separateDynamic);
				return;
			}
			const Projection projection = light.type == Light::Selection::Type::Spot
											  ? Projection::spot(light)
											  : Projection::directional(light, _camera);
			const bool staticDirty = cache.point || cache.cascaded || !cache.staticTarget || cache.resolution != light.shadowResolution ||
									 cache.projection != projection.viewProjection ||
									 cache.staticSignature != _staticSignature;
			const bool dynamicDirty = staticDirty || !cache.finalTarget ||
									  cache.dynamicSignature != _dynamicSignature;
			DepthRenderer renderer(context, batch, _staticCasters, _dynamicCasters);
			Framebuffer::Handle staticTarget = cache.staticTarget;
			if (staticDirty)
			{
				staticTarget = cache.depthTargets.acquire(light.shadowResolution);
				renderer.render(staticTarget, projection.viewProjection, _staticCasters, true);
			}
			Framebuffer::Handle finalTarget = cache.finalTarget;
			Texture::Handle finalTexture = cache.finalTexture;
			if (dynamicDirty)
			{
				if (_dynamicCasters.empty())
				{
					finalTarget = staticTarget;
					finalTexture = staticTarget->depthStencilAttachment()->texture();
				}
				else
				{
					finalTarget = cache.depthTargets.acquire(light.shadowResolution);
					finalTexture = finalTarget->depthStencilAttachment()->texture();
					renderer.copy(staticTarget, finalTarget);
					renderer.render(finalTarget, projection.viewProjection, _dynamicCasters, false);
				}
			}
			if (!dynamicDirty && cache.publishedAdmission == light.admission)
			{
				return;
			}
			renderer.finish();
			for (const auto &target : {cache.staticTarget, cache.finalTarget})
			{
				if (target.identifier() != staticTarget.identifier() && target.identifier() != finalTarget.identifier())
				{
					cache.depthTargets.retire(target);
				}
			}
			cache.cascaded = false;
			cache.cascades = {};
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
			_directionalCascades(_state->directionalCascades.load(std::memory_order_relaxed)),
			_directionalCascadeCount(static_cast<std::size_t>(_state->directionalCascadeMode.load(std::memory_order_relaxed))),
			_lights(std::move(lights)),
			_staticCasters(std::move(staticCasters)),
			_dynamicCasters(std::move(dynamicCasters)),
			_staticSignature(signature(_staticCasters)),
			_dynamicSignature(signature(_dynamicCasters))
		{
			if (_directionalCascades)
			{
				_bounds.reserve(_staticCasters.size() + _dynamicCasters.size());
				for (const auto &caster : _staticCasters)
				{
					_bounds.push_back(caster.bounds);
				}
				for (const auto &caster : _dynamicCasters)
				{
					_bounds.push_back(caster.bounds);
				}
			}
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
			std::size_t slots = Lighting::System::MaxShadowSamplers;
			std::size_t remaining = static_cast<std::size_t>(std::count_if(_lights.begin(), _lights.end(), [](const auto &light) {
				return light.type != Light::Selection::Type::Point;
			}));
			for (const auto &light : _lights)
			{
				std::size_t cascadeCount = 1;
				bool separateDynamic = false;
				if (light.type != Light::Selection::Type::Point)
				{
					if (_directionalCascades && light.type == Light::Selection::Type::Directional)
					{
						const auto available = slots >= remaining ? slots - remaining + 1 : 1;
						cascadeCount = std::min(_directionalCascadeCount, available);
					}
					// Prefer independent static/dynamic depth, without a full-size
					// blit on every object movement. Keep one slot for each other
					// admitted light; fall back to combined depth under pressure.
					separateDynamic = _directionalCascades && light.type == Light::Selection::Type::Directional &&
									  !_dynamicCasters.empty() && slots >= remaining + 2 * cascadeCount - 1;
					const std::size_t used = separateDynamic ? 2 * cascadeCount : cascadeCount;
					slots = slots >= used ? slots - used : 0;
					--remaining;
				}
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
				_renderLight(context, surface->depthBatch, cache, light, cascadeCount, separateDynamic);
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

	void RenderingSystem::setDirectionalCascadeMode(CascadeMode mode)
	{
		const auto count = static_cast<std::size_t>(mode);
		if (count == 0 || count > Projection::CascadeCount)
		{
			throw spk::Exception("Invalid directional cascade mode");
		}
		_state->directionalCascadeMode.store(mode, std::memory_order_relaxed);
		_state->directionalCascades.store(true, std::memory_order_relaxed);
	}

	RenderingSystem::CascadeMode RenderingSystem::directionalCascadeMode() const noexcept
	{
		return _state->directionalCascadeMode.load(std::memory_order_relaxed);
	}

	void RenderingSystem::setDirectionalCascadesEnabled(bool enabled) noexcept
	{
		_state->directionalCascades.store(enabled, std::memory_order_relaxed);
	}

	bool RenderingSystem::directionalCascadesEnabled() const noexcept
	{
		return _state->directionalCascades.load(std::memory_order_relaxed);
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
		const bool needsBounds = directionalCascadesEnabled() && std::any_of(lights.begin(), lights.end(), [](const auto &light) {
									 return light.type == Light::Selection::Type::Directional;
								 });
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
					const auto transform = base * instance.localTransform;
					Caster caster{instance.mesh, transform, needsBounds ? worldBounds(instance.mesh, transform) : CasterBounds{}};
					caster.signature = casterSignature(caster);
					destination.push_back(std::move(caster));
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
				context.pointShadowBudget->add({.system = _state.get(), .viewIdentity = context.identity, .registrationOrder = light.registrationOrder, .admission = light.admission, .score = light.score, .limit = [state = _state] {
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
