#include "CollisionTestScene.h"
#include "SceneDeclarations.h"
#include <iostream>

void CollisionTestScene::Awake()
{
    // Устанавливаем функцию обратного вызова для обработки столкновений
    CollisionManager::GetInstance().SetCollisionCallback(
        [this](const CollisionInfo& info) { this->OnCollision(info); }
    );
}

void CollisionTestScene::Init()
{
    // Устанавливаем размер окна
    Vector2 windowSize(800, 600);
    SetWindowSize(windowSize.x, windowSize.y);
    
    // Создаем мяч с компонентом BallComponent
    ball = CreateObject();
    ball->AddComponent(new Model3DComponent("Assets/model.fbx"));
    ball->SetPosition(Vector3(0, 50, 0)); // Начальная позиция мяча (высоко над землей)
    ball->SetSize(Vector2(1, 1)); // Уменьшаем размер модели
    ball->SetLayer(10);
    
    // Добавляем компонент мяча
    ball->AddComponent(new BallComponent());
    
    // Создаем землю
    ground = CreateObject();
    ground->AddComponent(new Model3DComponent("Assets/model.fbx"));
    ground->SetPosition(Vector3(0, -20, 0)); // Позиция земли
    ground->SetSize(Vector2(1, 1)); // Делаем землю плоской и широкой
    ground->SetLayer(5);
    
    // Добавляем коллайдер земли
    GroundCollider* groundCollider = new GroundCollider(Vector3(1, 1, 1));
    ground->AddComponent(groundCollider);
    groundCollider->SetDebugDraw(true);
    
    // Регистрируем коллайдер в менеджере коллизий
    CollisionManager::GetInstance().RegisterCollider(groundCollider);
    
    // Создаем воду
    water = CreateObject();
    water->AddComponent(new Model3DComponent("Assets/model.fbx"));
    water->SetPosition(Vector3(50, 30, 0)); // Позиция воды
    water->SetSize(Vector2(1, 1)); // Делаем воду плоской
    water->SetLayer(6);
    
    // Добавляем коллайдер воды
    WaterCollider* waterCollider = new WaterCollider(Vector3(1, 1, 1));
    water->AddComponent(waterCollider);
    waterCollider->SetDebugDraw(true);
    
    // Регистрируем коллайдер в менеджере коллизий
    CollisionManager::GetInstance().RegisterCollider(waterCollider);
    
    // Добавляем инструкции
    Object* instructions = CreateObject();
    instructions->SetPosition(Vector2(10, 10));
    instructions->SetLayer(100);
    instructions->AddComponent(new TextComponent(16, "WASD - управление мячом, SPACE - подбросить мяч", Color(255, 255, 255), TextAlignment::LEFT));
}

void CollisionTestScene::Update()
{
    // Обновляем менеджер коллизий
    CollisionManager::GetInstance().Update();
    
    // Вращаем объекты для визуального эффекта
    ball->SetRotation(ball->GetAngle() + Vector3(0.5f, 0.5f, 0));
}

void CollisionTestScene::OnCollision(const CollisionInfo& info)
{
    // Эта функция вызывается для всех столкновений
    // Теперь мы можем определить тип коллайдера
    
    // Получаем типы коллайдеров
    ColliderType typeA = info.colliderA->GetColliderType();
    ColliderType typeB = info.colliderB->GetColliderType();
    
    // Выводим информацию о столкновении
    std::cout << "Столкновение между объектами типов: ";
    
    // Выводим тип первого коллайдера
    switch (typeA) {
        case ColliderType::Ground: std::cout << "Ground"; break;
        case ColliderType::Water: std::cout << "Water"; break;
        case ColliderType::Player: std::cout << "Player"; break;
        default: std::cout << "Unknown"; break;
    }
    
    std::cout << " и ";
    
    // Выводим тип второго коллайдера
    switch (typeB) {
        case ColliderType::Ground: std::cout << "Ground"; break;
        case ColliderType::Water: std::cout << "Water"; break;
        case ColliderType::Player: std::cout << "Player"; break;
        default: std::cout << "Unknown"; break;
    }
    
    std::cout << std::endl;
}

void CollisionTestScene::onKeyPressed(SDL_Keycode key)
{
    // Получаем компонент мяча
    BallComponent* ballComponent = ball->GetComponent<BallComponent>();
    if (!ballComponent) return;
    
    // Получаем текущую скорость мяча
    Vector3& velocity = ballComponent->velocity;
    
    // Обработка клавиш для управления мячом
    switch (key)
    {
        case SDLK_w: // Вперед
            velocity.z -= moveSpeed * 0.1f;
            break;
        case SDLK_s: // Назад
            velocity.z += moveSpeed * 0.1f;
            break;
        case SDLK_a: // Влево
            velocity.x -= moveSpeed * 0.1f;
            break;
        case SDLK_d: // Вправо
            velocity.x += moveSpeed * 0.1f;
            break;
        case SDLK_SPACE: // Подбросить мяч
            velocity.y += moveSpeed * 0.3f;
            break;
        case SDLK_ESCAPE: // Возврат в главное меню
            SwitchToScene(new StartScene());
            break;
    }
} 