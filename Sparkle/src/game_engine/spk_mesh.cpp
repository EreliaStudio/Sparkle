#include "game_engine/spk_mesh.hpp"

namespace spk
{
	void Mesh::Data::insert(const Vector2 &p_data)
	{
		vertexes.push_back(p_data.x);
		vertexes.push_back(p_data.y);
	}

	void Mesh::Data::insert(const Vector3 &p_data)
	{
		vertexes.push_back(p_data.x);
		vertexes.push_back(p_data.y);
		vertexes.push_back(p_data.z);
	}

	std::size_t Mesh::VertexHash::operator()(const Mesh::Data::Vertex &p_vertex) const
	{
		std::size_t h1 = std::hash<float>{}(p_vertex.position.x);
		std::size_t h2 = std::hash<float>{}(p_vertex.position.y);
		std::size_t h3 = std::hash<float>{}(p_vertex.position.z);
		std::size_t h4 = std::hash<float>{}(p_vertex.uvs.x);
		std::size_t h5 = std::hash<float>{}(p_vertex.uvs.y);
		std::size_t h6 = std::hash<float>{}(p_vertex.normal.x);
		std::size_t h7 = std::hash<float>{}(p_vertex.normal.y);
		std::size_t h8 = std::hash<float>{}(p_vertex.normal.z);
		return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3) ^ (h5 << 4) ^ (h6 << 5) ^ (h7 << 6) ^ (h8 << 7);
	}

	bool Mesh::VertexEqual::operator()(const Mesh::Data::Vertex &p_lhs, const Mesh::Data::Vertex &p_rhs) const
	{
		return p_lhs.position == p_rhs.position && p_lhs.uvs == p_rhs.uvs && p_lhs.normal == p_rhs.normal;
	}

	Mesh::Data Mesh::_bakeWithNormals() const
	{
		Data result;

		result.vertexSize = 8 * sizeof(float);
		result.vertexes.reserve(8 * (_indexes.size() / 3));
		result.indexes.reserve(1 * (_indexes.size() / 3));

		std::unordered_map<Data::Vertex, unsigned int, VertexHash, VertexEqual> uniqueVertexMap;

		size_t nbVertex = 0;
		for (size_t i = 0; i < _indexes.size(); i += 3)
		{
			unsigned int pointIndex = _indexes[i];
			unsigned int uvsIndex = _indexes[i + 1];
			unsigned int normalIndex = _indexes[i + 2];

			if (_points.size() <= pointIndex)
				throw std::runtime_error("Can't obtain point [" + std::to_string(pointIndex) + "] inside a vector of [" + std::to_string(_points.size()) + "] points");

			if (_uvs.size() <= uvsIndex)
				throw std::runtime_error("Can't obtain uvs [" + std::to_string(uvsIndex) + "] inside a vector of [" + std::to_string(_uvs.size()) + "] uvs");

			if (_normals.size() <= normalIndex)
				throw std::runtime_error("Can't obtain normal [" + std::to_string(normalIndex) + "] inside a vector of [" + std::to_string(_normals.size()) + "] normals");

			Data::Vertex vertex = {
				_points[pointIndex],
				_uvs[uvsIndex],
				_normals[normalIndex]};

			auto [it, inserted] = uniqueVertexMap.try_emplace(vertex, uniqueVertexMap.size());

			if (inserted == true)
			{
				result.insert(_points[pointIndex]);

				result.insert(_uvs[uvsIndex]);

				result.insert(_normals[normalIndex]);
			}
			result.indexes.push_back(it->second);
		}

		return result;
	}

	Mesh::Data Mesh::_bakeWithoutNormals() const
	{
		Data result;

		result.vertexSize = 5 * sizeof(float);
		result.vertexes.reserve(5 * (_indexes.size() / 3));
		result.indexes.reserve(1 * (_indexes.size() / 3));

		std::unordered_map<Data::Vertex, unsigned int, VertexHash, VertexEqual> uniqueVertexMap;

		size_t nbVertex = 0;
		for (size_t i = 0; i < _indexes.size(); i += 3)
		{
			unsigned int pointIndex = _indexes[i];
			unsigned int uvsIndex = _indexes[i + 1];

			if (_points.size() <= pointIndex)
				throw std::runtime_error("Can't obtain point [" + std::to_string(pointIndex) + "] inside a vector of [" + std::to_string(_points.size()) + "] points");

			if (_uvs.size() <= uvsIndex)
				throw std::runtime_error("Can't obtain uvs [" + std::to_string(uvsIndex) + "] inside a vector of [" + std::to_string(_uvs.size()) + "] uvs");

			Data::Vertex vertex = {
				_points[pointIndex],
				_uvs[uvsIndex],
				Vector3(0, 0, 0)};

			auto [it, inserted] = uniqueVertexMap.try_emplace(vertex, uniqueVertexMap.size());

			if (inserted == true)
			{
				result.insert(_points[pointIndex]);

				result.insert(_uvs[uvsIndex]);
			}
			result.indexes.push_back(it->second);
		}

		return result;
	}

	Mesh::Mesh()
	{

	}

	Mesh::Mesh(const Mesh& p_mesh) :
		_points(p_mesh._points),
		_uvs(p_mesh._uvs),
		_normals(p_mesh._normals),
		_indexes(p_mesh._indexes),
		_editionNotifier()
	{

	}

	Mesh& Mesh::operator=(const Mesh& p_mesh)
	{
		if (this != &p_mesh)
		{
			_points = p_mesh._points;
			_uvs = p_mesh._uvs;
			_normals = p_mesh._normals;
			_indexes = p_mesh._indexes;
		}
		return *this;
	}

	Mesh::Mesh(Mesh&& p_mesh) :
		_editionNotifier(std::move(p_mesh._editionNotifier)),
		_points(std::move(p_mesh._points)),
		_uvs(std::move(p_mesh._uvs)),
		_normals(std::move(p_mesh._normals)),
		_indexes(std::move(p_mesh._indexes))
	{

	}
	
	Mesh& Mesh::operator = (Mesh&& p_mesh)
	{
		if (this != &p_mesh)
		{
			_editionNotifier = std::move(p_mesh._editionNotifier);
			_points = std::move(p_mesh._points);
			_uvs = std::move(p_mesh._uvs);
			_normals = std::move(p_mesh._normals);

			_indexes = std::move(p_mesh._indexes);
		}

		return (*this);
	}

	std::unique_ptr<Notifier::Contract> Mesh::subscribeToEdition(const Notifier::Callback& p_callback) const
	{
		return (_editionNotifier.subscribe(p_callback));
	}
	
	void Mesh::validate()
	{
		_editionNotifier.notify_all();
	}

	void Mesh::reservePoints(size_t p_expectedNbPoints)
	{
		_points.reserve(p_expectedNbPoints);
	}

	void Mesh::addPoint(const Vector3 &p_point)
	{
		_points.push_back(p_point);
	}

	void Mesh::reserveUVs(size_t p_expectedNbUVs)
	{
		_uvs.reserve(p_expectedNbUVs);
	}

	void Mesh::addUVs(const Vector2 &p_uvs)
	{
		_uvs.push_back(p_uvs);
	}

	void Mesh::reserveNormal(size_t p_expectedNbNormal)
	{
		_normals.reserve(p_expectedNbNormal);
	}

	void Mesh::addNormal(const Vector3 &p_normals)
	{
		_normals.push_back(p_normals);
	}

	void Mesh::reserveIndex(size_t p_expectedNbIndex)
	{
		_indexes.reserve(p_expectedNbIndex);
	}

	void Mesh::addIndex(unsigned int p_pointIndex, unsigned int p_uvsIndex, unsigned int p_normalIndex)
	{
		_indexes.push_back(p_pointIndex);
		_indexes.push_back(p_uvsIndex);
		_indexes.push_back(p_normalIndex);
	}

	void Mesh::bakeNormals()
	{
		_normals.clear();

		for (size_t i = 0; i < _indexes.size(); i += 9)
		{
			Vector3 pointA = _points[_indexes[i + 0]];
			Vector3 pointB = _points[_indexes[i + 3]];
			Vector3 pointC = _points[_indexes[i + 6]];

			Vector3 normal = (pointB - pointA).cross(pointC - pointA);

			_indexes[i + 2] = _normals.size();
			_indexes[i + 5] = _normals.size();
			_indexes[i + 8] = _normals.size();

			_normals.push_back(normal);
		}
	}

	Mesh::Data Mesh::bake() const
	{
		if (_indexes.size() < 3)
		{
			std::cout << "Indexes : " << _indexes.size() << std::endl;
			return (Data());
		}

		if (_indexes[2] != std::numeric_limits<unsigned int>::max())
			return (_bakeWithNormals());
		else
			return (_bakeWithoutNormals());
	}

	spk::Mesh createSpriteMesh()
	{
		spk::Mesh result;

		result.addPoint(spk::Vector3(0.5f, 0.0f, 0.5f));
		result.addPoint(spk::Vector3(-0.5f, 0.0f, 0.5f));
		result.addPoint(spk::Vector3(0.5f, 0.0f, -0.5f));
		result.addPoint(spk::Vector3(-0.5f, 0.0f, -0.5f));
		
		result.addUVs(spk::Vector2(1, 1));
		result.addUVs(spk::Vector2(0, 1));
		result.addUVs(spk::Vector2(1, 0));
		result.addUVs(spk::Vector2(0, 0));

		result.addIndex(0, 0);
		result.addIndex(1, 1);
		result.addIndex(2, 2);
		result.addIndex(2, 2);
		result.addIndex(1, 1);
		result.addIndex(3, 3);

		return (result);
	}
}