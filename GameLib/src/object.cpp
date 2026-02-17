#include "object.h"
#include "component.h"
#include "image.h"
#include "Utils.h"
#include <typeinfo>
#include "SceneManager.h"

Object::Object(SceneManager *scene) : currentScene(scene)
{
	deltatime = 0;
	layer = 0;
	SetRotation(0);
	active = true;
}

bool Object::Crossing(Object *obj, const float &x_range, const float &y_range)
{
	Vector2 temp_pos = obj->GetPosition();
	Vector2 temp_size = obj->GetSize();

	return ((temp_pos.y + temp_size.y * y_range) >= (position.y)) &&
		   (temp_pos.y) <= (position.y + size.y * y_range) &&
		   (temp_pos.x + temp_size.x * x_range >= position.x) && temp_pos.x <= position.x + size.x * x_range;
}

bool Object::Crossing(Object *obj)
{
	return Crossing(obj, 1.0f, 1.0f);
}

Vector2 Object::GetPosition() const
{
	return position.toVector2();
}
Vector3 Object::GetPosition3D() const
{
	return position;
}
void Object::SetPosition(const Vector2 &vec2)
{
	position.x = vec2.x;
	position.y = vec2.y;
}

void Object::SetPosition(const Vector3 &vec3)
{
	position = vec3;
}

void Object::SetRotation(float angle)
{
	this->angle.z = angle;
}
void Object::SetRotation(const Vector3& angle)
{
	this->angle = angle;
}
void Object::SetPositionOnPlatform(const Vector2 &vec2)
{
	SetPosition(Vector2(vec2.x, vec2.y - size.y));
}

void Object::MoveY(const float &pos_y)
{
	position.y += pos_y;
}

void Object::MoveX(const float &pos_x)
{
	position.x += pos_x;
}

Vector2 Object::GetSize()
{
	return size.toVector2();
}

void Object::InitSize(Image *img)
{
	if (img != nullptr)
		size = img->GetSize().toVector3();
}

void Object::SetSize(const Vector2 &vec2)
{
	size = vec2.toVector3();
}

void Object::SetSize(const Vector3 &vec3)
{
	size = vec3;
}

void Object::InitSize()
{
	Image *img = GetComponent<Image>();
	if (img != nullptr)
		size = img->GetSize().toVector3();
}

void Object::SetLayer(int newLayer)
{
	layer = newLayer;
	currentScene->updateLayer();
}

int Object::GetLayer() const
{
	return layer;
}

void Object::AddComponent(Component *component)
{
	for (auto it = components.begin(); it != components.end();)
	{
		if (typeid(*component) == typeid(**it))
		{
			delete *it;
			it = components.erase(it);
		}
		else
		{
			++it;
		}
	}
	component->setOwner(this);
	component->Init();
	components.push_back(component);
}

Component *Object::GetComponent(const std::type_info &ti) const
{
	for (auto &component : components)
	{
		if (typeid(*component) == ti)
		{
			return component;
		}
	}
	return nullptr;
}

template <typename T>
void Object::RemoveComponent()
{
	Component *comp = this->GetComponent(typeid(T));
	if (comp != nullptr)
	{
		delete comp;

		components.erase(std::remove(components.begin(), components.end(), comp), components.end());
	}
}

void Object::update(float deltaTime)
{
	for (auto &component : components)
	{
		component->Update();
		component->Update(deltaTime);
	}
	this->deltatime = deltaTime;
}

void Object::lateUpdate(float deltaTime)
{
	for (auto &component : components)
	{
		component->LateUpdate(deltaTime);
	}
}

SceneManager *Object::GetScene() const
{
	return currentScene;
}

void Object::UpdateEvents(SDL_Event &event)
{
	for (auto &component : components)
	{
		int x, y;
		SDL_GetMouseState(&x, &y);
		if (event.type == SDL_MOUSEBUTTONDOWN)
			component->OnMouseButtonDown(Vector2((float)x, (float)y));
		else if (event.type == SDL_MOUSEBUTTONUP)
			component->OnMouseButtonUp(Vector2((float)x, (float)y));
		else if (event.type == SDL_MOUSEMOTION)
			component->OnMouseButtonMotion(Vector2((float)x, (float)y));
		else if (event.type == SDL_KEYDOWN)
			component->onKeyPressed(event.key.keysym.sym);
		else if (event.type == SDL_KEYUP)
			component->onKeyReleased(event.key.keysym.sym);
	}
}

void Object::SetActive(bool status)
{

	active = status;
}

Object *Object::CloneObject() const
{
	Object *clone = currentScene->CreateObject();
	clone->position = position;
	clone->size = size;
	clone->angle = angle;
	clone->layer = layer;
	clone->active = active;
	clone->deltatime = deltatime;
	for (auto *comp : components)
	{
		if (comp)
		{
			Component *compClone = comp->Clone();
			if (compClone != nullptr)
			{
				clone->AddComponent(compClone);
			}
		}
	}
	return clone;
}

Object::~Object()
{
	for (auto *component : components)
	{
		delete component;
	}
	components.clear();
}
