#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace spk
{
	class RenderCommand;
	struct RenderContext;

	class RenderPass
	{
	public:
		using Name = std::string;
		using Order = std::int32_t;

		struct Key
		{
			Name name;
			Order order;
		};

		RenderPass();
		RenderPass(const RenderPass &) = delete;
		RenderPass(RenderPass &&) noexcept;
		~RenderPass();

		RenderPass &operator=(const RenderPass &) = delete;
		RenderPass &operator=(RenderPass &&) noexcept;

		void append(std::unique_ptr<const RenderCommand> renderCommand);

		template <typename TCommandType, typename... TArgs>
		void emplace(TArgs &&...args)
		{
			append(std::make_unique<TCommandType>(std::forward<TArgs>(args)...));
		}

		void execute(RenderContext &renderContext) const;

	private:
		std::vector<std::unique_ptr<const RenderCommand>> _commands;
	};
}
