#include "vfx_pulse.h"

#include "entities/vfx.h"

void VfxPulseSeqAction::InitDerived(GameManager& g) {
    _s = State();
    if (ne::Entity* e = g._neEntityManager->FindEntityByNameAndType(_p._vfxEntityName, ne::EntityType::Vfx)) {
        _s._entityId = e->_id;
    }
}

void VfxPulseSeqAction::ExecuteDerived(GameManager& g) {
    if (ne::Entity* e = g._neEntityManager->GetEntity(_s._entityId)) {
        VfxEntity* vfx = (VfxEntity*) e;
        vfx->OnHit(g);
    }
}
