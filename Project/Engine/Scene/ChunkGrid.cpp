#include "ChunkGrid.h"
#include <Engine/Scene/GameObjects/BaseObject.h>

glm::vec3 ChunkGrid::getPos(const BaseObject* obj) {
    return const_cast<BaseObject*>(obj)->getPosition(); // your getter is non-const
}
