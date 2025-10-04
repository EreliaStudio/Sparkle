#include "playground.hpp"

enum class PlaygroundTileFlag
{
	None = 0
};

class HeightMap : public spk::Grid2D<int>
{
private:
	struct FalloffFunction
    {
        static float linear(const float& p_value)
        {
            return std::clamp(p_value, 0.0f, 1.0f);
        }
        static float smoothstep(const float& p_value)
        {
			float clampedValue = std::clamp(p_value, 0.0f, 1.0f);
			return clampedValue * clampedValue * (3.0f - 2.0f * clampedValue);
        }
        static float power(const float& p_value, const float& p_power)
        {
			float clampedValue = std::clamp(p_value, 0.0f, 1.0f);
			return std::pow(clampedValue, p_power);
        }
        static float cosine(const float& p_value)
        {
			float clampedValue = std::clamp(p_value, 0.0f, 1.0f);
			return 0.5f * (1.0f - std::cos(clampedValue * 3.1415926535f));
        }
        static float exponential(const float& p_value, const float& p_exponent)
        {
			float clampedValue = std::clamp(p_value, 0.0f, 1.0f);
			return (std::exp(p_exponent * clampedValue) - 1.0f) / (std::exp(p_exponent) - 1.0f);
        }
    };

	float distanceToEdge(int p_x, int p_y)
	{	
		const std::size_t distLeft   = p_x;
		const std::size_t distRight  = (size().x - 1u) - p_x;
		const std::size_t distTop    = p_y;
		const std::size_t distBottom = (size().y - 1u) - p_y;
		
		return (std::min(std::min(distLeft, distRight), std::min(distTop,  distBottom)));
	}

	static constexpr float heightMin = 0.0f;
	static constexpr float heightMax = 100.0f;
	static constexpr float heightRange = heightMax - heightMin;

public:
	HeightMap(const spk::Vector2Int& p_size) :
		spk::Grid2D<int>(p_size, static_cast<int>(heightRange / 2))
	{
		spk::Perlin heightPerlin(123456);
		heightPerlin.setFrequency(0.005f);
		heightPerlin.setOctaves(5);

		const std::size_t maxThick = std::min<std::size_t>(size().x, size().y) / 2;
        const float thickness = std::min(100.0f, static_cast<float>(maxThick));

		for (size_t x = 0; x < size().x; x++)
		{
			auto row = this->operator[](x);
			for (size_t y = 0; y < size().y; y++)
			{
				float h = heightPerlin.sample2D(x, y, heightMin, heightMax);

				float edgeOffset = FalloffFunction::exponential(1.0f - std::min(1.0f, distanceToEdge(x, y) / thickness), 2) * heightRange;

				h = std::clamp(h - edgeOffset, heightMin, heightMax);
                row[y] = static_cast<int>(std::lround(h));
			}
		}
	}
};
 
class MapGenerator : public spk::Grid2D<spk::Tile<PlaygroundTileFlag>::ID>
{
public:
	using TileID = spk::Tile<PlaygroundTileFlag>::ID;

private:
	spk::Grid2D<int> _generateLandShape()
	{
		spk::Grid2D<int> result(size());

		for (size_t x = 0; x < result.size().x; x++)
		{
			auto row = result[x];
			
			for (size_t y = 0; y < result.size().y; y++)
			{
				
			}
		}

		return (result);
	}

public:
	explicit MapGenerator(const spk::Vector2UInt &p_size) :
		spk::Grid2D<spk::Tile<PlaygroundTileFlag>::ID>(p_size, 36)
	{
		auto heightMap = HeightMap(p_size);

		auto landShape = _generateLandShape();
	}
};

