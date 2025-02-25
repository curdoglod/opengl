#pragma once
#include "component.h"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <unordered_set>

// Тип коллайдера (для определения типа объекта)
enum class ColliderType {
    Default,
    Ground,
    Water,
    Player,
    Enemy,
    Item
    // Можно добавить другие типы по необходимости
};

// Компонент BoxCollider3D представляет собой простой кубический коллайдер для 3D объектов
class BoxCollider3D : public Component
{
public:
    // Конструктор с параметрами размера коллайдера и типа
    BoxCollider3D(const Vector3& size = Vector3(1.0f, 1.0f, 1.0f), ColliderType type = ColliderType::Default);
    virtual ~BoxCollider3D();

    // Переопределяем методы Component
    virtual void Init() override;
    virtual void Update(float dt) override;
    
    // Метод для проверки столкновения с другим коллайдером
    bool IsColliding(const BoxCollider3D* other) const;
    
    // Методы для получения и установки параметров коллайдера
    Vector3 GetSize() const { return size; }
    void SetSize(const Vector3& newSize) { size = newSize; UpdateBounds(); }
    
    Vector3 GetOffset() const { return offset; }
    void SetOffset(const Vector3& newOffset) { offset = newOffset; UpdateBounds(); }
    
    // Методы для получения и установки типа коллайдера
    ColliderType GetColliderType() const { return colliderType; }
    void SetColliderType(ColliderType type) { colliderType = type; }
    
    // Метод для отображения коллайдера (для отладки)
    void SetDebugDraw(bool enabled) { debugDraw = enabled; }
    bool IsDebugDrawEnabled() const { return debugDraw; }
    
    // Получение границ коллайдера в мировых координатах
    Vector3 GetMin() const { return min; }
    Vector3 GetMax() const { return max; }
    
    // Виртуальные методы для обработки столкновений
    // Вызывается при первом столкновении с другим объектом
    virtual void OnCollisionEnter3D(Object* otherObject, BoxCollider3D* otherCollider) {}
    
    // Вызывается при выходе из столкновения с другим объектом
    virtual void OnCollisionExit3D(Object* otherObject, BoxCollider3D* otherCollider) {}
    
    // Вызывается каждый кадр, пока объекты находятся в столкновении
    virtual void OnCollisionStay3D(Object* otherObject, BoxCollider3D* otherCollider) {}
    
    // Метод для автоматического определения размера коллайдера на основе Model3DComponent
    void AutoDetectSizeFromModel();
    
    // Клонирование компонента
    virtual Component* Clone() const override { return new BoxCollider3D(size, colliderType); }

private:
    // Обновление границ коллайдера на основе позиции и размера
    void UpdateBounds();
    
    // Инициализация данных для отрисовки (для режима отладки)
    void InitRenderData();
    
    // Отрисовка коллайдера в режиме отладки
    void DebugRender();

    // Метод для обработки текущих столкновений
    void ProcessCollisions();

private:
    Vector3 size;      // Размер коллайдера
    Vector3 offset;    // Смещение относительно центра объекта
    Vector3 min;       // Минимальная точка AABB в мировых координатах
    Vector3 max;       // Максимальная точка AABB в мировых координатах
    bool debugDraw;    // Флаг отрисовки коллайдера для отладки
    ColliderType colliderType; // Тип коллайдера
    
    // Множество коллайдеров, с которыми в данный момент есть столкновение
    std::unordered_set<BoxCollider3D*> currentCollisions;
    
    // Данные для отрисовки в режиме отладки
    GLuint VAO, VBO, EBO;
    static GLuint shaderProgram;
    static GLuint loadShaderProgram();

    // Дружественный класс для доступа к приватным членам
    friend class CollisionManager;
}; 