#pragma once
#include "component.h"
#include "object.h"
#include "Utils.h"

class Rigidbody2D : public Component {
public:
    Rigidbody2D()
        : velocity(0.0f, 0.0f)
        , acceleration(0.0f, 0.0f)
        , mass(1.0f)
        , useGravity(false)
        , gravity(9.81f) {}

    void Init() override {}

    void Update(float deltaTime) override {
        if (!object) return;

        Vector2 frameAccel = acceleration;
        if (useGravity) {
            frameAccel.y += gravity;
        }

        velocity += frameAccel * deltaTime;
        object->SetPosition(object->GetPosition() + velocity * deltaTime);

        acceleration = Vector2(0.0f, 0.0f);
    }

    void SetVelocity(const Vector2& v) { velocity = v; }
    void SetAcceleration(const Vector2& a) { acceleration = a; }
    void SetMass(float m) { mass = m; }
    void SetUseGravity(bool ug) { useGravity = ug; }
    void SetGravity(float g) { gravity = g; }

    Vector2 GetVelocity() const { return velocity; }
    Vector2 GetAcceleration() const { return acceleration; }
    float GetMass() const { return mass; }
    bool IsUsingGravity() const { return useGravity; }

    Component* Clone() const override { return new Rigidbody2D(*this); }

private:
    Vector2 velocity;
    Vector2 acceleration;
    float mass;
    bool useGravity;
    float gravity;
};
