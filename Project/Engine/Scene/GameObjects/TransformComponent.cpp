#include "TransformComponent.h"
#include "GameObject.h"

void TransformComponent::applyNow()
{
	if (auto* go = getParent()) go->broadcastTransformChanged();
}