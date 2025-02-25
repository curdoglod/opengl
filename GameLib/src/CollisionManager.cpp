#include "CollisionManager.h"
#include <algorithm>

// Реализация метода получения экземпляра синглтона
CollisionManager& CollisionManager::GetInstance()
{
    static CollisionManager instance;
    return instance;
}

// Конструктор
CollisionManager::CollisionManager()
    : collisionCallback(nullptr)
{
}

// Регистрация коллайдера
void CollisionManager::RegisterCollider(BoxCollider3D* collider)
{
    // Проверяем, что коллайдер еще не зарегистрирован
    if (std::find(colliders.begin(), colliders.end(), collider) == colliders.end()) {
        colliders.push_back(collider);
        collisionMap[collider] = std::unordered_set<BoxCollider3D*>();
    }
}

// Удаление коллайдера из системы
void CollisionManager::UnregisterCollider(BoxCollider3D* collider)
{
    auto it = std::find(colliders.begin(), colliders.end(), collider);
    if (it != colliders.end()) {
        // Удаляем коллайдер из списка
        colliders.erase(it);
        
        // Удаляем все записи о столкновениях с этим коллайдером
        for (auto& pair : collisionMap) {
            pair.second.erase(collider);
        }
        
        // Удаляем запись о коллайдере из карты столкновений
        collisionMap.erase(collider);
    }
}

// Обновление и проверка всех коллизий
void CollisionManager::Update()
{
    // Создаем временную карту для отслеживания текущих столкновений
    std::unordered_map<BoxCollider3D*, std::unordered_set<BoxCollider3D*>> currentCollisions;
    
    // Инициализируем карту для всех коллайдеров
    for (auto* collider : colliders) {
        currentCollisions[collider] = std::unordered_set<BoxCollider3D*>();
    }
    
    // Проверяем все пары коллайдеров
    for (size_t i = 0; i < colliders.size(); ++i) {
        for (size_t j = i + 1; j < colliders.size(); ++j) {
            BoxCollider3D* colliderA = colliders[i];
            BoxCollider3D* colliderB = colliders[j];
            
            // Проверяем столкновение
            bool isColliding = colliderA->IsColliding(colliderB);
            
            if (isColliding) {
                // Добавляем в текущие столкновения
                currentCollisions[colliderA].insert(colliderB);
                currentCollisions[colliderB].insert(colliderA);
                
                // Создаем информацию о столкновении
                CollisionInfo info;
                info.colliderA = colliderA;
                info.colliderB = colliderB;
                info.objectA = colliderA->object;
                info.objectB = colliderB->object;
                
                // Проверяем, было ли это столкновение в предыдущем кадре
                bool wasCollidingBefore = collisionMap[colliderA].find(colliderB) != collisionMap[colliderA].end();
                
                if (!wasCollidingBefore) {
                    // Новое столкновение - вызываем OnCollisionEnter3D
                    colliderA->OnCollisionEnter3D(info.objectB, colliderB);
                    colliderB->OnCollisionEnter3D(info.objectA, colliderA);
                } else {
                    // Продолжающееся столкновение - вызываем OnCollisionStay3D
                    colliderA->OnCollisionStay3D(info.objectB, colliderB);
                    colliderB->OnCollisionStay3D(info.objectA, colliderA);
                }
                
                // Вызываем функцию обратного вызова, если она установлена
                if (collisionCallback) {
                    collisionCallback(info);
                }
            }
        }
    }
    
    // Проверяем, какие столкновения закончились
    for (auto& pair : collisionMap) {
        BoxCollider3D* colliderA = pair.first;
        
        // Проверяем, что коллайдер все еще существует
        if (currentCollisions.find(colliderA) != currentCollisions.end()) {
            // Находим столкновения, которые были в предыдущем кадре, но нет в текущем
            for (auto* colliderB : pair.second) {
                if (currentCollisions[colliderA].find(colliderB) == currentCollisions[colliderA].end()) {
                    // Столкновение закончилось - вызываем OnCollisionExit3D
                    colliderA->OnCollisionExit3D(colliderB->object, colliderB);
                    colliderB->OnCollisionExit3D(colliderA->object, colliderA);
                }
            }
        }
    }
    
    // Обновляем карту столкновений
    collisionMap = std::move(currentCollisions);
}

// Установка функции обратного вызова для обработки столкновений
void CollisionManager::SetCollisionCallback(CollisionCallback callback)
{
    collisionCallback = callback;
} 