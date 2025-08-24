#include <sparkle.hpp>

namespace tmp
{
	class Edge
	{
	private:
		spk::Vector3 _first;
		spk::Vector3 _second;

		spk::Vector3 _direction;

	public:
		Edge(const spk::Vector3 &p_first, const spk::Vector3 &p_second) :
			_first(p_first),
			_second(p_second)
		{
			if (_second == _first)
			{
				GENERATE_ERROR("Can't create an edge of lenght == 0");
			}

			_direction = (_second - _first).normalize();
		}

		const spk::Vector3 &first() const
		{
			return (_first);
		}

		const spk::Vector3 &second() const
		{
			return (_second);
		}

		const spk::Vector3 &direction() const
		{
			return (_direction);
		}

		bool contains(const spk::Vector3 &p_point, bool p_checkAlignment = true) const
		{
			if (p_point == _first)
			{
				return (true);
			}
			const spk::Vector3 v = p_point - _first;

			if (p_checkAlignment == true && v.normalize() != _direction)
			{
				return (false);
			}

			const float t = v.dot(_direction);

			const float len = (_second - _first).dot(_direction);

			return (t >= 0) && (t <= len);
		}

		bool isInverse(const Edge &p_other) const
		{
			return (_first == p_other.second() && _second == p_other.first());
		}

		Edge inverse() const
		{
			return (Edge(_second, _first));
		}

		bool isParallel(const Edge &p_other) const
		{
			return (direction() == p_other.direction() || direction() == p_other.direction().inverse());
		}

		bool isColinear(const Edge &p_other) const
		{
			if (isParallel(p_other) == false)
			{
				return (false);
			}

			spk::Vector3 delta = (p_other._first - _first);

			if (delta == spk::Vector3(0, 0, 0))
			{
				return (true);
			}

			return (std::fabs(delta.normalize().dot(_direction)) == 1);
		}

		bool operator==(const Edge &p_other) const
		{
			return (first() == p_other.first()) && (second() == p_other.second());
		}

		bool operator<(const Edge &p_other) const
		{
			if (first() != p_other.first())
			{
				return (first() < p_other.first());
			}
			return (second() < p_other.second());
		}

		friend std::ostream &operator<<(std::ostream &p_os, const Edge &p_edge)
		{
			p_os << "(" << p_edge.first() << " -> " << p_edge.second() << ")";
			return p_os;
		}

		friend std::wostream &operator<<(std::wostream &p_wos, const Edge &p_edge)
		{
			p_wos << L"(" << p_edge.first() << L" -> " << p_edge.second() << L")";
			return p_wos;
		}
	};

	class Polygon
	{
	private:
		std::vector<tmp::Edge> _edges;

		void _addEdge(const spk::Vector3 &p_a, const spk::Vector3 &p_b)
		{
			_edges.push_back(tmp::Edge(p_a, p_b));
		}

	public:
		const std::vector<tmp::Edge> edges() const
		{
			return (_edges);
		}

		bool isPlanar() const
		{
			if (_edges.size() < 2)
			{
				return (false);
			}

			spk::Vector3 expectedNormal = normal();

			for (size_t i = 2; i < _edges.size(); i++)
			{
				float dot = expectedNormal.dot(_edges[i].direction());

				if (dot != 0)
				{
					return (false);
				}
			}
			return (true);
		}

		spk::Vector3 normal() const
		{
			if (_edges.size() < 2)
			{
				GENERATE_ERROR("Can't generate a normal for a polygon with less than 2 edges");
			}

			return (_edges[0].direction().cross(_edges[1].direction()));
		}

		bool isCoplanar(const Polygon &p_other) const
		{
			if (isPlanar() == false)
			{
				return (false);
			}

			if (p_other.isPlanar() == false)
			{
				return (false);
			}

			spk::Vector3 currentNormal = normal();
			spk::Vector3 otherNormal = p_other.normal();

			return ((currentNormal == otherNormal || currentNormal.inverse() == otherNormal) && currentNormal.dot(p_other.edges()[0].first()) == 0);
		}

		bool isAdjacent(const Polygon &p_other) const
		{
			if (isCoplanar(p_other) == false)
			{
				return (false);
			}

			for (const auto &edgeA : _edges)
			{
				for (const auto &edgeB : p_other.edges())
				{
					if (edgeA.isColinear(edgeB) == true)
					{
						if (edgeA.contains(edgeB.first(), false) || edgeA.contains(edgeB.second(), false) || edgeB.contains(edgeA.first(), false) ||
							edgeB.contains(edgeA.second(), false))
						{
							return (true);
						}
					}
				}
			}
			return (false);
		}

		bool isConvex(float eps = 1e-6f, bool strictly = false) const
		{
			
		}

