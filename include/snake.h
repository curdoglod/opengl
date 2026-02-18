#pragma once
#include "component.h"
#include "iostream"
#include "math.h"
#include "Scene.h"

static Vector2 Lerp(const Vector2 &start, const Vector2 &end, float t)
{
    return start + (end - start) * t;
}

class SnakeComponent : public Component
{
public:
    SnakeComponent(int block_size)
    {
        this->block_size = block_size;
    }

    void Init() override
    {
        headImg = Engine::GetResourcesArchive()->GetFile("head_down.png");
        bodyImg = Engine::GetResourcesArchive()->GetFile("body.png");
        bodyRotImg = Engine::GetResourcesArchive()->GetFile("bodyRot.png");

        object->AddComponent(new Image(headImg));

        Vector2 startPos(block_size * 8, block_size * 6);
        object->SetPosition(startPos);

        direction = Vector2(0, 1);
        queuedDirection = Vector2(0, 0);
        angle = 0;

        speed = 160.0f;
        moveDuration = (float)block_size / speed;
        moveTimer = 0.0f;

        gridPositions.clear();
        previousGridPositions.clear();
        gridPositions.push_back(startPos);
        Vector2 tailPos = startPos - direction * block_size;
        gridPositions.push_back(tailPos);

        previousGridPositions = gridPositions;

        AddSegment(); 
    }

    void onKeyPressed(SDL_Keycode key) override
    {
        Vector2 newDir(0, 0);
        if (key == SDLK_w)
            newDir = Vector2(0, -1);
        else if (key == SDLK_s)
            newDir = Vector2(0, 1);
        else if (key == SDLK_a)
            newDir = Vector2(-1, 0);
        else if (key == SDLK_d)
            newDir = Vector2(1, 0);
        else
            return;

        if (newDir.x == -direction.x && newDir.y == -direction.y)
            return;

        queuedDirection = newDir;
        UpdateHeadSprite(newDir);
    }

    void onKeyReleased(SDL_Keycode key) override
    {
    }

    void Update(float deltaTime) override
    {
        moveTimer += deltaTime;
        while (moveTimer >= moveDuration)
        {
            moveTimer -= moveDuration;
            previousGridPositions = gridPositions;

            if (queuedDirection.x != 0 || queuedDirection.y != 0)
            {
                direction = queuedDirection;
                queuedDirection = Vector2(0, 0);
            }

            Vector2 newHeadPos = gridPositions[0] + direction * block_size;

            std::vector<Vector2> newGridPositions;
            newGridPositions.push_back(newHeadPos);
            for (size_t i = 1; i < gridPositions.size(); i++)
            {
                newGridPositions.push_back(gridPositions[i - 1]);
            }
            gridPositions = newGridPositions;
        }

        float t = moveTimer / moveDuration;

        Vector2 headPos = Lerp(previousGridPositions[0], gridPositions[0], t);
        object->SetPosition(headPos);

        for (size_t i = 0; i < bodySegments.size(); i++)
        {
            Vector2 segPos = Lerp(previousGridPositions[i + 1], gridPositions[i + 1], t);
            bodySegments[i]->SetPosition(segPos);
            Vector2 moveVec = gridPositions[i + 1] - previousGridPositions[i + 1];

            float segAngle = (fabs(moveVec.x) > fabs(moveVec.y)) ? 90.0f : 0.0f;
            bodySegments[i]->SetRotation(segAngle);
        }
    }

    void AddSegment()
    {
        Vector2 tailPos = gridPositions.back();
        gridPositions.push_back(tailPos);
        previousGridPositions.push_back(tailPos);

        Object *seg = CreateObject();
        seg->AddComponent(new Image(bodyImg));
        seg->SetPosition(tailPos);
        seg->SetLayer(10);
        bodySegments.push_back(seg);
    }

private:
    void UpdateHeadSprite(const Vector2 &dir)
    {
        if (dir.x > 0)
            angle = 270;
        else if (dir.x < 0)
            angle = 90;
        else if (dir.y < 0)
            angle = 180;
        else if (dir.y > 0)
            angle = 0;
        object->SetRotation(angle);
    }

private:
    std::vector<unsigned char> headImg;
    std::vector<unsigned char> bodyImg;
    std::vector<unsigned char> bodyRotImg; 

    Vector2 direction;
    Vector2 queuedDirection;
    float angle;

    float speed;
    int block_size;
    float moveDuration;
    float moveTimer;

    std::vector<Object *> bodySegments;
    std::vector<Vector2> gridPositions;
    std::vector<Vector2> previousGridPositions;
};