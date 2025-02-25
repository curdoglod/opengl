#pragma once
#include <vector>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include "BoxCollider3D.h"

// Структура для хранения информации о столкновении
struct CollisionInfo {
    Object* objectA;
    Object* objectB;
    BoxCollider3D* colliderA;
    BoxCollider3D* colliderB;
    
    // Получение типа коллайдера другого объекта
    ColliderType GetOtherColliderType(Object* thisObject) const {
        if (thisObject == objectA) {
            return colliderB->GetColliderType();
        } else {
            return colliderA->GetColliderType();
        }
    }
    
    // Получение другого объекта
    Object* GetOtherObject(Object* thisObject) const {
        if (thisObject == objectA) {
            return objectB;
        } else {
            return objectA;
        }
    }
    
    // Получение другого коллайдера
    BoxCollider3D* GetOtherCollider(BoxCollider3D* thisCollider) const {
        if (thisCollider == colliderA) {
            return colliderB;
        } else {
            return colliderA;
        }
    }
};

// Тип функции обратного вызова для обработки столкновений
using CollisionCallback = std::function<void(const CollisionInfo&)>;

// Класс для управления коллизиями
class CollisionManager {
public:
    // Получение экземпляра синглтона
    static CollisionManager& GetInstance();
    
    // Регистрация коллайдера
    void RegisterCollider(BoxCollider3D* collider);
    
    // Удаление коллайдера из системы
    void UnregisterCollider(BoxCollider3D* collider);
    
    // Обновление и проверка всех коллизий
    void Update();
    
    // Установка функции обратного вызова для обработки столкновений
    void SetCollisionCallback(CollisionCallback callback);
    
private:
    // Приватный конструктор (паттерн Singleton)
    CollisionManager();
    
    // Запрет копирования и присваивания
    CollisionManager(const CollisionManager&) = delete;
    CollisionManager& operator=(const CollisionManager&) = delete;
    
    // Список всех зарегистрированных коллайдеров
    std::vector<BoxCollider3D*> colliders;
    
    // Карта текущих столкновений (ключ - коллайдер, значение - множество коллайдеров, с которыми он сталкивается)
    std::unordered_map<BoxCollider3D*, std::unordered_set<BoxCollider3D*>> collisionMap;
    
    // Функция обратного вызова для обработки столкновений
    CollisionCallback collisionCallback;
}; 