#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <iostream>
#include "application.h"
#include "scene.h"
#include "object.h"
#include "Model3DComponent.h"
#include "BoxCollider3D.h"

// Простая тестовая сцена для проверки автоматического определения размера коллайдера
class AutoColliderTestScene : public Scene
{
public:
    AutoColliderTestScene() : Scene("AutoColliderTest") {}
    
    void Init() override
    {
        std::cout << "Инициализация тестовой сцены автоопределения размера коллайдера" << std::endl;
        
        // Создаем объект
        Object* modelObject = new Object();
        modelObject->SetName("TestModel");
        modelObject->SetPosition3D(Vector3(0, 0, 0));
        
        // Добавляем компонент 3D модели
        Model3DComponent* modelComponent = new Model3DComponent("assets/models/cube.obj");
        modelObject->AddComponent(modelComponent);
        
        // Добавляем коллайдер
        BoxCollider3D* collider = new BoxCollider3D();
        collider->SetDebugDraw(true); // Включаем отображение коллайдера
        modelObject->AddComponent(collider);
        
        // Добавляем объект в сцену
        AddObject(modelObject);
        
        // Создаем второй объект с ручной настройкой размера коллайдера для сравнения
        Object* compareObject = new Object();
        compareObject->SetName("CompareModel");
        compareObject->SetPosition3D(Vector3(200, 0, 0)); // Размещаем справа
        
        // Добавляем тот же компонент 3D модели
        Model3DComponent* compareModelComponent = new Model3DComponent("assets/models/cube.obj");
        compareObject->AddComponent(compareModelComponent);
        
        // Добавляем коллайдер с фиксированным размером
        BoxCollider3D* compareCollider = new BoxCollider3D(Vector3(50, 50, 50));
        compareCollider->SetDebugDraw(true);
        compareObject->AddComponent(compareCollider);
        
        // Добавляем объект в сцену
        AddObject(compareObject);
        
        std::cout << "Тестовая сцена инициализирована. Слева - модель с автоматическим размером коллайдера, справа - с фиксированным размером." << std::endl;
    }
    
    void Update(float dt) override
    {
        Scene::Update(dt);
        
        // Вывод информации один раз в секунду
        static float timer = 0;
        timer += dt;
        if (timer >= 1.0f) {
            timer = 0;
            
            // Выводим размеры коллайдеров
            Object* testModel = FindObject("TestModel");
            Object* compareModel = FindObject("CompareModel");
            
            if (testModel && compareModel) {
                BoxCollider3D* testCollider = testModel->GetComponent<BoxCollider3D>();
                BoxCollider3D* compareCollider = compareModel->GetComponent<BoxCollider3D>();
                
                if (testCollider && compareCollider) {
                    std::cout << "Размер автоколлайдера: " 
                              << testCollider->GetSize().x << ", "
                              << testCollider->GetSize().y << ", "
                              << testCollider->GetSize().z << std::endl;
                              
                    std::cout << "Размер фиксированного коллайдера: " 
                              << compareCollider->GetSize().x << ", "
                              << compareCollider->GetSize().y << ", "
                              << compareCollider->GetSize().z << std::endl;
                }
            }
        }
    }
};

int main(int argc, char* argv[])
{
    // Инициализируем приложение
    Application app;
    if (!app.Init("Тест автоматического размера коллайдера", 800, 600)) {
        return -1;
    }
    
    // Создаем и устанавливаем тестовую сцену
    AutoColliderTestScene* testScene = new AutoColliderTestScene();
    app.SetScene(testScene);
    
    // Запускаем главный цикл приложения
    app.Run();
    
    return 0;
} 