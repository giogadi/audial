#include "entities/enemy_spawner.h"

#include "imgui/imgui.h"

#include "game_manager.h"
#include "aabb.h"
#include "rng.h"
#include "imgui_vector_util.h"
#include "serial_vector_util.h"
#include "entities/typing_enemy.h"
#include "motion_manager.h"

extern GameManager gGameManager;

void EnemySpawnerEntity::SaveDerived(serial::Ptree pt) const {
	serial::SaveVectorInChildNode(pt, "enemy_templates", "enemy", _p.enemyTemplates);
}

void EnemySpawnerEntity::LoadDerived(serial::Ptree pt) {
	serial::LoadVectorFromChildNode(pt, "enemy_templates", _p.enemyTemplates);
}

ne::BaseEntity::ImGuiResult EnemySpawnerEntity::ImGuiDerived(GameManager& g) {
	if (ImGui::TreeNode("Enemy templates")) {
		imgui_util::InputVectorOptions options;
		options.treePerItem = false;
		options.removeOnSameLine = true;
		imgui_util::InputVector(_p.enemyTemplates, options);
		ImGui::TreePop();
	}	
	return ImGuiResult::Done;
}

void EnemySpawnerEntity::InitDerived(GameManager& g) {
	_s.timeLeft = _p.spawnCooldown;
	_s.currentIndex = 0;
	_s.enemyTemplates.clear();
	_s.enemyTemplates.reserve(_p.enemyTemplates.size());
	for (EditorId eId : _p.enemyTemplates) {
		ne::Entity* e = g._neEntityManager->FindEntityByEditorIdAndType(eId, ne::EntityType::TypingEnemy);
		_s.enemyTemplates.push_back(e->_id);
	}
}

void EnemySpawnerEntity::UpdateDerived(GameManager& g, float dt) {
	if (g._editMode) {
		return;
	}

	_s.timeLeft -= dt;
	if (_s.timeLeft <= 0.f) {
		// Spawn an enemy, let's GOOO
		TypingEnemyEntity* e = static_cast<TypingEnemyEntity*>(g._neEntityManager->AddEntity(ne::EntityType::TypingEnemy));
		
		if (_s.currentIndex >= 0 && _s.currentIndex < _s.enemyTemplates.size()) {
			TypingEnemyEntity const& enemyTemplate = *static_cast<TypingEnemyEntity*>(g._neEntityManager->GetActiveOrInactiveEntity(_s.enemyTemplates[_s.currentIndex]));

			serial::Ptree enemyPt = serial::Ptree::MakeNew();
			enemyTemplate.Save(enemyPt);
			e->Load(enemyPt);
			enemyPt.DeleteData();

			// Sample a point from within spawner's aabb
			// Aabb aabb;
			// aabb._min = _transform.Pos() - _transform.Scale() * 0.5f;
			// aabb._max = _transform.Pos() + _transform.Scale() * 0.5f;
			// aabb._min._y = 0.f;
			// aabb._max._y = 0.f;
			// Vec3 enemyPos = aabb.SampleRandom();

            // Sample from edges of spawner's aabb
            Vec3 enemyPos; 
            {
                int edge = rng::GetIntGlobal(0, 3);
                switch (edge) {
                    case 0: {
                        // TOP
                        enemyPos._z = -1.f;
                        enemyPos._x = rng::GetFloatGlobal(-1.f, 1.f);
                        break;
                    }
                    case 1: {
                        // BOTTOM
                        enemyPos._z = 1.f;
                        enemyPos._x = rng::GetFloatGlobal(-1.f, 1.f); 
                        break;
                    }
                    case 2: {
                        // LEFT
                        enemyPos._x = -1.f;
                        enemyPos._z = rng::GetFloatGlobal(-1.f, 1.f); 
                        break;
                    }
                    case 3: {
                        // RIGHT
                        enemyPos._x = 1.f;
                        enemyPos._z = rng::GetFloatGlobal(-1.f, 1.f); 
                        break;
                    }
                }
                enemyPos.ElemWiseMult(_transform.Scale() * 0.5f);
                enemyPos += _transform.Pos();
            }

			e->_initTransform.SetPos(enemyPos);

			// Sample text
			{
				int letterIx = rng::GetIntGlobal(0, 25);
				char letter = 'a' + letterIx;
				e->_p._keyText = std::string(1, letter);
			}

            // Generate motion
            {
                Motion* m = g._motionManager->AddMotion(e->_id);
                Vec3 dir = _transform.Pos() - enemyPos;
                dir.Normalize();
                m->v = dir * 0.f;
                m->a = dir * 4.f;
                m->maxSpeed = 20.f;
                m->timeLeft = 10;
            }

			e->Init(g);

			_s.timeLeft = _p.spawnCooldown;
			++_s.currentIndex;
			if (_s.currentIndex >= _p.enemyTemplates.size()) {
				_s.currentIndex = 0;
			}
		}		
	}
}
