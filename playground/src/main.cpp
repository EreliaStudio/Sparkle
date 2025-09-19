#include <sparkle.hpp>

class ShaderAtlas : public spk::Singleton<ShaderAtlas>
{
	friend class spk::Singleton<ShaderAtlas>;
private:
	ShaderAtlas()
	{
		_shaderObjectFactoryInstanciator->add(spk::JSON::File("playground/resources/shaders/shader_object_descriptors.json").root());		
	
		addShader(L"MapPainterShader", createMapPainterShader());
	}

	spk::Lumina::ShaderObjectFactory::Instanciator _shaderObjectFactoryInstanciator;
	std::unordered_map<std::wstring, spk::Lumina::Shader> _shaders;

	spk::Lumina::Shader createMapPainterShader()
	{
		spk::Lumina::Shader result = spk::Lumina::Shader(
				spk::FileUtils::readFileAsString("playground/resources/shaders/map_painter.vert"),
				spk::FileUtils::readFileAsString("playground/resources/shaders/map_painter.vert")
			);

		result.addAttribute(0, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector2);

		result.addUBO(L"CameraUBO", _shaderObjectFactoryInstanciator->ubo(L"CameraUBO"), spk::Lumina::Shader::Mode::Constant);
		result.addUBO(L"ChunkDataUBO", _shaderObjectFactoryInstanciator->ubo(L"ChunkDataUBO"), spk::Lumina::Shader::Mode::Attribute);
		result.addSSBO(L"ColorPaletteSSBO", _shaderObjectFactoryInstanciator->ssbo(L"ColorPaletteSSBO"), spk::Lumina::Shader::Mode::Constant);

		return (result);
	}

public:
	void addShader(const std::wstring& p_name, const spk::Lumina::Shader& p_shader)
	{
		if (_shaders.contains(p_name) == true)
		{
			GENERATE_ERROR("Shader [" + spk::StringUtils::wstringToString(p_name) + "] already created");
		}

		_shaders[p_name] = p_shader;
	}

	spk::Lumina::Shader& shader(const std::wstring& p_name)
	{
		if (_shaders.contains(p_name) == false)
		{
			GENERATE_ERROR("Shader [" + spk::StringUtils::wstringToString(p_name) + "] not loaded.");
		}

		return (_shaders.at(p_name));
	}
	
	static spk::OpenGL::SamplerObject &sampler(const std::wstring &p_name)
	{
		return spk::Lumina::Shader::Constants::sampler(p_name);
	}
	
	static spk::OpenGL::UniformBufferObject &ubo(const std::wstring &p_name)
	{
		return spk::Lumina::Shader::Constants::ubo(p_name);
	}

	static spk::OpenGL::ShaderStorageBufferObject &ssbo(const std::wstring &p_name)
	{
		return spk::Lumina::Shader::Constants::ssbo(p_name);
	}
};

class Camera
{
private:
	ShaderAtlas::Instanciator _shaderAtlasInstanciator;
	spk::OpenGL::UniformBufferObject &_cameraUBO;

	static inline spk::SafePointer<const Camera> _mainCamera = nullptr;
	spk::Vector2 _position;
	spk::Camera _innerCamera;

	spk::Vector2Int _screenSize;
	spk::Vector2Int _cellSizeInPixels = {32, 32};

	void _updateOrthographicView()
	{
		if (_cellSizeInPixels.x <= 0 || _cellSizeInPixels.y <= 0)
		{
			GENERATE_ERROR("Cell size in pixels can't be defined to 0 or lower");
		}
		if (_screenSize.x <= 0 || _screenSize.y <= 0)
		{
			GENERATE_ERROR("Screen size can't be defined to 0 or lower");
		}

		spk::Vector2 nbCellOnScreen = spk::Vector2(_screenSize) / spk::Vector2(_cellSizeInPixels);

		_innerCamera.setOrthographic(-nbCellOnScreen.x / 2, nbCellOnScreen.x / 2, -nbCellOnScreen.y / 2, nbCellOnScreen.y / 2, 0.1f, 1000.0f);
		_cameraUBO[L"projectionMatrix"] = _innerCamera.projectionMatrix();
		_cameraUBO.validate();
	}

	void _updateView()
	{
		_cameraUBO[L"viewMatrix"] = spk::Matrix4x4::translation(spk::Vector3(-_position.x, -_position.y, -10.0f));
		_cameraUBO.validate();
	}

public:
	Camera() :
		_cameraUBO(_shaderAtlasInstanciator->ubo(L"CameraUBO"))
	{
		place({0, 0});
	}

	void setAsMainCamera() const
	{
		_mainCamera = this;
	}

	static spk::SafePointer<const Camera> mainCamera()
	{
		return (_mainCamera);
	}

