#include <sparkle.hpp>
#include <vector>

namespace
{
	spk::ObjMesh mergeObjMeshes(spk::ObjMesh &p_a, spk::ObjMesh &p_b)
	{
		auto colA = spk::CollisionMesh::fromObjMesh(&p_a);
		auto colB = spk::CollisionMesh::fromObjMesh(&p_b);

		std::vector<spk::Polygon> polys = colA.units();
		for (const auto &polyB : colB.units())
		{
			bool fused = false;
			for (auto &poly : polys)
			{
				if (poly.isCoplanar(polyB) == true)
				{
					spk::Polygon fusedPoly = poly.fuze(polyB);
					if (fusedPoly.edges().empty() == false)
					{
						poly = fusedPoly;
						fused = true;
						break;
					}
				}
			}
			if (fused == false)
			{
				polys.push_back(polyB);
			}
		}

		spk::ObjMesh result;
		for (const auto &poly : polys)
		{
			const auto &edges = poly.edges();
			if (edges.size() == 4)
			{
				spk::ObjMesh::Quad q;
				q.a.position = edges[0].first();
				q.b.position = edges[0].second();
				q.c.position = edges[1].second();
				q.d.position = edges[2].second();
				result.addShape(q);
			}
			else if (edges.size() == 3)
			{
				spk::ObjMesh::Triangle t;
				t.a.position = edges[0].first();
				t.b.position = edges[0].second();
				t.c.position = edges[1].second();
				result.addShape(t);
			}
		}
		return result;
	}

	class RendererSwapper : public spk::Component
	{
	private:
		std::vector<std::pair<spk::SafePointer<spk::ObjMeshRenderer>, spk::SafePointer<spk::CollisionMeshRenderer>>> _pairs;
		bool _useCollision = false;

	public:
		RendererSwapper(const std::wstring &p_name) :
			spk::Component(p_name)
		{
		}

		void addPair(spk::SafePointer<spk::ObjMeshRenderer> p_obj, spk::SafePointer<spk::CollisionMeshRenderer> p_col)
		{
			_pairs.emplace_back(p_obj, p_col);
		}

		void onStart() override
		{
			for (auto &pair : _pairs)
			{
				pair.first->activate();
				pair.second->deactivate();
			}
		}

		void onKeyboardEvent(spk::KeyboardEvent &p_event) override
		{
			if (p_event.type == spk::KeyboardEvent::Type::Press && p_event.key == spk::Keyboard::F1)
			{
				_useCollision = (_useCollision == false);
				for (auto &pair : _pairs)
				{
					if (_useCollision == true)
					{
						pair.first->deactivate();
						pair.second->activate();
					}
					else
					{
						pair.second->deactivate();
						pair.first->activate();
					}
				}
			}
		}
	};
} // namespace

int main()
{
	spk::GraphicalApplication app;
	auto window = app.createWindow(L"ObjMesh Merge Playground", {{0, 0}, {800, 600}});
	window->setUpdateTimer(0);
	window->requestMousePlacement({400, 300});

	spk::GameEngine engine;
	spk::GameEngineWidget engineWidget(L"EngineWidget", window->widget());
	engineWidget.setGeometry({0, 0}, window->geometry().size);
	engineWidget.setGameEngine(&engine);
	engineWidget.activate();

	spk::ObjMesh base = spk::ObjMesh::loadFromFile("playground/resources/obj/full_block.obj");

	spk::ObjMesh cubeB = base;
	cubeB.applyOffset({1.0f, 0.0f, 0.0f});
	spk::ObjMesh adjacentMesh = mergeObjMeshes(base, cubeB);
	spk::CollisionMesh adjacentCollision = spk::CollisionMesh::fromObjMesh(&adjacentMesh);

	spk::ObjMesh cubeC = base;
	cubeC.applyOffset({0.5f, 0.0f, 0.0f});
	spk::ObjMesh overlapMesh = mergeObjMeshes(base, cubeC);
	spk::CollisionMesh overlapCollision = spk::CollisionMesh::fromObjMesh(&overlapMesh);

	spk::Entity adjacentEntity(L"Adjacent", nullptr);
	auto adjObj = adjacentEntity.addComponent<spk::ObjMeshRenderer>(L"Adjacent/Obj");
	adjObj->setMesh(&adjacentMesh);
	adjObj->setTexture(adjacentMesh.material());
	auto adjCol = adjacentEntity.addComponent<spk::CollisionMeshRenderer>(L"Adjacent/Col");
	adjCol->setMesh(&adjacentCollision);
	adjCol->deactivate();
	adjacentEntity.activate();
	engine.addEntity(&adjacentEntity);

	spk::Entity overlapEntity(L"Overlap", nullptr);
	auto overObj = overlapEntity.addComponent<spk::ObjMeshRenderer>(L"Overlap/Obj");
	overObj->setMesh(&overlapMesh);
	overObj->setTexture(overlapMesh.material());
	auto overCol = overlapEntity.addComponent<spk::CollisionMeshRenderer>(L"Overlap/Col");
	overCol->setMesh(&overlapCollision);
	overCol->deactivate();
	overlapEntity.transform().place({3.0f, 0.0f, 0.0f});
	overlapEntity.activate();
	engine.addEntity(&overlapEntity);

	spk::Entity controller(L"Controller", nullptr);
	auto swapper = controller.addComponent<RendererSwapper>(L"Controller/Swapper");
	swapper->addPair(adjObj, adjCol);
	swapper->addPair(overObj, overCol);
	controller.activate();
	engine.addEntity(&controller);

	return app.run();
}
