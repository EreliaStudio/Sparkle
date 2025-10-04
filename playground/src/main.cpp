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
		spk::Vector2UInt _chunkSize;
		std::vector<Unit> _data;

	public:
		Data(const spk::Vector2UInt &p_size, Unit fill = 36) :
			_size(p_size),
			_chunkSize((static_cast<spk::Vector2>(p_size) / static_cast<spk::Vector2>(Chunk::size)).floor()),
			_data(p_size.x * p_size.y, fill)
		{
		}

		const spk::Vector2UInt& size() const
		{
			return _size;
		}

		const spk::Vector2UInt& chunkSize() const
		{
			return _chunkSize;
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

public:
	MapDataVisualizer(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent)
	{

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
