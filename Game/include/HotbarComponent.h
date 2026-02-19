#ifndef HotbarComponent_H
#define HotbarComponent_H
#include "component.h" 
#include "BlockComponent.h" 


class HotbarComponent : public Component
{
public:
    void Init() override;
    void Update() override;     
    BlockType GetSelectedSlot() const;
    void SetSelectedSlot(int slot);
private: 
    BlockType selectedSlot = BlockType::Grass;
    int selectedSlotIndex = 0;
    std::vector<Object*> hotbarSlots;
}; 

#endif // HotbarComponent_H