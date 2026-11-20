#pragma once

#include <concepts>
#include <exception>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "exception.hpp"
#include "world/chunk_coordinate.hpp"

namespace spk
{
	template <typename TChunk, typename TGenerationData>
	class Generator
	{
	public:
		class Pass
		{
		public:
			virtual ~Pass() = default;

			virtual void apply(
				typename TChunk::Builder &builder,
				const ChunkCoordinate &coordinate,
				TGenerationData &data) = 0;
		};

	private:
		std::vector<std::unique_ptr<Pass>> _passes;

	protected:
		virtual std::optional<typename TChunk::Builder> createChunkBuilder(const ChunkCoordinate &coordinate) = 0;
		virtual TGenerationData createGenerationData(const ChunkCoordinate &coordinate) = 0;

	public:
		virtual ~Generator() = default;

		void addPass(std::unique_ptr<Pass> pass)
		{
			if (pass == nullptr)
			{
				throw spk::Exception("Cannot add a null generation pass");
			}
			_passes.push_back(std::move(pass));
		}

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
				std::optional<typename TChunk::Builder> builder = createChunkBuilder(coordinate);
				if (!builder.has_value())
				{
					return std::nullopt;
				}

				TGenerationData data = createGenerationData(coordinate);
				for (const std::unique_ptr<Pass> &pass : _passes)
				{
					pass->apply(*builder, coordinate, data);
				}
				return std::move(*builder).build();
			}
			catch (const spk::Exception &)
			{
				throw;
			}
			catch (...)
			{
				throw spk::Exception("Chunk generation failed", std::current_exception());
			}
		}
	};
}
