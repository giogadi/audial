#include "add_motion.h"

#include "imgui_util.h"
#include "motion_manager.h"

void AddMotionSeqAction::LoadDerived(serial::Ptree pt) {
	serial::LoadFromChildOf(pt, "editor_id", _p.editorId);
	serial::LoadFromChildOf(pt, "v", _p.v);
	pt.TryGetFloat("time", &_p.time);
}
void AddMotionSeqAction::SaveDerived(serial::Ptree pt) const {
	serial::SaveInNewChildOf(pt, "editor_id", _p.editorId);
	serial::SaveInNewChildOf(pt, "v", _p.v);
	pt.PutFloat("time", _p.time);
}
bool AddMotionSeqAction::ImGui() {
	imgui_util::InputEditorId("Editor id", &_p.editorId);
	imgui_util::InputVec3("Velocity", &_p.v);
	ImGui::InputFloat("Time", &_p.time);

	return false;
}

void AddMotionSeqAction::InitDerived(GameManager& g) {
	ne::Entity* e = g._neEntityManager->FindEntityByEditorId(_p.editorId);
	if (e) {
		_s.entityId = e->_id;
	}
}

void AddMotionSeqAction::ExecuteDerived(GameManager& g) {
	Motion* motion = g._motionManager->AddMotion(_s.entityId);
	motion->v = _p.v;
	motion->timeLeft = _p.time;
}