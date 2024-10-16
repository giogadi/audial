#include "add_motion.h"

#include "imgui_util.h"
#include "motion_manager.h"

extern GameManager gGameManager;

void AddMotionSeqAction::LoadDerived(serial::Ptree pt) {
	serial::LoadFromChildOf(pt, "editor_id", _p.editorId);	
	serial::LoadFromChildOf(pt, "v0", _p.v0);
	serial::LoadFromChildOf(pt, "a", _p.a);
	pt.TryGetFloat("time", &_p.time);
	if (pt.GetVersion() < 12) {
		Vec3 offset;
		serial::LoadFromChildOf(pt, "offset", offset);
		_p.v0 = Vec3();
		_p.a = Vec3();
		if (_p.time > 0.f) {
			_p.v0 = offset / _p.time;
		}		
	}
}
void AddMotionSeqAction::SaveDerived(serial::Ptree pt) const {
	serial::SaveInNewChildOf(pt, "editor_id", _p.editorId);
	serial::SaveInNewChildOf(pt, "v0", _p.v0);
	serial::SaveInNewChildOf(pt, "a", _p.a);
	pt.PutFloat("time", _p.time);
}
bool AddMotionSeqAction::ImGui() {
	bool needsInit = false;
	needsInit = imgui_util::InputEditorId("Editor id", &_p.editorId) || needsInit;
	bool needPreview = false;
	if (imgui_util::InputVec3("v0", &_p.v0)) {
		needPreview = true;
	}
	if (imgui_util::InputVec3("a", &_p.a)) {
		needPreview = true;
	}
	if (ImGui::InputFloat("Time", &_p.time, 0.f, 0.f, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) {
		needPreview = true;
	}

	if (needPreview) {
		ExecuteDerived(gGameManager);
	}

	return needsInit;
}

void AddMotionSeqAction::InitDerived(GameManager& g) {
	ne::Entity* e = g._neEntityManager->FindEntityByEditorId(_p.editorId);
	if (e) {
		_s.entityId = e->_id;
	}
}

void AddMotionSeqAction::ExecuteDerived(GameManager& g) {
	Motion* motion = g._motionManager->AddMotion(_s.entityId);
	motion->v = _p.v0;
	motion->a = _p.a;
	motion->entityId = _s.entityId;
	motion->timeLeft = _p.time;
}