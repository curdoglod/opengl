#include "BoxCollider3D.h"
#include "object.h"
#include "Model3DComponent.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

// Инициализация статической переменной шейдерной программы
GLuint BoxCollider3D::shaderProgram = 0;

// Вершинный шейдер для отрисовки коллайдера в режиме отладки
static const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

// Фрагментный шейдер для отрисовки коллайдера в режиме отладки
static const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

uniform vec4 color;

void main()
{
    FragColor = color;
}
)";

// Вспомогательная функция компиляции шейдера
static GLuint compileShader(GLenum type, const char* src)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Ошибка компиляции шейдера: " << infoLog << std::endl;
    }
    return shader;
}

// Загрузка шейдерной программы для отрисовки коллайдера
GLuint BoxCollider3D::loadShaderProgram()
{
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Ошибка линковки шейдерной программы: " << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

// Конструктор
BoxCollider3D::BoxCollider3D(const Vector3& size, ColliderType type)
    : size(size), offset(Vector3(0.0f, 0.0f, 0.0f)), 
      min(Vector3(0.0f, 0.0f, 0.0f)), max(Vector3(0.0f, 0.0f, 0.0f)),
      debugDraw(false), colliderType(type), VAO(0), VBO(0), EBO(0)
{
    // Если шейдерная программа еще не создана, создаем ее
    if (shaderProgram == 0) {
        shaderProgram = loadShaderProgram();
    }
}

// Деструктор
BoxCollider3D::~BoxCollider3D()
{
    // Освобождаем ресурсы OpenGL
    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }
}

// Инициализация компонента
void BoxCollider3D::Init()
{
    // Инициализируем данные для отрисовки в режиме отладки
    InitRenderData();
    
    // Автоматически определяем размер на основе модели, если она есть
    AutoDetectSizeFromModel();
    
    // Обновляем границы коллайдера
    UpdateBounds();
}

// Обновление компонента
void BoxCollider3D::Update(float dt)
{
    // Обновляем границы коллайдера
    UpdateBounds();
    
    // Обрабатываем текущие столкновения
    ProcessCollisions();
    
    // Если включен режим отладки, отрисовываем коллайдер
    if (debugDraw) {
        DebugRender();
    }
}

// Проверка столкновения с другим коллайдером
bool BoxCollider3D::IsColliding(const BoxCollider3D* other) const
{
    // Проверяем пересечение AABB (Axis-Aligned Bounding Box)
    return (min.x <= other->max.x && max.x >= other->min.x) &&
           (min.y <= other->max.y && max.y >= other->min.y) &&
           (min.z <= other->max.z && max.z >= other->min.z);
}

// Обработка текущих столкновений
void BoxCollider3D::ProcessCollisions()
{
    // Этот метод будет вызываться из CollisionManager
    // Реализация находится в CollisionManager.cpp
}

// Обновление границ коллайдера
void BoxCollider3D::UpdateBounds()
{
    if (!object) return;
    
    // Получаем позицию объекта
    Vector3 position = object->GetPosition3D();
    
    // Вычисляем половину размера коллайдера с учетом масштабирования
    Vector3 halfSize = Vector3(size.x / (2.0f * 35), size.y / (2.0f * 35), size.z / (2.0f * 35));
    
    // Вычисляем центр коллайдера - теперь точно соответствует центру модели
    // Используем то же масштабирование, что и в Model3DComponent
    Vector3 center = Vector3(
        position.x/35 + offset.x,
        position.y/35 + offset.y,
        position.z/35 + offset.z
    );
    
    // Вычисляем минимальную и максимальную точки AABB
    min = Vector3(
        center.x - halfSize.x,
        center.y - halfSize.y,
        center.z - halfSize.z
    );
    
    max = Vector3(
        center.x + halfSize.x,
        center.y + halfSize.y,
        center.z + halfSize.z
    );
}

// Инициализация данных для отрисовки
void BoxCollider3D::InitRenderData()
{
    // Вершины куба
    float vertices[] = {
        // Передняя грань
        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        
        // Задняя грань
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f
    };
    
    // Индексы для рисования линий (каркаса куба)
    unsigned int indices[] = {
        // Передняя грань
        0, 1, 1, 2, 2, 3, 3, 0,
        // Задняя грань
        4, 5, 5, 6, 6, 7, 7, 4,
        // Соединения
        0, 4, 1, 5, 2, 6, 3, 7
    };
    
    // Создаем VAO, VBO и EBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    // Привязываем VAO
    glBindVertexArray(VAO);
    
    // Привязываем и заполняем VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // Привязываем и заполняем EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    // Настраиваем атрибуты вершин
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Отвязываем VAO
    glBindVertexArray(0);
}

