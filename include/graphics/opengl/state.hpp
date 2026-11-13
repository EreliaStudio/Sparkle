#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace spk
{
	struct RenderContext;
}

namespace spk::OpenGL
{
	class State final
	{
	public:
		enum class Comparison : std::uint32_t
		{
			Never = 0x0200,
			Less = 0x0201,
			Equal = 0x0202,
			LessEqual = 0x0203,
			Greater = 0x0204,
			NotEqual = 0x0205,
			GreaterEqual = 0x0206,
			Always = 0x0207
		};

		enum class BlendFactor : std::uint32_t
		{
			Zero = 0,
			One = 1,
			SourceColor = 0x0300,
			OneMinusSourceColor = 0x0301,
			SourceAlpha = 0x0302,
			OneMinusSourceAlpha = 0x0303,
			DestinationAlpha = 0x0304,
			OneMinusDestinationAlpha = 0x0305,
			DestinationColor = 0x0306,
			OneMinusDestinationColor = 0x0307,
			SourceAlphaSaturate = 0x0308,
			ConstantColor = 0x8001,
			OneMinusConstantColor = 0x8002,
			ConstantAlpha = 0x8003,
			OneMinusConstantAlpha = 0x8004
		};

		enum class BlendEquation : std::uint32_t
		{
			Add = 0x8006,
			Subtract = 0x800A,
			ReverseSubtract = 0x800B,
			Minimum = 0x8007,
			Maximum = 0x8008
		};

		enum class StencilOperation : std::uint32_t
		{
			Zero = 0,
			Keep = 0x1E00,
			Replace = 0x1E01,
			Increment = 0x1E02,
			Decrement = 0x1E03,
			Invert = 0x150A,
			IncrementWrap = 0x8507,
			DecrementWrap = 0x8508
		};

		enum class Culling : std::uint32_t
		{
			Front = 0x0404,
			Back = 0x0405,
			Both = 0x0408
		};

		enum class Winding : std::uint32_t
		{
			Clockwise = 0x0900,
			CounterClockwise = 0x0901
		};

		enum class PolygonMode : std::uint32_t
		{
			Point = 0x1B00,
			Line = 0x1B01,
			Fill = 0x1B02
		};

		struct DepthTest
		{
			bool enabled = true;
			bool write = true;
			Comparison comparison = Comparison::LessEqual;
			double clear = 1.0;
			std::array<double, 2> range{0.0, 1.0};

			[[nodiscard]] static DepthTest capture(RenderContext &context);
			void apply(RenderContext &context) const noexcept;
		};

		struct StencilTest
		{
			struct Face
			{
				Comparison comparison = Comparison::Always;
				int reference = 0;
				std::uint32_t comparisonMask = ~0u;
				std::uint32_t writeMask = ~0u;
				StencilOperation fail = StencilOperation::Keep;
				StencilOperation depthFail = StencilOperation::Keep;
				StencilOperation pass = StencilOperation::Keep;
			};

			bool enabled = false;
			int clear = 0;
			Face front{};
			Face back{};

			[[nodiscard]] static StencilTest capture(RenderContext &context);
			void apply(RenderContext &context) const noexcept;
		};

		struct Blend
		{
			struct Attachment
			{
				bool enabled = false;
				BlendFactor sourceRGB = BlendFactor::SourceAlpha;
				BlendFactor destinationRGB = BlendFactor::OneMinusSourceAlpha;
				BlendFactor sourceAlpha = BlendFactor::One;
				BlendFactor destinationAlpha = BlendFactor::OneMinusSourceAlpha;
				BlendEquation equationRGB = BlendEquation::Add;
				BlendEquation equationAlpha = BlendEquation::Add;
			};

			std::vector<Attachment> attachments;
			std::array<float, 4> constantColor{0, 0, 0, 0};

			[[nodiscard]] static Blend capture(RenderContext &context);
			[[nodiscard]] static Blend disabled(RenderContext &context);
			void apply(RenderContext &context) const noexcept;
		};

		struct Rasterization
		{
			bool cullEnabled = true;
			Culling cull = Culling::Back;
			Winding frontWinding = Winding::CounterClockwise;
			PolygonMode polygonMode = PolygonMode::Fill;
			bool polygonOffsetEnabled = false;
			float polygonOffsetFactor = 0;
			float polygonOffsetUnits = 0;
			bool multisampleEnabled = true;
			bool sampleAlphaToCoverageEnabled = false;

			[[nodiscard]] static Rasterization capture(RenderContext &context);
			void apply(RenderContext &context) const noexcept;
		};

		struct Scissor
		{
			bool enabled = false;
			std::array<int, 4> rectangle{0, 0, 0, 0};

			[[nodiscard]] static Scissor capture(RenderContext &context);
			void apply(RenderContext &context) const noexcept;
		};

		struct Viewport
		{
			std::array<int, 4> rectangle{0, 0, 0, 0};

			[[nodiscard]] static Viewport capture(RenderContext &context);
			void apply(RenderContext &context) const noexcept;
		};

		struct ColorOutput
		{
			std::vector<std::array<bool, 4>> masks;
			std::array<float, 4> clear{0, 0, 0, 0};
			bool framebufferSRGB = false;

			[[nodiscard]] static ColorOutput capture(RenderContext &context);
			[[nodiscard]] static ColorOutput writable(RenderContext &context, bool framebufferSRGB = false);
			[[nodiscard]] static bool destinationIsSRGB(RenderContext &context) noexcept;
			[[nodiscard]] static bool srgbEnabled(RenderContext &context) noexcept;
			static void setSRGB(RenderContext &context, bool enabled) noexcept;

			class SRGBScope;
			void apply(RenderContext &context) const noexcept;
		};

		struct Clear
		{
			static void color(RenderContext &context, std::uint32_t attachment, const std::array<float, 4> &rgba) noexcept;
			static void depth(RenderContext &context, float value = 1.0f) noexcept;
			static void stencil(RenderContext &context, int value = 0) noexcept;
		};

		struct Bindings
		{
			struct TextureUnit
			{
				std::uint32_t texture2D = 0;
				std::uint32_t sampler = 0;
			};

			std::uint32_t drawFramebuffer = 0;
			std::uint32_t readFramebuffer = 0;
			std::uint32_t program = 0;
			std::uint32_t vertexArray = 0;
			std::uint32_t activeTexture = 0;
			std::vector<TextureUnit> textureUnits;

			[[nodiscard]] static Bindings capture(RenderContext &context);
			void apply(RenderContext &context) const noexcept;
		};

		DepthTest depthTest;
		StencilTest stencilTest;
		Blend blend;
		Rasterization rasterization;
		Scissor scissor;
		Viewport viewport;
		ColorOutput colorOutput;
		Bindings bindings;

		[[nodiscard]] static State capture(RenderContext &context);
		void apply(RenderContext &context) const noexcept;

		class Scope;
	};

	class State::ColorOutput::SRGBScope final
	{
	private:
		RenderContext &_context;
		bool _previous;

	public:
		explicit SRGBScope(RenderContext &context, bool enable) noexcept;
		~SRGBScope();
		SRGBScope(const SRGBScope &) = delete;
		SRGBScope &operator=(const SRGBScope &) = delete;
	};

	class State::Scope final
	{
	private:
		RenderContext &_context;
		State _previous;

	public:
		explicit Scope(RenderContext &context);
		~Scope();
		Scope(const Scope &) = delete;
		Scope &operator=(const Scope &) = delete;
	};
}
