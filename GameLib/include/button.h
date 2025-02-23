#ifndef BUTTON_H
#define BUTTON_H

#include "component.h"
#include <functional>
#include <SDL.h>
#include "image.h"

class ButtonComponent : public Component
{
public:
    ButtonComponent(std::function<void()> onClick) : onClick(onClick), image(nullptr)
    {
    }

    ButtonComponent(std::function<void()> onClick, Image *image) : onClick(onClick), image(image)
    {
    }
    void Init() override
    {

        if (!image || !image->GetSprite())
        {
            object->AddComponent(new Image(Engine::GetDefaultArchive()->GetFile("ImageDefault.png")));
        }

        image = object->GetComponent<Image>();
        pos = object->GetPosition();
        size = image->GetSize();
    }

    void Update(float deltaTime) override
    {
    }

    void OnMouseButtonDown(Vector2 mouse_position) override
    {
        pos = object->GetPosition();
        if (mouse_position.x >= pos.x && mouse_position.x <= pos.x + size.x &&
            mouse_position.y >= pos.y && mouse_position.y <= pos.y + size.y)
        {
            onClick();
            image->GetSprite()->SetColorAndOpacity(120, 120, 120, 1.0f);
        }
    }

    void OnMouseButtonUp(Vector2 mouse_position)
    {
        if (mouse_position.x >= pos.x && mouse_position.x <= pos.x + size.x &&
            mouse_position.y >= pos.y && mouse_position.y <= pos.y + size.y)
        {
        }
        image->GetSprite()->SetColorAndOpacity(255, 255, 255, 1.0f);
    }

    void OnMouseButtonMotion(Vector2 mouse_position) override
    {
        pos = object->GetPosition();
        size = image->GetSize();
        if (mouse_position.x >= pos.x && mouse_position.x <= pos.x + size.x &&
            mouse_position.y >= pos.y && mouse_position.y <= pos.y + size.y)
        {

            image->GetSprite()->SetColorAndOpacity(180, 180, 180, 1.0f);
        }
        else
            image->GetSprite()->SetColorAndOpacity(255, 255, 255, 1.0f);
    }

    void SetOnClick(std::function<void()> onClick)
    {
        this->onClick = onClick;
    }
    virtual ButtonComponent *Clone() const override
    {
        return new ButtonComponent(onClick, image);
    }

private:
    Vector2 pos;
    Vector2 size;
    Image *image;
    std::function<void()> onClick;
};

#endif // BUTTON_COMPONENT_H
