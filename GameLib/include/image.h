#ifndef IMAGE_H
#define IMAGE_H

#include "component.h"
#include "sprite.h"
#include "SceneManager.h"
#include "engine.h"

class Image : public Component
{
public:
    Image(const std::vector<unsigned char> &imgData)
    {
        this->imgData = imgData;
        sprite = nullptr;
        size = Vector2(0, 0);
    }
    Image(const std::vector<unsigned char> &imgData, Vector2 size)
    {
        this->imgData = imgData;
        sprite = nullptr;
        this->size = size;
    }

    void Init() override
    {
        SetNewSprite(imgData);
    }

    void SetNewSprite(const std::vector<unsigned char> &imgData)
    {
        if (sprite != nullptr)
            delete sprite;
        sprite = object->GetScene()->createSprite(imgData);
        this->imgData = imgData;
        if (!sprite)
        {
            sprite = object->GetScene()->createSprite(Engine::GetDefaultArchive()->GetFile("ImageDefault.png"));
            this->imgData = Engine::GetDefaultArchive()->GetFile("ImageDefault.png");
        }
        if (sprite != nullptr)
        {
            if (size == Vector2(0, 0))
                size = sprite->getSize();
            SetSize(size);

            object->InitSize(this);
        }
    }
    void Update(float deltaTime) override
    {
        if (sprite != nullptr)
            sprite->draw(object->GetPosition(), object->GetAngle());
    }

    void SetSize(const Vector2 &newSize)
    {
        if (sprite != nullptr)
        {
            sprite->setSize((int)newSize.x, (int)newSize.y);
            size = newSize;
            object->InitSize();
        }
        else
        {
            std::cerr << "Sprite is null" << std::endl;
        }
    }

    Vector2 GetSize() const
    {

        return size;
    }

    Sprite *GetSprite() const
    {
        return sprite;
    }

    virtual Image *Clone() const override
    {
        return new Image(imgData, size);
    }

    ~Image()
    {
        delete sprite;
    }

private:
    std::vector<unsigned char> imgData;
    Sprite *sprite;
    Vector2 size;
};

#endif
