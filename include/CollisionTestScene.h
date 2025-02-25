#pragma once
#include "SceneManager.h"
#include "BoxCollider3D.h"
#include "CollisionManager.h"
#include "Model3DComponent.h"

// Пользовательский компонент для мяча с обработкой столкновений
class BallComponent : public Component {
public:
    BallComponent() {}
    
    void Init() override {
        // Добавляем коллайдер к объекту
        ballCollider = new BoxCollider3D(Vector3(10, 10, 10), ColliderType::Player);
        object->AddComponent(ballCollider);
        ballCollider->SetDebugDraw(true);
        
        // Регистрируем коллайдер в менеджере коллизий
        CollisionManager::GetInstance().RegisterCollider(ballCollider);
    }
    
    void Update(float deltaTime) override {
        // Применяем гравитацию
        velocity.y -= gravity * deltaTime;
        
        // Обновляем позицию
        Vector3 position = object->GetPosition3D();
        position = position + velocity * deltaTime;
        
        // Проверяем, не упал ли мяч слишком низко
        if (position.y < -100) {
            position.y = 50; // Возвращаем мяч наверх
            velocity = Vector3(0, 0, 0);
        }
        
        object->SetPosition(position);
    }
    
    // Обработка столкновений через коллайдер
    void OnCollisionWithGround() {
        // Отскок от земли
        velocity.y = -velocity.y * bounceCoefficient;
        
        // Уменьшаем горизонтальную скорость (трение)
        velocity.x *= 0.9f;
        velocity.z *= 0.9f;
    }
    
    void OnCollisionWithWater() {
        // Замедление в воде
        velocity = velocity * 0.7f;
    }
    
    Vector3 velocity = Vector3(0, 0, 0);
    float gravity = 9.8f;
    float bounceCoefficient = 0.8f;
    
private:
    BoxCollider3D* ballCollider = nullptr;
};

// Пользовательский коллайдер для земли с обработкой столкновений
class GroundCollider : public BoxCollider3D {
public:
    GroundCollider(const Vector3& size) : BoxCollider3D(size, ColliderType::Ground) {}
    
    void OnCollisionEnter3D(Object* otherObject, BoxCollider3D* otherCollider) override {
        std::cout << "Столкновение с землей!" << std::endl;
        
        // Проверяем, является ли другой объект мячом
        BallComponent* ball = otherObject->GetComponent<BallComponent>();
        if (ball) {
            ball->OnCollisionWithGround();
        }
    }
};

// Пользовательский коллайдер для воды с обработкой столкновений
class WaterCollider : public BoxCollider3D {
public:
    WaterCollider(const Vector3& size) : BoxCollider3D(size, ColliderType::Water) {}
    
    void OnCollisionEnter3D(Object* otherObject, BoxCollider3D* otherCollider) override {
        std::cout << "Столкновение с водой!" << std::endl;
        
        // Проверяем, является ли другой объект мячом
        BallComponent* ball = otherObject->GetComponent<BallComponent>();
        if (ball) {
            ball->OnCollisionWithWater();
        }
    }
};

class CollisionTestScene : public SceneManager {
public:
    void Awake() override;
    void Init() override;
    void Update() override;
    
    // Обработка столкновений
    void OnCollision(const CollisionInfo& info);
    
    // Обработка клавиш для управления объектами
    void onKeyPressed(SDL_Keycode key) override;
    
private:
    Object* ball;
    Object* ground;
    Object* water;
    
    // Скорость движения объектов
    float moveSpeed = 50.0f;
}; 