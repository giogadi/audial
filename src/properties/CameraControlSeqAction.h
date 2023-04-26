#pragma once


#include "matrix.h"


#include "serial.h"

struct CameraControlSeqActionProps {
    
    bool _setTarget = false;
    
    std::string _targetEntityName;
    
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