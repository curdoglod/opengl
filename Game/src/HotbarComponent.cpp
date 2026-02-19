#include "HotbarComponent.h"
#include "Scene.h"
#include "engine.h"
#include "ArchiveUnpacker.h"
#include "components.h"

void HotbarComponent::Init()
{
    selectedSlot = BlockType::Dirt;
    for (int i = 0; i < 9; ++i)
    {
        Object *slot = CreateObject();
        slot->SetLayer(900);
        slot->SetPosition(Vector2((i - 5) * 40.0f, 0) + object->GetPosition());
        slot->AddComponent(new Image(Engine::GetResourcesArchive()->GetFile("hotbar_slot.png")));
        hotbarSlots.push_back(slot);
    }
    selectedSlotIndex=0; 
    hotbarSlots[selectedSlotIndex]->GetComponent<Image>()->SetNewSprite(Engine::GetResourcesArchive()->GetFile("hotbar_slot_selected.png"));

}

void HotbarComponent::SetSelectedSlot(int slot)
{
    selectedSlotIndex = slot;
    for (size_t i = 0; i < hotbarSlots.size(); ++i)
    {
        hotbarSlots[i]->GetComponent<Image>()->SetNewSprite(Engine::GetResourcesArchive()->GetFile("hotbar_slot.png"));
    }
    selectedSlot = static_cast<BlockType>(slot);
    hotbarSlots[selectedSlotIndex]->GetComponent<Image>()->SetNewSprite(Engine::GetResourcesArchive()->GetFile("hotbar_slot_selected.png"));
}

void HotbarComponent::Update()
{
}

BlockType HotbarComponent::GetSelectedSlot() const
{
    return selectedSlot;
}