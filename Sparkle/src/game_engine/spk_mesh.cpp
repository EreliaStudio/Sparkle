#include "game_engine/spk_mesh.hpp"

namespace spk
{
	void Mesh::Data::insert(const Vector2 &p_data)
	{
		vertices.push_back(p_data.x);
		vertices.push_back(p_data.y);
	}

	void Mesh::Data::insert(const Vector3 &p_data)
	{
		vertices.push_back(p_data.x);
		vertices.push_back(p_data.y);
		vertices.push_back(p_data.z);
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

	Mesh::Data Mesh::_bake(bool p_pointFlag, bool p_uvFlag, bool p_normalFlag) const
	{
		Data result;

		size_t nbFloat = (p_pointFlag == true ? 3 : 0) + (p_uvFlag == true ? 2 : 0) + (p_normalFlag == true ? 3 : 0);
		result.vertexSize = nbFloat * sizeof(float);
		result.vertices.reserve(nbFloat * (_indexes.size() / 3));
		result.indexes.reserve(1 * (_indexes.size() / 3));

		std::unordered_map<Data::Vertex, unsigned int, VertexHash, VertexEqual> uniqueVertexMap;

		size_t nbVertex = 0;
		for (size_t i = 0; i < _indexes.size(); i += 3)
		{
			unsigned int pointIndex = _indexes[i];
			unsigned int uvsIndex = _indexes[i + 1];
			unsigned int normalIndex = _indexes[i + 2];

			Data::Vertex vertex;
			
			if (p_pointFlag == true)
				vertex.position = _points[pointIndex];
			if (p_uvFlag == true)
				vertex.uvs = _uvs[uvsIndex];
			if (p_normalFlag == true)
				vertex.normal = _normals[normalIndex];

			auto [it, inserted] = uniqueVertexMap.try_emplace(vertex, uniqueVertexMap.size());

			if (inserted == true)
			{
				if (p_pointFlag == true)
					result.insert(_points[pointIndex]);

				if (p_uvFlag == true)
					result.insert(_uvs[uvsIndex]);

				if (p_normalFlag == true)
					result.insert(_normals[normalIndex]);
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

	void Mesh::clear()
	{
		clearPoints();
		clearUVs();
		clearNormals();
		clearIndexes();
	}

	void Mesh::reservePoints(size_t p_expectedNbPoints)
	{
		_points.reserve(p_expectedNbPoints);
	}

	void Mesh::addPoint(const Vector3 &p_point)
	{
		_points.push_back(p_point);
	}

	const std::vector<spk::Vector3>& Mesh::points() const
	{
		return (_points);
	}

	void Mesh::clearPoints()
	{
		_points.clear();
	}

	void Mesh::reserveUVs(size_t p_expectedNbUVs)
	{
		_uvs.reserve(p_expectedNbUVs);
	}

	void Mesh::addUVs(const Vector2 &p_uvs)
	{
		_uvs.push_back(p_uvs);
	}

	const std::vector<spk::Vector2>& Mesh::UVs() const
	{
		return (_uvs);
	}

	void Mesh::clearUVs()
	{
		_uvs.clear();
	}

	void Mesh::reserveNormal(size_t p_expectedNbNormal)
	{
		_normals.reserve(p_expectedNbNormal);
	}

	void Mesh::addNormal(const Vector3 &p_normals)
	{
		_normals.push_back(p_normals);
	}

	const std::vector<spk::Vector3>& Mesh::normals() const
	{
		return (_normals);
	}

	void Mesh::clearNormals()
	{
		_normals.clear();
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

	const std::vector<unsigned int>& Mesh::indexes() const
	{
		return (_indexes);
	}

	void Mesh::clearIndexes()
	{
		_indexes.clear();
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
			return (Data());
		}

		return (_bake(
				_indexes[0] != std::numeric_limits<unsigned int>::max(),
				_indexes[1] != std::numeric_limits<unsigned int>::max(),
				_indexes[2] != std::numeric_limits<unsigned int>::max()
			));
	}

	spk::Mesh createSpriteMesh()
	{
		return (createSpriteMesh(spk::Vector2(0, 0), spk::Vector2(1, 1)));
	}

	spk::Mesh createSpriteMesh(const spk::Vector2& p_anchor, const spk::Vector2& p_size)
	{
		spk::Mesh result;

		result.addPoint(spk::Vector3(0.5f, 0.5f, 0.0f));
		result.addPoint(spk::Vector3(-0.5f, 0.5f, 0.0f));
		result.addPoint(spk::Vector3(0.5f, -0.5f, 0.0f));
		result.addPoint(spk::Vector3(-0.5f, -0.5f, 0.0f));
		
		result.addUVs(p_anchor + spk::Vector2(1, 0) * p_size);
		result.addUVs(p_anchor + spk::Vector2(0, 0) * p_size);
		result.addUVs(p_anchor + spk::Vector2(1, 1) * p_size);
		result.addUVs(p_anchor + spk::Vector2(0, 1) * p_size);

		result.addIndex(0, 0);
		result.addIndex(1, 1);
		result.addIndex(2, 2);
		result.addIndex(2, 2);
		result.addIndex(1, 1);
		result.addIndex(3, 3);

		return (result);
	}
}