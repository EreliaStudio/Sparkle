#pragma once

#include <cstdint>
#include <functional>
#include <memory>

namespace spk
{
	struct RenderContext;
	class GPUResourceCollection;

	class GPUResource
	{
		friend class GPUResourceCollection;

	public:
		using Identifier = std::uint64_t;
		using Generation = std::uint64_t;

		enum class Kind : std::uint8_t
		{
			Buffer,
			VertexArray,
			Texture,
			Framebuffer,
			Program
		};

		static inline constexpr size_t NbKind = 5;

	protected:
		class Instance
		{
		public:
			virtual ~Instance() = default;
		};

	private:
		class LifeTime;

		Identifier _identifier;
		Generation _generation = 0;
		std::shared_ptr<LifeTime> _lifeTime;

		[[nodiscard]] static Identifier _generateIdentifier() noexcept;
		void _subscribeToRelease(std::function<void(Identifier)> callback) const;

	protected:
		GPUResource();

		[[nodiscard]] virtual Kind _kind() const noexcept = 0;
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

		void validate() noexcept;

		[[nodiscard]] Identifier identifier() const noexcept;
		[[nodiscard]] Generation generation() const noexcept;
	};
}