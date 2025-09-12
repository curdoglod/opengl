#pragma once
#include "component.h"
#include "Utils.h"

class BoxCollider3D;

class Rigidbody3D : public Component {
public:
    Rigidbody3D() : velocity(0,0,0), acceleration(0,0,0), mass(1.0f), useGravity(true), gravity(-9.81f) {}

    void Init() override {}
    void Update(float dt) override;

    void SetVelocity(const Vector3& v) { velocity = v; }
    void SetAcceleration(const Vector3& a) { acceleration = a; }
    void SetMass(float m) { mass = m; }
    void SetUseGravity(bool ug) { useGravity = ug; }
    void SetGravity(float g) { gravity = g; }
    Vector3 GetVelocity() const { return velocity; }
    Vector3 GetAcceleration() const { return acceleration; }
    float GetMass() const { return mass; }
    bool IsUsingGravity() const { return useGravity; }

    Component* Clone() const override { return new Rigidbody3D(*this); }

private:
    void integrate(float dt);
    void resolveCollisions();

private:
    Vector3 velocity;
    Vector3 acceleration;
    float mass;
    bool useGravity;
    float gravity; // gravity along Y axis (negative is downward)
};
