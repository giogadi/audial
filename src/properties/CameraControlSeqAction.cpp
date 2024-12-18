#include "src/properties/CameraControlSeqAction.h"

#include "imgui/imgui.h"

#include "imgui_util.h"



void CameraControlSeqActionProps::Load(serial::Ptree pt) {
   
   pt.TryGetBool("setTarget", &_setTarget);
   
   pt.TryGetBool("jumpToTarget", &_jumpToTarget);
   
   serial::LoadFromChildOf(pt, "targetEditorId", _targetEditorId);
   
   pt.TryGetFloat("targetTrackingFactor", &_targetTrackingFactor);
   
   pt.TryGetBool("setOffset", &_setOffset);
   
   serial::LoadFromChildOf(pt, "desiredTargetToCameraOffset", _desiredTargetToCameraOffset);
   
   pt.TryGetBool("hasMinX", &_hasMinX);
   
   pt.TryGetFloat("minX", &_minX);
   
   pt.TryGetBool("hasMinZ", &_hasMinZ);
   
   pt.TryGetFloat("minZ", &_minZ);
   
   pt.TryGetBool("hasMaxX", &_hasMaxX);
   
   pt.TryGetFloat("maxX", &_maxX);
   
   pt.TryGetBool("hasMaxZ", &_hasMaxZ);
   
   pt.TryGetFloat("maxZ", &_maxZ);
   
}

void CameraControlSeqActionProps::Save(serial::Ptree pt) const {
    
    pt.PutBool("setTarget", _setTarget);
    
    pt.PutBool("jumpToTarget", _jumpToTarget);
    
    serial::SaveInNewChildOf(pt, "targetEditorId", _targetEditorId);
    
    pt.PutFloat("targetTrackingFactor", _targetTrackingFactor);
    
    pt.PutBool("setOffset", _setOffset);
    
    serial::SaveInNewChildOf(pt, "desiredTargetToCameraOffset", _desiredTargetToCameraOffset);
    
    pt.PutBool("hasMinX", _hasMinX);
    
    pt.PutFloat("minX", _minX);
    
    pt.PutBool("hasMinZ", _hasMinZ);
    
    pt.PutFloat("minZ", _minZ);
    
    pt.PutBool("hasMaxX", _hasMaxX);
    
    pt.PutFloat("maxX", _maxX);
    
    pt.PutBool("hasMaxZ", _hasMaxZ);
    
    pt.PutFloat("maxZ", _maxZ);
    
}

bool CameraControlSeqActionProps::ImGui() {
    bool changed = false;
    
    {
        bool thisChanged = ImGui::Checkbox("setTarget", &_setTarget);
        changed = changed || thisChanged;
    }
    
    {
        bool thisChanged = ImGui::Checkbox("jumpToTarget", &_jumpToTarget);
        changed = changed || thisChanged;
    }
    
    {
        bool thisChanged = imgui_util::InputEditorId("targetEditorId", &_targetEditorId);
        changed = changed || thisChanged;
    }
    
    {
        bool thisChanged = ImGui::InputFloat("targetTrackingFactor", &_targetTrackingFactor);
        changed = changed || thisChanged;
    }
    
    {
        bool thisChanged = ImGui::Checkbox("setOffset", &_setOffset);
        changed = changed || thisChanged;
    }
    
    {
        bool thisChanged = imgui_util::InputVec3("desiredTargetToCameraOffset", &_desiredTargetToCameraOffset);
        changed = changed || thisChanged;
    }
    
    {
        bool thisChanged = ImGui::Checkbox("hasMinX", &_hasMinX);
        changed = changed || thisChanged;
    }
    
    {
        bool thisChanged = ImGui::InputFloat("minX", &_minX);
        changed = changed || thisChanged;
    }
    
    {
        bool thisChanged = ImGui::Checkbox("hasMinZ", &_hasMinZ);
        changed = changed || thisChanged;
    }
    
    {
        bool thisChanged = ImGui::InputFloat("minZ", &_minZ);
        changed = changed || thisChanged;
    }
    
    {
        bool thisChanged = ImGui::Checkbox("hasMaxX", &_hasMaxX);
        changed = changed || thisChanged;
    }
    
    {
        bool thisChanged = ImGui::InputFloat("maxX", &_maxX);
        changed = changed || thisChanged;
    }
    
    {
        bool thisChanged = ImGui::Checkbox("hasMaxZ", &_hasMaxZ);
        changed = changed || thisChanged;
    }
    
    {
        bool thisChanged = ImGui::InputFloat("maxZ", &_maxZ);
        changed = changed || thisChanged;
    }
    
    return changed;
}