	void initialize(const spk::Vector2Int &p_screenSize, const spk::Vector2Int &p_cellSizeInPixels)
	{
		_screenSize = p_screenSize;
		_cellSizeInPixels = p_cellSizeInPixels;
		_updateOrthographicView();
	}

	void setScreenSize(const spk::Vector2Int &p_screenSize)
	{
		_screenSize = p_screenSize;
		_updateOrthographicView();
	}

	void setCellSize(const spk::Vector2Int &p_cellSizeInPixels)
	{
		_cellSizeInPixels = p_cellSizeInPixels;
		_updateOrthographicView();
	}

	void place(const spk::Vector2 &p_position)
	{
		_position = p_position;
		_updateView();
	}

	const spk::Vector2 &position() const
	{
		return (_position);
	}

	spk::Vector2 screenToWorldPosition(const spk::Vector2Int& p_screenPosition) const
	{
		if (_screenSize.x <= 0 || _screenSize.y <= 0)
		{
			GENERATE_ERROR("Screen size must be set before calling screenToWorldPosition");
		}

		const float ndcX = (p_screenPosition.x / static_cast<float>(_screenSize.x)) * 2.0f - 1.0f;
		const float ndcY = 1.0f - (p_screenPosition.y / static_cast<float>(_screenSize.y)) * 2.0f; 

		spk::Vector4 ndcPos(ndcX, ndcY, -1.0f, 1.0f);
		spk::Vector4 camPos4 = _innerCamera.inverseProjectionMatrix() * ndcPos;

		return spk::Vector2(camPos4.x + _position.x, camPos4.y + _position.y);
	}

	spk::Vector2 ndcToWorldPosition(const spk::Vector2& p_ndc) const
	{
		if (_screenSize.x <= 0 || _screenSize.y <= 0)
		{
			GENERATE_ERROR("Screen size must be set before calling screenToWorldPosition");
		}

		spk::Vector4 camPos4 = _innerCamera.inverseProjectionMatrix() * spk::Vector4(p_ndc, -1.0f, 1.0f);

		return spk::Vector2(camPos4.x + _position.x, camPos4.y + _position.y);
	}
};

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
	ShaderAtlas::Instanciator _shaderAtlasInstanciator;
	spk::OpenGL::ShaderStorageBufferObject& ssbo;

public:
	ColorPalette() :
		ssbo(_shaderAtlasInstanciator->ssbo(L"ColorPaletteSSBO"))
	{

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
		struct Painter
		{
		private:
			ShaderAtlas::Instanciator _shaderAtlasInstanciator;
			spk::Lumina::Shader& _shader;
			spk::Lumina::Shader::Object _object;
			spk::OpenGL::BufferSet &_bufferSet;
			spk::OpenGL::UniformBufferObject& _chunkDataUBO;

		public:
			Painter() :
				_shader(_shaderAtlasInstanciator->shader(L"MapPainterShader")),
				_object(_shader.createObject()),
				_bufferSet(_object.bufferSet()),
				_chunkDataUBO(_object.ubo(L"ChunkDataUBO"))
			{
				_bufferSet.layout() << spk::Vector2(0, 0);
				_bufferSet.layout() << spk::Vector2(1, 0);
				_bufferSet.layout() << spk::Vector2(1, 1);
				_bufferSet.layout() << spk::Vector2(0, 1);

				_bufferSet.indexes() << 0 << 1 << 2 << 2 << 1 << 3;

				_object.setNbInstance(Chunk::Width * Chunk::Height);
			}

			void render()
			{
				_object.render();
			}

			void updateData(const spk::Vector2Int& p_chunkCoordinates, const Chunk& p_chunk)
			{
				_chunkDataUBO[L"position"] = p_chunkCoordinates;
				_chunkDataUBO[L"data"] = p_chunk.content;
			}
		};

		static inline const size_t Width = 16;
		static inline const size_t Height = 16;
		static inline const spk::Vector2Int Size = spk::Vector2Int(Width, Height);

		ColorPalette::ID content[Width * Height];

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
	ColorPalette _palette;
	std::unordered_map<spk::Vector2Int, Chunk> _chunks;

public:
	ColorPalette& palette()
	{
		return (_palette);
	}

	const ColorPalette& palette() const
	{
		return (_palette);
	}

	static spk::Vector2Int worldToChunkCoordinates(const spk::Vector2& p_worldCoordinates)
	{
		return (spk::Vector2::floor(p_worldCoordinates / spk::Vector2(Chunk::Size)));
	}

	static spk::Vector2Int absoluteToRelativeCoordinates(const spk::Vector2& p_worldCoordinates)
	{
		return (spk::Vector2Int(spk::positiveModulo(p_worldCoordinates.x, Chunk::Width),
								spk::positiveModulo(p_worldCoordinates.y, Chunk::Width)));
	}

	Chunk& chunk(const spk::Vector2Int& coordinates)
	{
		return (_chunks[coordinates]);
	}

	const Chunk& chunk(const spk::Vector2Int& coordinates) const
	{
		return (_chunks.at(coordinates));
	}
};

