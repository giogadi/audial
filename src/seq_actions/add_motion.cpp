#include "add_motion.h"

#include "imgui_util.h"
#include "motion_manager.h"

extern GameManager gGameManager;

void AddMotionSeqAction::LoadDerived(serial::Ptree pt) {
	serial::LoadFromChildOf(pt, "spec", _p.spec);
	serial::LoadFromChildOf(pt, "editor_id", _p.editorId);
	if (pt.GetVersion() < 14) {
		_p.spec.Load(pt);
	}
}
void AddMotionSeqAction::SaveDerived(serial::Ptree pt) const {
	serial::SaveInNewChildOf(pt, "spec", _p.spec);
	serial::SaveInNewChildOf(pt, "editor_id", _p.editorId);
}
bool AddMotionSeqAction::ImGui() {
	bool needsInit = false;
	needsInit = imgui_util::InputEditorId("Editor id", &_p.editorId) || needsInit;
	
	bool needPreview = _p.spec.ImGui();
	if (needPreview) {
		ExecuteDerived(gGameManager);
	}

	return needsInit;
}

void AddMotionSeqAction::InitDerived(GameManager& g) {
	ne::Entity* e = g._neEntityManager->FindEntityByEditorId(_p.editorId, /*isActive=*/nullptr, "AddMotionSeqAction::Init");
	if (e) {
		_s.entityId = e->_id;
	}
}

void AddMotionSeqAction::ExecuteDerived(GameManager& g) {
	ne::BaseEntity *e = g._neEntityManager->GetActiveOrInactiveEntity(_s.entityId);
	if (e) {
		g._motionManager->AddMotion(_p.spec, e);
	} else {
		printf("AddMotionSeqAction::Execute: no entity found\n");
	}
}