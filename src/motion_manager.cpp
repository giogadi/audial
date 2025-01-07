#include "motion_manager.h"

#include <cstdio>

#include "new_entity.h"
#include "math_util.h"
#include "imgui_util.h"

namespace {
	static int constexpr kMaxMotions = 1024;

	// returns true if swapped something else in
	bool SwapRemove(Motion* motions, int* count, int removeIx) {
		if (removeIx + 1 < *count) {
			motions[removeIx] = motions[*count - 1];
			*count -= 1;
			return true;
		} else {
			*count -= 1;
			return false;
		}
	}
}

void MotionSpec::Save(serial::Ptree pt) const {
	pt.PutBool("do_pos", updatePos);
	serial::SaveInNewChildOf(pt, "v0", v0);
	serial::SaveInNewChildOf(pt, "a", a);
	pt.PutFloat("max_speed", maxSpeed);
	pt.PutFloat("time", totalTime);
	pt.PutBool("do_color", doColor);	
	serial::SaveInNewChildOf(pt, "end_color", endColor);
	pt.PutBool("do_scale", doScale);
	serial::SaveInNewChildOf(pt, "scale", scaleToApply);
}
void MotionSpec::Load(serial::Ptree pt) {
	pt.TryGetBool("do_pos", &updatePos);
	serial::LoadFromChildOf(pt, "v0", v0);
	serial::LoadFromChildOf(pt, "a", a);
	pt.TryGetFloat("max_speed", &maxSpeed);
	pt.TryGetFloat("time", &totalTime);
	pt.TryGetBool("do_color", &doColor);
	serial::LoadFromChildOf(pt, "end_color", endColor);
	pt.TryGetBool("do_scale", &doScale);
	serial::LoadFromChildOf(pt, "scale", scaleToApply);

	if (pt.GetVersion() < 12) {
		Vec3 offset;
		serial::LoadFromChildOf(pt, "offset", offset);
		v0 = Vec3();
		a = Vec3();
		if (totalTime > 0.f) {
			v0 = offset / totalTime;
		}
	}
}
bool MotionSpec::ImGui() {
	bool changed = false;	
	if (ImGui::InputFloat("Time", &totalTime, 0.f, 0.f, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) {
		changed = true;
	}
	changed = ImGui::Checkbox("Pos", &updatePos) || changed;
	if (updatePos) {
		if (imgui_util::InputVec3("v0", &v0, true)) {
			changed = true;
		}
		if (imgui_util::InputVec3("a", &a, true)) {
			changed = true;
		}
		if (ImGui::InputFloat("Max speed", &maxSpeed, 0.f, 0.f, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) {
			changed = true;
		}
	}
	changed = ImGui::Checkbox("Scale", &doScale) || changed;
	if (doScale) {
		if (imgui_util::InputVec3("Scale", &scaleToApply, true)) {
			changed = true;
		}
	}	
	changed = ImGui::Checkbox("Color", &doColor) || changed;
	if (doColor) {
		changed = imgui_util::ColorEdit4("End##color", &endColor) || changed;
	}

	return changed;
}

void MotionManager::Init() {
	_motions = new Motion[kMaxMotions]();
}

void MotionManager::Destroy() {
	delete[] _motions;
	_motions = nullptr;
}

Motion* MotionManager::AddMotion(MotionSpec const &spec, ne::BaseEntity *entity) {
	if (_motionCount + 1 >= kMaxMotions) {
		printf("MotionManager: EXCEEDED MAX MOTIONS\n");
		return nullptr;
	}
	Motion* motion = &_motions[_motionCount++];
	*motion = Motion();
	motion->spec = spec;
	motion->entityId = entity->_id;
	motion->timeLeft = spec.totalTime;
	motion->v = spec.v0;
	motion->startColor = entity->_modelColor;
	motion->startScale = entity->_transform.Scale();
	motion->endScale = Vec3::ElemWiseMult(motion->startScale, spec.scaleToApply);
	return motion;
}

void MotionManager::Update(float dt, GameManager& g) {
	for (int ii = 0; ii < _motionCount; ++ii) {
		Motion* motion = &_motions[ii];
		bool needRemove = false;
		motion->timeLeft -= dt;
		if (motion->timeLeft <= 0.f) {
			needRemove = true;
			/*if (g._editMode) {
				// Keep running the motion for 1 second past, then reset.
				if (motion->timeLeft < -1.f) {
					ne::BaseEntity* e = g._neEntityManager->GetEntity(motion->entityId);
					if (e) {
						if (e)
						e->_transform = e->_initTransform;
						e->_modelColor = e->_initModelColor;
					}
					needRemove = true;
				} else {
					continue;
				}
			} else {
				needRemove = true;
			}*/
		}
		ne::BaseEntity* entity = nullptr;
		if (!needRemove) {
			entity = g._neEntityManager->GetEntity(motion->entityId);
			if (entity == nullptr) {
				needRemove = true;
			}
		}
		if (needRemove) {
			if (SwapRemove(_motions, &_motionCount, ii)) {
				--ii;
			}
			continue;
		}

		if (motion->spec.updatePos) {
			Vec3 dv = motion->spec.a * dt;
			motion->v += dv;
			if (motion->spec.maxSpeed >= 0.f) {
				Vec3 dir = motion->v;
				float speed = dir.Normalize();
				if (speed > motion->spec.maxSpeed) {
					motion->v = dir * motion->spec.maxSpeed;
				}
			}
			Vec3 dp = motion->v * dt;
			entity->_transform.Translate(dp);
		}        

		// NOTE: inverseLerp handles clamping of x
		float t = math_util::InverseLerp(0.f, motion->spec.totalTime, motion->spec.totalTime - motion->timeLeft);

		if (motion->spec.doColor) {			
			entity->_modelColor = math_util::Vec4Lerp(motion->startColor, motion->spec.endColor, t);
		}

		if (motion->spec.doScale) {
			Vec3 newScale = math_util::Vec3Lerp(t, motion->startScale, motion->endScale);
			entity->_transform.SetScale(newScale);
		}		
	}
}
