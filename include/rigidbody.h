#pragma once
#include "Utils.h"
#include "object.h"
#include "component.h"

class Object;
class Rigidbody : public Component {
public:
    Rigidbody() : velocity(5.0f, 10.0f), acceleration(0.0f, 9.9f), mass(4.0f), useGravity(true) {}

    void Update(float deltaTime) override {

        if (useGravity) {
            acceleration.y += gravity * mass;
        }


        velocity += acceleration * deltaTime;
        object->SetPosition(object->GetPosition() + velocity * deltaTime);


        acceleration = Vector2 (0.0f, 0.0f);


    }

    void setVelocity(const Vector2& v) { velocity = v; }
    void setAcceleration(const Vector2& a) { acceleration = a; }
    void setMass(float m) { mass = m; }
    void setUseGravity(bool ug) { useGravity = ug; }

    Vector2 getVelocity() const { return velocity; }
    Vector2 getAcceleration() const { return acceleration; }
    float getMass() const { return mass; }
    bool isUsingGravity() const { return useGravity; }

private:
    Vector2 velocity;
    Vector2 acceleration;
    float mass;
    bool useGravity;

    const float gravity = 9.81f;
};

