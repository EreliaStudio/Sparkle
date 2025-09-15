#include <sparkle.hpp>

struct Shape
{
	spk::Vector2 position;
	float angle;
};

class ColorPalette
{
public:
	using ID = uint8_t;
	using HexCode = uint32_t;

private:
	spk::OpenGL::ShaderStorageBufferObject ssbo;

public:
	ColorPalette()
	{
		ssbo = spk::OpenGL::ShaderStorageBufferObject(L"ColorPaletteSSBO", 2, 0, 0, 4, 0);
		ssbo.dynamicArray().addElement(L"hexCode", 0, 4);
	}

	ID identifier(const ColorPalette::HexCode& color)
	{
		for (size_t i = 0; i < ssbo.dynamicArray().nbElement(); i++)
		{
			const HexCode& savedCode = ssbo.dynamicArray()[i].as<HexCode>();
		
			if (savedCode == color)
			{
				return (i);
			}
		}
		ID result = ssbo.dynamicArray().nbElement();
		ssbo.dynamicArray().pushBack(color);
		return (result);
	}

	void validate()
	{
		ssbo.validate();
	}

	void activate()
	{
		ssbo.activate();
	}

	void deactivate()
	{
		ssbo.activate();
	}
};

struct Map
{
public:
	struct Chunk
	{
		ColorPalette::ID content[16 * 16];

		ColorPalette::ID& at(const int& x, const int& y)
		{
			return (content[x + y * 16]);
		}

		const ColorPalette::ID& at(const int& x, const int& y) const
		{
			return (content[x + y * 16]);
		}
	};

private:
	ColorPalette palette;
	std::unordered_map<spk::Vector2Int, Chunk> chunks;

public:
	ColorPalette& palette()
	{
		return (palette);
	}
	
	const ColorPalette& palette() const
	{
		return (palette);
	}

	Chunk& chunk(const spk::Vector2Int& coordinates)
	{
		return (chunks[coordinates]);
	}

	const Chunk& chunk(const spk::Vector2Int& coordinates) const
	{
		return (chunks.at(coordinates));
	}
};

class MainWidget : public spk::Widget
{
private:

public:
	MainWidget(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent)
	{
		
	}
};

int main()
{
	spk::GraphicalApplication app;
	auto window = app.createWindow(L"TacticalActionArenaGame", {{0, 0}, {800, 600}});
	window->setUpdateTimer(0);

	return (app.run());
}