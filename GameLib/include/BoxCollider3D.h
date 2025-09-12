#pragma once
#include "component.h"
#include "Utils.h"

class BoxCollider3D : public Component {
public:
    BoxCollider3D() : halfExtents(0.5f, 0.5f, 0.5f), isTrigger(false) {}

    void Init() override;
    void Update(float dt) override {}

    void AutoFitFromModel();

    void SetHalfExtents(const Vector3& he) { halfExtents = he; }
    Vector3 GetHalfExtents() const { return halfExtents; }

    void SetTrigger(bool trigger) { isTrigger = trigger; }
    bool IsTrigger() const { return isTrigger; }

    Vector3 GetCenter() const { return object ? object->GetPosition3D() : Vector3(); }

    bool Overlaps(const BoxCollider3D* other) const;

    Component* Clone() const override { return new BoxCollider3D(*this); }

private:
    Vector3 halfExtents;
    bool isTrigger;
};