class Context : public spk::Singleton<Context>
{
	friend class spk::Singleton<Context>;

private:
	Context()
	{
		camera.setAsMainCamera();
	}

public:
	Camera camera;
	Map map;
};

enum class Event
{
	RefreshChunkVisibility
};

using EventDispatcher = spk::EventDispatcher<Event>;

class MapRenderer : public spk::Widget
{
private:
	Context::Instanciator _contextInstanciator;
	EventDispatcher::Instanciator _eventDispatcherInstanciator;

	using ChunkPainterPool = spk::Pool<Map::Chunk::Painter>;
	ChunkPainterPool _painterPool;
	std::unordered_map<spk::Vector2Int, ChunkPainterPool::Object> _usedPainters; 

	EventDispatcher::Contract<> _refreshChunkVisibilityContract;

	void _onGeometryChange()
	{

	}

	void _refreshChunkVisibility()
	{
		DEBUG_LINE();
		spk::Vector2Int minCoord = Map::worldToChunkCoordinates(Camera::mainCamera()->ndcToWorldPosition({-1, -1}));
		spk::Vector2Int maxCoord = Map::worldToChunkCoordinates(Camera::mainCamera()->ndcToWorldPosition({1, 1}));

		std::set<spk::Vector2Int> neededCoordinates;

		DEBUG_LINE();
		for (int x = minCoord.x; x < maxCoord.x; x++)
		{
			for (int y = minCoord.y; y < maxCoord.y; y++)
			{
				neededCoordinates.insert(spk::Vector2Int(x, y));
			}
		}
		DEBUG_LINE();

		for (auto it = _usedPainters.begin(); it != _usedPainters.end(); )
		{
			if (!neededCoordinates.contains(it->first))
			{
				it = _usedPainters.erase(it);
			}
			else
			{
				++it;
			}
		}
		DEBUG_LINE();

		DEBUG_LINE();
		_usedPainters.reserve(_usedPainters.size() + neededCoordinates.size());
		DEBUG_LINE();
		for (const auto& coordinates : neededCoordinates)
		{
		DEBUG_LINE();
			if (_usedPainters.find(coordinates) == _usedPainters.end())
			{
		DEBUG_LINE();
				auto obj = _painterPool.obtain();
		DEBUG_LINE();
				obj->updateData(coordinates, Context::instance()->map.chunk(coordinates));
		DEBUG_LINE();
				_usedPainters.emplace(coordinates, std::move(obj));
		DEBUG_LINE();
			}
		}

		DEBUG_LINE();
		_requestPaint();
	}

	void _onPaintEvent(spk::PaintEvent& p_event)
	{
		for (auto& [coordinates, painter] : _usedPainters)
		{
			painter->render();
		}
	}

public:
	MapRenderer(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent)
	{
		_refreshChunkVisibilityContract = EventDispatcher::subscribe(Event::RefreshChunkVisibility, [&](){
			_refreshChunkVisibility();
		});
	}
};

class CameraManager : public spk::Widget
{
private:
	Context::Instanciator _contextInstanciator;
	EventDispatcher::Instanciator _eventDispatcherInstanciator;

	void _onGeometryChange()
	{
		DEBUG_LINE();
		try
		{
			Context::instance()->camera.setScreenSize(geometry().size);
		}
		catch (const std::runtime_error& e)
		{
			PROPAGATE_ERROR("Error while setting screen size", e);
		}
		DEBUG_LINE();
		try
		{
			EventDispatcher::emit(Event::RefreshChunkVisibility);
		}
		catch (const std::runtime_error& e)
		{
			PROPAGATE_ERROR("Error uppon RefreshChunkVisibility emition ", e);
		}
		DEBUG_LINE();
	}

public:
	CameraManager(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent)
	{
		Context::instance()->camera.initialize({1000, 1000}, {32, 32});
	}
};

int main()
{
	spk::GraphicalApplication app;
	auto window = app.createWindow(L"TacticalActionArenaGame", {{0, 0}, {800, 600}});
	window->setUpdateTimer(0);

	MapRenderer mapRenderer = MapRenderer(L"MapRenderer", window->widget());
	mapRenderer.setGeometry(window->geometry());
	mapRenderer.activate();

	CameraManager cameraManager = CameraManager(L"CameraManager", window->widget());
	cameraManager.setGeometry(window->geometry());
	cameraManager.activate();

	return (app.run());
}