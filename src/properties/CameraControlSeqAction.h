#pragma once


#include "editor_id.h"

#include "matrix.h"


#include "serial.h"

struct CameraControlSeqActionProps {
    
    bool _setTarget = false;
    
    bool _jumpToTarget = false;
    
    EditorId _targetEditorId;
    
    float _targetTrackingFactor = 0.05f;
    
    bool _setOffset = false;
    
    Vec3 _desiredTargetToCameraOffset;
    
    bool _hasMinX = false;
    
    float _minX = 0.f;
    
    bool _hasMinZ = false;
    
    float _minZ = 0.f;
    
    bool _hasMaxX = false;
    
    float _maxX = 0.f;
    
    bool _hasMaxZ = false;
    
    float _maxZ = 0.f;
    

    void Load(serial::Ptree pt);
    void Save(serial::Ptree pt) const;
    bool ImGui();
};