// Отрисовка коллайдера в режиме отладки
void BoxCollider3D::DebugRender()
{
    if (!object || VAO == 0) return;
    
    // Сохраняем текущее состояние OpenGL
    GLint oldPolygonMode;
    glGetIntegerv(GL_POLYGON_MODE, &oldPolygonMode);
    
    // Устанавливаем режим отрисовки линий
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    
    // Используем шейдерную программу
    glUseProgram(shaderProgram);
    
    // Используем центр коллайдера из рассчитанных границ
    Vector3 center = Vector3(
        (min.x + max.x) / 2.0f,
        (min.y + max.y) / 2.0f,
        (min.z + max.z) / 2.0f
    );
    
    // Получаем матрицы преобразования
    glm::mat4 model = glm::mat4(1.0f);
    
    // Получаем угол поворота объекта
    Vector3 rotation = object->GetAngle();
    
    // Применяем позицию из рассчитанных границ
    model = glm::translate(model, glm::vec3(center.x, center.y, center.z));
    
    // Применяем поворот
    model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    
    // Применяем масштаб, используя размеры из границ
    Vector3 scaledSize = Vector3(
        (max.x - min.x),
        (max.y - min.y),
        (max.z - min.z)
    );
    
    model = glm::scale(model, glm::vec3(scaledSize.x, scaledSize.y, scaledSize.z));
    
    // Получаем матрицы вида и проекции такие же, как в Model3DComponent
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 5.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    
    glm::mat4 projection = glm::perspective(
        glm::radians(90.0f),
        800.0f / 480.0f,
        0.1f,
        100.0f
    );
    
    // Устанавливаем uniform-переменные
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
    GLint colorLoc = glGetUniformLocation(shaderProgram, "color");
    
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    
    // Выбираем цвет в зависимости от типа коллайдера
    switch (colliderType) {
        case ColliderType::Ground:
            glUniform4f(colorLoc, 0.0f, 1.0f, 0.0f, 1.0f); // Зеленый для земли
            break;
        case ColliderType::Water:
            glUniform4f(colorLoc, 0.0f, 0.0f, 1.0f, 1.0f); // Синий для воды
            break;
        case ColliderType::Player:
            glUniform4f(colorLoc, 1.0f, 1.0f, 0.0f, 1.0f); // Желтый для игрока
            break;
        case ColliderType::Enemy:
            glUniform4f(colorLoc, 1.0f, 0.0f, 0.0f, 1.0f); // Красный для врага
            break;
        case ColliderType::Item:
            glUniform4f(colorLoc, 1.0f, 0.5f, 0.0f, 1.0f); // Оранжевый для предметов
            break;
        default:
            glUniform4f(colorLoc, 0.5f, 0.5f, 0.5f, 1.0f); // Серый для остальных
            break;
    }
    
    // Отрисовываем каркас куба
    glBindVertexArray(VAO);
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    
    // Восстанавливаем предыдущее состояние OpenGL
    glPolygonMode(GL_FRONT_AND_BACK, oldPolygonMode);
}

// Метод для автоматического определения размера коллайдера на основе Model3DComponent
void BoxCollider3D::AutoDetectSizeFromModel()
{
    if (!object) return;
    
    // Проверяем, есть ли у объекта компонент Model3DComponent
    Model3DComponent* modelComponent = object->GetComponent<Model3DComponent>();
    if (modelComponent) {
        // Получаем размеры модели
        Vector3 modelDimensions = modelComponent->GetModelDimensions();
        
        // Устанавливаем размер коллайдера равным размеру модели
        // Учитываем, что при отображении размер делится на 35, поэтому умножаем на 35
        size = Vector3(
            modelDimensions.x * 35.0f,
            modelDimensions.y * 35.0f,
            modelDimensions.z * 35.0f
        );
        
        std::cout << "Автоматически установлен размер коллайдера: " 
                  << size.x << ", " << size.y << ", " << size.z << std::endl;
    }
} 