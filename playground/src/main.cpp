#include "playground.hpp"

enum class PlaygroundTileFlag
{
	None = 0
};

class MapData
{
public:
	using TileID = spk::Tile<PlaygroundTileFlag>::ID;

private:
	template <typename TUnit>
	class Data
	{
	public:
		using Unit = TUnit;
		struct Row
		{
			Unit *ptr;
			size_t len;
			Unit &operator[](size_t y)
			{
				return ptr[y];
			}
			const Unit &operator[](size_t y) const
			{
				return ptr[y];
			}
		};

	private:
		spk::Vector2UInt _size;
		std::vector<Unit> _data;

	public:
		Data(const spk::Vector2UInt &p_size, Unit fill = 36) :
			_size(p_size),
			_data(p_size.x * p_size.y, fill)
		{
		}

		const spk::Vector2UInt& size() const
		{
			return _size;
		}

		Unit &operator()(int x, int y)
		{
			return _data[static_cast<size_t>(x) * _size.y + y];
		}
		const Unit &operator()(int x, int y) const
		{
			return _data[static_cast<size_t>(x) * _size.y + y];
		}

		Row operator[](int x)
		{
			return Row{_data.data() + static_cast<size_t>(x) * _size.y, _size.y};
		}

		Row operator[](int x) const
		{
			return Row{const_cast<Unit *>(_data.data()) + static_cast<size_t>(x) * _size.y, _size.y};
		}

		Unit *data()
		{
			return _data.data();
		}
		const Unit *data() const
		{
			return _data.data();
		}
	};

	Data<TileID> _tileMap;

	Data<int> _generateHeightData()
	{
		spk::Perlin heightPerlin(123456);
		Data<int> result(_tileMap.size());

		for (size_t x = 0; x < result.size().x; x++)
		{
			auto row = result[x];

			for (size_t y = 0; y < result.size().y; y++)
			{
				row[y] = heightPerlin.sample2D(x, y, 0, 100);
			}
		}

		return (result);
	}

	void _generateLandScape(const Data<int>& p_heightMap)
	{
		for (size_t x = 0; x < p_heightMap.size().x; x++)
		{
			auto row = p_heightMap[x];
			auto targetRow = _tileMap[x];

			for (size_t y = 0; y < p_heightMap.size().y; y++)
			{
				if (row[y] > 50)
				{
					targetRow[y] = 0;
				}
			}
		}
	}

public:
	MapData(const spk::Vector2Int& p_size) :
		_tileMap(p_size)
	{
		Data<int> heightMap = _generateHeightData();

		_generateLandScape(heightMap);
	}

	Data<TileID>::Row operator[](int x)
	{
		return _tileMap[x];
	}

	Data<TileID>::Row operator[](int x) const
	{
		return _tileMap[x];
	}
};

class MapDataVisualizer : public spk::Widget
{
private:
	std::unordered_map<MapData::TileID, spk::Color> _tileIdToColor = {
		{0, spk::Color(190, 38, 51)}, // Fire biome - Gym
		{1, spk::Color(227, 114, 122)}, // Fire biome - City
		{2, spk::Color(238, 170, 174)}, // Fire biome - Territory
		{3, spk::Color(207, 207, 207)}, // Air biome - Gym
		{4, spk::Color(230, 230, 230)}, // Air biome - City
		{5, spk::Color(255, 255, 255)}, // Air biome - Territory
		{6, spk::Color(49, 162, 242)}, // Water biome - Gym
		{7, spk::Color(90, 179, 245)}, // Water biome - City
		{8, spk::Color(156, 209, 249)}, // Water biome - Territory
		{9, spk::Color(73, 60, 43)}, // Stone biome - Gym
		{10, spk::Color(123, 101, 72)}, // Stone biome - City
		{11, spk::Color(185, 164, 136)}, // Stone biome - Territory
		{12, spk::Color(68, 137, 26)}, // Plant biome - Gym
		{13, spk::Color(96, 195, 37)}, // Plant biome - City
		{14, spk::Color(157, 228, 115)}, // Plant biome - Territory
		{15, spk::Color(27, 38, 50)}, // Ghost biome - Gym
		{16, spk::Color(57, 80, 106)}, // Ghost biome - City
		{17, spk::Color(120, 148, 182)}, // Ghost biome - Territory
		{18, spk::Color(235, 137, 49)}, // Fight biome - Gym
		{19, spk::Color(239, 161, 90)}, // Fight biome - City
		{20, spk::Color(245, 199, 156)}, // Fight biome - Territory
		{21, spk::Color(254, 240, 71)}, // Electricity biome - Gym
		{22, spk::Color(254, 246, 145)}, // Electricity biome - City
		{23, spk::Color(254, 250, 189)}, // Electricity biome - Territory
		{24, spk::Color(98, 98, 98)}, // Steel biome - Gym
		{25, spk::Color(120, 120, 120)}, // Steel biome - City
		{26, spk::Color(153, 153, 153)}, // Steel biome - Territory
		{27, spk::Color(80, 13, 139)}, // Psy biome - Gym
		{28, spk::Color(120, 19, 204)}, // Psy biome - City
		{29, spk::Color(177, 97, 240)}, // Psy biome - Territory
		{30, spk::Color(165, 28, 195)}, // Fairy biome - Gym
		{31, spk::Color(198, 54, 226)}, // Fairy biome - City
		{32, spk::Color(222, 134, 238)}, // Fairy biome - Territory
		{33, spk::Color(91, 246, 214)}, // Ice biome - Gym
		{34, spk::Color(157, 250, 231)}, // Ice biome - City
		{35, spk::Color(196, 252, 241)}, // Ice biome - Territory
		{36, spk::Color(0, 87, 132)}, // Deep sea
		{37, spk::Color(0, 135, 208)}  // Shallow sea
	};

	spk::Texture _mapAsImage; 

	spk::TexturePainter _textureRenderer;

	void _onGeometryChange() override
	{
		MapData mapData(geometry().size);

		std::vector<std::uint32_t> pixels(geometry().size.x * geometry().size.y);

		const spk::Color fallback(255, 0, 255);

		std::vector<std::optional<uint32_t>> colors;

		for (std::size_t x = 0; x < geometry().size.y; ++x)
		{
			auto row = mapData[x];
			for (std::size_t y = 0; y < geometry().size.x; ++y)
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

				pixels[y * geometry().size.y + x] = colors[id].value();
			}
		}

		_mapAsImage.setPixels(
			reinterpret_cast<const std::uint8_t*>(pixels.data()),
			geometry().size,
			spk::Texture::Format::RGBA,
			spk::Texture::Filtering::Nearest,
			spk::Texture::Wrap::ClampToBorder,
			spk::Texture::Mipmap::Disable
		);
				
		_textureRenderer.clear();
		_textureRenderer.prepare(geometry(), spk::Texture::Section::whole, layer() + 0.01f);
		_textureRenderer.validate();
	}

	void _onPaintEvent(spk::PaintEvent& p_event) override
	{
		_textureRenderer.render();
	}

public:
	MapDataVisualizer(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent)
	{
		_textureRenderer.setTexture(&_mapAsImage);
	}
};

int main()
{
	spk::GraphicalApplication app;
	auto window = app.createWindow(L"TacticalActionArenaGame", {{0, 0}, {1000, 800}});
	window->setUpdateTimer(0);

	MapDataVisualizer visualizer = MapDataVisualizer(L"MapDataVisualizer", window->widget());
	visualizer.setGeometry(window->geometry());
	visualizer.activate();
		
	return (app.run());
}
