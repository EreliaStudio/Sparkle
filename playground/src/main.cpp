#include <sparkle.hpp>

struct Vertex
{
	spk::Vector3 position;
	spk::Vector2 uv;
	spk::Vector3 normal;

	bool operator==(const Vertex& other) const
	{
		return position == other.position &&
			   uv == other.uv &&
			   normal == other.normal;
	}

	bool operator<(const Vertex& other) const
	{
		if (position != other.position)
			return position < other.position;
		if (uv != other.uv)
			return uv < other.uv;
		return normal < other.normal;
	}
};

namespace std
{
	template<>
	struct hash<Vertex>
	{
		std::size_t operator()(const Vertex& v) const
		{
			std::size_t h1 = std::hash<spk::Vector3>{}(v.position);
			std::size_t h2 = std::hash<spk::Vector2>{}(v.uv);
			std::size_t h3 = std::hash<spk::Vector3>{}(v.normal);

			std::size_t seed = h1;
			seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed ^= h3 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			return seed;
		}
	};
}

template <typename TVertexType, typename = std::enable_if_t<spk::IsOBJable<TVertexType>::value>>
class TMesh
{
public:
	using Vertex = TVertexType;

	struct Triangle
	{
		Vertex a;
		Vertex b;
		Vertex c;
	};

	struct Quad
	{
		Vertex a;
		Vertex b;
		Vertex c;
		Vertex d;
	};

	struct Buffer
	{
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indexes;
	};

private:
	using Shape = std::variant<Triangle, Quad>;

	std::vector<Shape> _shapes;

	mutable bool _needBake = false;
	mutable Buffer _buffer;

	mutable std::mutex _mutex; 
	
	unsigned int _getOrInsertVertex(const Vertex& v, std::unordered_map<Vertex, unsigned int>& vertexMap) const
	{
		auto it = vertexMap.find(v);
		if (it != vertexMap.end())
		{
			return it->second;
		}
		else
		{
			const unsigned int newIndex = static_cast<unsigned int>(_buffer.vertices.size());
			_buffer.vertices.push_back(v);
			vertexMap[v] = newIndex;
			return newIndex;
		}
	}

	void _bakeTriangle(const Triangle& t, std::unordered_map<Vertex, unsigned int>& vertexMap) const
	{
		_buffer.indexes.push_back(_getOrInsertVertex(t.a, vertexMap));
		_buffer.indexes.push_back(_getOrInsertVertex(t.b, vertexMap));
		_buffer.indexes.push_back(_getOrInsertVertex(t.c, vertexMap));
	}

	void _bakeQuad(const Quad& q, std::unordered_map<Vertex, unsigned int>& vertexMap) const
	{
		unsigned int ia = _getOrInsertVertex(q.a, vertexMap);
		unsigned int ib = _getOrInsertVertex(q.b, vertexMap);
		unsigned int ic = _getOrInsertVertex(q.c, vertexMap);
		unsigned int id = _getOrInsertVertex(q.d, vertexMap);

		_buffer.indexes.push_back(ia);
		_buffer.indexes.push_back(ib);
		_buffer.indexes.push_back(ic);

		_buffer.indexes.push_back(ia);
		_buffer.indexes.push_back(ic);
		_buffer.indexes.push_back(id);
	}

public:
	TMesh() = default;

	void addShape(const Vertex& p_a, const Vertex& p_b, const Vertex& p_c)
	{
		std::lock_guard lock(_mutex);
		_shapes.push_back(Triangle{ p_a, p_b, p_c });
		_needBake = true;
	}

	void addShape(const Triangle& p_triangle)
	{
		std::lock_guard lock(_mutex);
		_shapes.push_back(p_triangle);
		_needBake = true;
	}

	void addShape(const Vertex& p_a, const Vertex& p_b, const Vertex& p_c, const Vertex& p_d)
	{
		std::lock_guard lock(_mutex);
		_shapes.push_back(Quad{ p_a, p_b, p_c, p_d });
		_needBake = true;
	}

	void addShape(const Quad& p_quad)
	{
		std::lock_guard lock(_mutex);
		_shapes.push_back(p_quad);
		_needBake = true;
	}

	void popShape()
	{
		std::lock_guard lock(_mutex);
		if (!_shapes.empty())
		{
			_shapes.pop_back();
			_needBake = true;
		}
	}

	void clear()
	{
		std::lock_guard lock(_mutex);
		_shapes.clear();
		_buffer.vertices.clear();
		_buffer.indexes.clear();
		_needBake = false;
	}

	const std::vector<Shape>& shapes() const
	{
		return _shapes;
	}

	auto begin() const { return _shapes.begin(); }
	auto end() const { return _shapes.end(); }

	bool hasPendingChanges() const
	{
		std::lock_guard lock(_mutex);
		return _needBake;
	}

	void bake() const
	{
		std::lock_guard lock(_mutex);

		_buffer.vertices.clear();
		_buffer.indexes.clear();

		std::unordered_map<Vertex, unsigned int> vertexMap;

		for (const auto& s : _shapes)
		{
			if (std::holds_alternative<Triangle>(s))
			{
				_bakeTriangle(std::get<Triangle>(s), vertexMap);
			}
			else
			{
				_bakeQuad(std::get<Quad>(s), vertexMap);
			}
		}

		_needBake = false;
	}

	void exportToOBJ(const std::filesystem::path& p_path) const;
	void importFromOBJ(const std::filesystem::path& p_path);

	const Buffer& buffer() const
	{
		if (_needBake)
		{
			bake();
		}
		return _buffer;
	}
};

using Mesh = TMesh<Vertex>;

class MeshRenderer : public spk::Component
{
private:
	class Painter
	{
	};
};

class TmpEntity : public spk::Entity
{
private:
public:
};

class TestWidget : public spk::Screen
{
private:
	spk::GameEngineWidget _gameEngineWidget;
	spk::GameEngine _engine;

	void _onGeometryChange()
	{
		_gameEngineWidget.setGeometry({}, geometry().size);
	}

public:
	TestWidget(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Screen(p_name, p_parent),
		_gameEngineWidget(p_name + L"/GameEngineWidget", this)
	{
		_gameEngineWidget.setGameEngine(&_engine);
		_gameEngineWidget.activate();
	}
};

int main()
{
	spk::GraphicalApplication app;

	spk::SafePointer<spk::Window> win = app.createWindow(L"Playground", {{0, 0}, {800, 600}});

	TestWidget testWidget = TestWidget(L"TestWidget", win->widget());
	testWidget.setGeometry({0, 0}, win->geometry().size);
	testWidget.activate();

	return (app.run());
}