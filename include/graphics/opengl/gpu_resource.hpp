#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>

#include "design_pattern/trait/versioned_trait.hpp"

namespace spk
{
	struct RenderContext;
	class GPUResourceCollection;

	class GPUResource : public VersionedTrait
	{
		friend class GPUResourceCollection;

	public:
		using Identifier = std::uint64_t;
		using Generation = Version;
		using RecyclingScore = std::uint32_t;

		enum class Kind : std::uint8_t
		{
			Buffer,
			VertexArray,
			Texture,
			Framebuffer,
			Program,
			Sampler
		};

		static inline constexpr std::size_t NbKind = 6;

	protected:
		class Instance
		{
		public:
			virtual ~Instance() = default;
		};

	private:
		class LifeTime;

		Identifier _identifier;
		std::shared_ptr<LifeTime> _lifeTime;

		[[nodiscard]] static Identifier _generateIdentifier() noexcept;
		void _subscribeToRelease(std::function<void(Identifier)> callback) const;

	protected:
		GPUResource();

		[[nodiscard]] virtual Kind _kind() const noexcept = 0;
		[[nodiscard]] virtual RecyclingScore _recyclingScore(const Instance &instance) const noexcept;
		[[nodiscard]] virtual std::unique_ptr<Instance> _create(RenderContext &context) const = 0;
		virtual void _synchronize(Instance &instance, RenderContext &context) const = 0;
		virtual void _bind(Instance &instance, RenderContext &context) const = 0;

	public:
		GPUResource(const GPUResource &) = delete;
		GPUResource(GPUResource &&other) noexcept;
		virtual ~GPUResource() = default;

		GPUResource &operator=(const GPUResource &) = delete;
		GPUResource &operator=(GPUResource &&) = delete;

		void activate(RenderContext &context) const;

		void validate();

		[[nodiscard]] Identifier identifier() const noexcept;
		[[nodiscard]] Generation generation() const noexcept;
	};
}
