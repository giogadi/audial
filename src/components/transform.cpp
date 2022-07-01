#include "transform.h"

void TransformComponent::Save(ptree& pt) const {
    _transform.Save(pt.add_child("mat4", ptree()));
}
void TransformComponent::Load(ptree const& pt) {
    _transform.Load(pt.get_child("mat4"));
}