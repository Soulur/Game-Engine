#include "Entity.h"

namespace Mc 
{
	Entity::Entity(entt::entity handle, Scene* scene)
		: m_EntityHandle(handle) ,	m_Scene(scene)
	{
	}

    Entity Entity::CreateChild(const std::string &name)
	{
		// 1. 调用 Scene::CreateEntity 来实际创建 Entity
		Entity child = m_Scene->CreateEntity(name);

		// 2. 确保子 Entity 有 HierarchyComponent，并设置其 Parent UUID
		child.AddOrReplaceComponent<HierarchyComponent>().Parent = GetUUID();

		return child;
	}

	std::vector<Entity> Entity::GetChildren()
	{
		std::vector<Entity> children;
		if (HasComponent<HierarchyComponent>())
		{
			// 注意：这里需要确保 m_Scene 有 GetEntityByUUID 方法
			for (UUID childID : GetComponent<HierarchyComponent>().Children)
			{
				Entity child = m_Scene->GetEntityByUUID(childID);
				if (child)
				{
					children.push_back(child);
				}
			}
		}
		return children;
	}

	void Entity::ClearChildren()
	{
		if (HasComponent<HierarchyComponent>())
		{
			// 遍历并解除子 Entity 的父级引用
			for (UUID childID : GetComponent<HierarchyComponent>().Children)
			{
				Entity child = m_Scene->GetEntityByUUID(childID);
				if (child && child.HasComponent<HierarchyComponent>())
				{
					child.GetComponent<HierarchyComponent>().Parent = 0;
				}
			}

			// 清空父级的 Children 列表
			GetComponent<HierarchyComponent>().Children.clear();
		}
	}
}