class MapGeneratorVisualizer : public spk::Widget
{
private:
	std::unordered_map<MapGenerator::TileID, spk::Color> _tileIdToColor = {
		{0, spk::Color(190, 38, 51)},	 // Fire biome - Gym
		{1, spk::Color(227, 114, 122)},	 // Fire biome - City
		{2, spk::Color(238, 170, 174)},	 // Fire biome - Territory
		{3, spk::Color(207, 207, 207)},	 // Air biome - Gym
		{4, spk::Color(230, 230, 230)},	 // Air biome - City
		{5, spk::Color(255, 255, 255)},	 // Air biome - Territory
		{6, spk::Color(49, 162, 242)},	 // Water biome - Gym
		{7, spk::Color(90, 179, 245)},	 // Water biome - City
		{8, spk::Color(156, 209, 249)},	 // Water biome - Territory
		{9, spk::Color(73, 60, 43)},	 // Stone biome - Gym
		{10, spk::Color(123, 101, 72)},	 // Stone biome - City
		{11, spk::Color(185, 164, 136)}, // Stone biome - Territory
		{12, spk::Color(68, 137, 26)},	 // Plant biome - Gym
		{13, spk::Color(96, 195, 37)},	 // Plant biome - City
		{14, spk::Color(157, 228, 115)}, // Plant biome - Territory
		{15, spk::Color(27, 38, 50)},	 // Ghost biome - Gym
		{16, spk::Color(57, 80, 106)},	 // Ghost biome - City
		{17, spk::Color(120, 148, 182)}, // Ghost biome - Territory
		{18, spk::Color(235, 137, 49)},	 // Fight biome - Gym
		{19, spk::Color(239, 161, 90)},	 // Fight biome - City
		{20, spk::Color(245, 199, 156)}, // Fight biome - Territory
		{21, spk::Color(254, 240, 71)},	 // Electricity biome - Gym
		{22, spk::Color(254, 246, 145)}, // Electricity biome - City
		{23, spk::Color(254, 250, 189)}, // Electricity biome - Territory
		{24, spk::Color(98, 98, 98)},	 // Steel biome - Gym
		{25, spk::Color(120, 120, 120)}, // Steel biome - City
		{26, spk::Color(153, 153, 153)}, // Steel biome - Territory
		{27, spk::Color(80, 13, 139)},	 // Psy biome - Gym
		{28, spk::Color(120, 19, 204)},	 // Psy biome - City
		{29, spk::Color(177, 97, 240)},	 // Psy biome - Territory
		{30, spk::Color(165, 28, 195)},	 // Fairy biome - Gym
		{31, spk::Color(198, 54, 226)},	 // Fairy biome - City
		{32, spk::Color(222, 134, 238)}, // Fairy biome - Territory
		{33, spk::Color(91, 246, 214)},	 // Ice biome - Gym
		{34, spk::Color(157, 250, 231)}, // Ice biome - City
		{35, spk::Color(196, 252, 241)}, // Ice biome - Territory
		{36, spk::Color(0, 87, 132)},	 // Deep sea
		{37, spk::Color(0, 135, 208)}	 // Shallow sea
	};

	spk::Texture _mapAsImage;

	spk::TexturePainter _textureRenderer;

	void _generateTexture(const MapGenerator& p_mapGenerator)
	{
		std::vector<std::uint32_t> pixels(geometry().size.x * geometry().size.y);

		const spk::Color fallback(255, 0, 255);

		std::vector<std::optional<uint32_t>> colors;

		for (std::size_t x = 0; x < geometry().size.x; ++x)
		{
			auto row = p_mapGenerator[x];
			for (std::size_t y = 0; y < geometry().size.y; ++y)
			{
				const auto id = row[y];

				if (colors.size() <= id)
				{
					colors.resize(id + 1);
				}

				if (colors[id].has_value() == false)
				{
					colors[id] = (_tileIdToColor.contains(id) == true ? _tileIdToColor[id].toInt() : fallback.toInt());
				}

				pixels[y * geometry().size.x + x] = colors[id].value();
			}
		}

		_mapAsImage.setPixels(reinterpret_cast<const std::uint8_t *>(pixels.data()), geometry().size, spk::Texture::Format::RGBA);
	}

	void _onGeometryChange() override
	{
		MapGenerator mapData(geometry().size);

		_generateTexture(mapData);

		_textureRenderer.clear();
		_textureRenderer.prepare(geometry(), spk::Texture::Section::whole, layer() + 0.01f);
		_textureRenderer.validate();
	}

	void _onPaintEvent(spk::PaintEvent &p_event) override
	{
		_textureRenderer.render();
	}

public:
	MapGeneratorVisualizer(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent)
	{
		_mapAsImage.setProperties(spk::Texture::Filtering::Nearest, spk::Texture::Wrap::ClampToBorder, spk::Texture::Mipmap::Disable);
		_textureRenderer.setTexture(&_mapAsImage);
	}
};

int main()
{
	spk::GraphicalApplication app;
	auto window = app.createWindow(L"TacticalActionArenaGame", {{0, 0}, {1000, 800}});
	window->setFullscreen(true);
	window->setUpdateTimer(0);

	MapGeneratorVisualizer visualizer = MapGeneratorVisualizer(L"MapGeneratorVisualizer", window->widget());
	visualizer.setGeometry(window->geometry());
	visualizer.activate();

	return (app.run());
}
