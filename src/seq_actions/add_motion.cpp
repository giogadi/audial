#include "add_motion.h"

#include "imgui_util.h"
#include "motion_manager.h"

void AddMotionSeqAction::LoadDerived(serial::Ptree pt) {
	serial::LoadFromChildOf(pt, "editor_id", _p.editorId);
	serial::LoadFromChildOf(pt, "offset", _p.offset);
	pt.TryGetFloat("time", &_p.time);
}
void AddMotionSeqAction::SaveDerived(serial::Ptree pt) const {
	serial::SaveInNewChildOf(pt, "editor_id", _p.editorId);
	serial::SaveInNewChildOf(pt, "offset", _p.offset);
	pt.PutFloat("time", _p.time);
}
bool AddMotionSeqAction::ImGui() {
	imgui_util::InputEditorId("Editor id", &_p.editorId);
	imgui_util::InputVec3("Offset", &_p.offset);
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
	if (_p.time <= 0.f) {
		motion->v = Vec3();
	} else {
		motion->v = _p.offset / _p.time;
	}	
	motion->timeLeft = _p.time;
}