		bool isOverlapping(const Polygon &p_other) const
		{
			
		}

		static Polygon makeTriangle(const spk::Vector3 &p_a, const spk::Vector3 &p_b, const spk::Vector3 &p_c)
		{
			Polygon result;

			result._addEdge(p_a, p_b);
			result._addEdge(p_b, p_c);
			result._addEdge(p_c, p_a);

			return (result);
		}

		static Polygon makeSquare(const spk::Vector3 &p_a, const spk::Vector3 &p_b, const spk::Vector3 &p_c, const spk::Vector3 &p_d)
		{
			Polygon result;

			result._addEdge(p_a, p_b);
			result._addEdge(p_b, p_c);
			result._addEdge(p_c, p_d);
			result._addEdge(p_d, p_a);

			return (result);
		}

		friend std::ostream &operator<<(std::ostream &p_os, const Polygon &p_poly)
		{
			p_os << "{";
			for (size_t i = 0; i < p_poly._edges.size(); i++)
			{
				p_os << p_poly._edges[i];
				if (i + 1 < p_poly._edges.size())
				{
					p_os << ", ";
				}
			}
			p_os << "}";
			return p_os;
		}

		friend std::wostream &operator<<(std::wostream &p_wos, const Polygon &p_poly)
		{
			p_wos << L"{";
			for (size_t i = 0; i < p_poly._edges.size(); i++)
			{
				p_wos << p_poly._edges[i];
				if (i + 1 < p_poly._edges.size())
				{
					p_wos << L", ";
				}
			}
			p_wos << L"}";
			return p_wos;
		}
	};
}

int main()
{
	spk::Vector3 points[4] = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}};
	spk::Vector3 deltaB = {1, 0, 0};
	spk::Vector3 deltaC = {0.5, 1, 0};
	spk::Vector3 deltaD = {0.5, 0.5, 0};
	tmp::Polygon polyA = tmp::Polygon::makeSquare(points[0], points[1], points[2], points[3]);
	tmp::Polygon polyB = tmp::Polygon::makeSquare(points[0] + deltaB, points[1] + deltaB, points[2] + deltaB, points[3] + deltaB);
	tmp::Polygon polyC = tmp::Polygon::makeSquare(points[0] + deltaC, points[1] + deltaC, points[2] + deltaC, points[3] + deltaC);
	tmp::Polygon polyD = tmp::Polygon::makeSquare(points[0] + deltaD, points[1] + deltaD, points[2] + deltaD, points[3] + deltaD);

	std::cout << polyA << std::endl;
	std::cout << polyB << std::endl;
	std::cout << polyC << std::endl;
	std::cout << polyD << std::endl;

	std::cout << " ----- A -> B -----" << std::endl;
	std::cout << "Is planar A : " << std::boolalpha << polyA.isPlanar() << std::endl;
	std::cout << "Is planar B : " << std::boolalpha << polyB.isPlanar() << std::endl;
	std::cout << "Is Coplanar : " << std::boolalpha << polyA.isCoplanar(polyB) << std::endl;
	std::cout << "Is adjacent : " << std::boolalpha << polyA.isAdjacent(polyB) << std::endl;
	std::cout << "Is overlapping : " << std::boolalpha << polyA.isOverlapping(polyB) << std::endl;
	std::cout << " ----- ------ -----" << std::endl << std::endl;

	std::cout << " ----- A -> C -----" << std::endl;
	std::cout << "Is planar A : " << std::boolalpha << polyA.isPlanar() << std::endl;
	std::cout << "Is planar C : " << std::boolalpha << polyC.isPlanar() << std::endl;
	std::cout << "Is Coplanar : " << std::boolalpha << polyA.isCoplanar(polyC) << std::endl;
	std::cout << "Is adjacent : " << std::boolalpha << polyA.isAdjacent(polyC) << std::endl;
	std::cout << "Is overlapping : " << std::boolalpha << polyA.isOverlapping(polyC) << std::endl;
	std::cout << " ----- ------ -----" << std::endl << std::endl;

	std::cout << " ----- A -> D -----" << std::endl;
	std::cout << "Is planar A : " << std::boolalpha << polyA.isPlanar() << std::endl;
	std::cout << "Is planar D : " << std::boolalpha << polyD.isPlanar() << std::endl;
	std::cout << "Is Coplanar : " << std::boolalpha << polyA.isCoplanar(polyD) << std::endl;
	std::cout << "Is adjacent : " << std::boolalpha << polyA.isAdjacent(polyD) << std::endl;
	std::cout << "Is overlapping : " << std::boolalpha << polyA.isOverlapping(polyD) << std::endl;
	std::cout << " ----- ------ -----" << std::endl << std::endl;

	return (0);
}
