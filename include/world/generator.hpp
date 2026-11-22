#pragma once

#include <concepts>
#include <exception>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "exception.hpp"
#include "world/chunk_coordinate.hpp"

namespace spk::World
{
	template <typename TChunk, typename TGenerationData, typename TBuilder = typename TChunk::Builder>
	class Generator
	{
	public:
		using Builder = TBuilder;

		class Pass
		{
		public:
			virtual ~Pass() = default;

			virtual void apply(
				Builder &builder,
				const ChunkCoordinate &coordinate,
				TGenerationData &data) = 0;
		};

	private:
		std::vector<std::unique_ptr<Pass>> _passes;

		virtual std::optional<Builder> _createChunkBuilder(const ChunkCoordinate &coordinate) = 0;
		virtual TGenerationData _createGenerationData() = 0;

	public:
		virtual ~Generator() = default;

		template <typename TPass, typename... TArgs>
			requires std::derived_from<TPass, Pass>
		TPass &addPass(TArgs &&...args)
		{
			auto pass = std::make_unique<TPass>(std::forward<TArgs>(args)...);
			TPass &result = *pass;
			_passes.push_back(std::move(pass));
			return result;
		}

		[[nodiscard]] std::optional<TChunk> generate(const ChunkCoordinate &coordinate)
		{
			try
			{
				std::optional<Builder> builder = _createChunkBuilder(coordinate);
				if (!builder.has_value())
				{
					return std::nullopt;
				}

				TGenerationData data = _createGenerationData();
				for (const std::unique_ptr<Pass> &pass : _passes)
				{
					pass->apply(*builder, coordinate, data);
				}
				if constexpr (std::same_as<decltype(std::declval<Builder &&>().build()), TChunk>)
				{
					return std::move(*builder).build();
				}
				else
				{
					TChunk chunk;
					chunk.setVolume(std::move(*builder).build());
					return chunk;
				}
			} catch (const spk::Exception &)
			{
				throw;
			} catch (...)
			{
				throw spk::Exception("Chunk generation failed", std::current_exception());
			}
		}
	};
}
