#include "piano_planet.h"

#include "math_util.h"
#include "game_manager.h"
#include "input_manager.h"
#include "entity_manager.h"
#include "components/model_instance.h"

bool PianoPlanetComponent::ConnectComponents(EntityId id, Entity& e, GameManager& g) {
    _g = &g;
    _entityId = id;
    return true;
}

void PianoPlanetComponent::Update(float const dt) {
    double scrollX, scrollY;
    _g->_inputManager->GetMouseScroll(scrollX, scrollY);

    // if (scrollX == 0.0 && scrollY == 0.0) {
    //     return;
    // }

    float constexpr kScreenUnitToValueUnit = 1.f / 600.f;

    _currentValue += scrollY * kScreenUnitToValueUnit;
    _currentValue = math_util::Clamp(_currentValue, 0.f, 1.f);

    // HOWDY
    printf("HOWDY %f\n", _currentValue);

    if (Entity* entity = _g->_entityManager->GetEntity(_entityId)) {
        // change the model's color based on the current value.
        std::shared_ptr<ModelComponent> pModel = entity->FindComponentOfType<ModelComponent>().lock();
        if (pModel != nullptr) {
            pModel->_color = Vec4(_currentValue, _currentValue, _currentValue, 1.f);
        }
    }
}

void PianoPlanetComponent::Save(serial::Ptree pt) const {

}

void PianoPlanetComponent::Load(serial::Ptree pt) {

}