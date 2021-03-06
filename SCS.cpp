#include <stdlib.h>
#include <stdio.h>
#include <algorithm>
#include <memory>
#include <iostream>
#include <string>
#include <Windows.h>
#include <Psapi.h>
#include <thread>
#include <random>
#include <functional>
#include <typeindex>
#include <array>
#include "Debug.h"
#include "Memory.h"

// Typical classes to test the component system
class Transform
{
public:

	float x, y;
	char t[0xFFFF] = { 0 };
	Transform(float _x, float _y) : x(_x), y(_y) { std::cout << "Transform component created" << std::endl; }
	void Update() { std::cout << "Transform: " << x << ", " << y << std::endl; }
};

class Collider
{
public:

	float x, y;
	char t[0xFFFF] = { 0 };
	Collider(float _x, float _y) : x(_x), y(_y) { std::cout << "Collider component created" << std::endl; }
	void Update() { std::cout << "Collider: " << x << ", " << y << std::endl; }
};

class MeshRenderer
{
public:

	float x, y;
	char t[0xFFFF] = { 0 };
	MeshRenderer(float _x, float _y) : x(_x), y(_y) { std::cout << "MeshRenderer component created" << std::endl; }
	void Update() { std::cout << "MeshRenderer: " << x << ", " << y << std::endl; }
};

class Script
{
public:

	float x, y;
	char t[0xFFFF] = { 0 };
	Script(float _x, float _y) : x(_x), y(_y) { std::cout << "Script component created" << std::endl; }
	void Update() { std::cout << "Script: " << x << ", " << y << std::endl; }
};

// The class that will handle the ptr to the real component class
template<typename T>
class ComponentHandle
{
private:
	T* component = nullptr;

public:

	ComponentHandle() {}
	ComponentHandle(T* component) : component(component) {}

	T* operator -> () const	{ return component; }
	operator bool() const	{ return IsValid(); }
	bool IsValid() const	{ return component != nullptr; }
};

// Defined components types for future sorting etc
enum eComponentType
{
	TRANSFORM,
	MESH_RENDERER,
	COLLIDER,
	SCRIPT,
	UNKNOWN,
};

// The abstract class base of the ComponentContainer class
class BaseComponentContainer
{
public:
	eComponentType type = UNKNOWN;
	virtual void Destroy() = 0;
	virtual void Update() = 0;
};

template <typename T>
class ComponentContainer : public BaseComponentContainer
{
public:
	T data = nullptr;

	ComponentContainer() {}
	ComponentContainer(const eComponentType& priority, const T& data) : data(data) { type = priority; }
	~ComponentContainer() {}

	// Destroy function for a component
	void Destroy()
	{
		std::cout << "Component " << std::hex << this << " DEALLOCATED" << std::endl;
		memory::Deallocate(this);
	}
	
	// Update function for a component
	void Update()
	{
		ComponentHandle<T>(&data)->Update();
	}
};

class GameObject
{
private:
	// Vector of components
	std::vector<BaseComponentContainer*> components;

	// Test values
	float val = 0.f;
	std::string name;

public:

	GameObject(std::string name, float val) : name(name), val(val)
	{
		std::cout << "GameObject " << std::hex << this << " ALLOCATED (name: " << name << " | val: " << val << ")" << std::endl;
	}
	~GameObject()
	{
		for (auto comp : components)
			comp->Destroy();
		std::cout << "GameObject " << std::hex << this << " DEALLOCATED" << std::endl;
	}

	// Get a component with the type specified (TODO: add index parameter in case of multi-component)
	template <typename T>
	ComponentHandle<T> GetComponent(const eComponentType& type)
	{
		auto it = std::find_if(components.begin(), components.end(), [&](BaseComponentContainer* component) { return component->type == type; });
		return ComponentHandle<T>(it != components.end() ? (&reinterpret_cast<ComponentContainer<T>*>(*it)->data) : nullptr);
	}

	// Checks if there is a component of the type passed
	bool HasComponent(const eComponentType& type)
	{
		return std::find_if(components.begin(), components.end(), [&](BaseComponentContainer* component) { return component->type == type; }) != components.end();
	}

	// Checks whether a component type can be created multiple times or not in the same game object
	bool IsMultiComponent(const eComponentType& type)
	{
		switch (type)
		{
		case TRANSFORM:
		case COLLIDER:
		case MESH_RENDERER: return false;
		case SCRIPT:		return true;
		}
		return false;
	}

	// Adds a component (we pass the type of the component as priority reference so we can sort the components. Useful to update components in certain order)
	template <typename T, typename... Args>
	ComponentHandle<T> AddComponent(const eComponentType& type, Args&&... args)
	{
		// Check if the component can be added
		if (!IsMultiComponent(type) && HasComponent(type))
			return ComponentHandle<T>();

		// Allocate the component container with my simple memory allocator class
		auto container = memory::Allocate<ComponentContainer<T>>(type, T(args...));
		components.push_back(container);

		// Sort the components in the vector for time-execution order
		std::sort(components.begin(), components.end(), [&](BaseComponentContainer* a1, BaseComponentContainer* a2) { return a1->type < a2->type; });

		std::cout << "Component: " << memory::GetTypeIndex<T>().name() << " (" << std::hex << container << ")" << " added" << std::endl;

		// Return the component handle we will use to retrieve component data
		return ComponentHandle<T>(&container->data);
	}

	// Print components type of this game object
	void PrintComponents()
	{
		std::cout << "-------- Components --------\n";
		for (auto comp : components)
			std::cout << "Component " << comp->type << std::endl;
		std::cout << "----------------------------\n";
	}

	// Update all components (TODO: Specify what components we want to update)
	void Update()
	{
		for (auto comp : components)
			comp->Update();
	}

	std::string GetName() const		{ return name; }

	float GetVal() const			{ return val; }
};

int main()
{
	std::mt19937_64 mt(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count());
	std::thread t1(debug);

	while (true)
	{
		// Create a game object for testing
		auto data = memory::Allocate<GameObject>("game_object_name_1", std::uniform_real_distribution<float>(0.f, 1000.f)(mt));

		// Add components to it
		data->AddComponent<Script>(SCRIPT, 1.f, 2.f);				// add success
		data->AddComponent<Script>(SCRIPT, 1.f, 2.f);				// add success
		data->AddComponent<Script>(SCRIPT, 1.f, 2.f);				// add success
		data->AddComponent<Collider>(COLLIDER, 2.f, 3.f);			// add success
		data->AddComponent<Collider>(COLLIDER, 2.f, 3.f);			// add failed (only 1 component of this type allowed)
		data->AddComponent<Collider>(COLLIDER, 2.f, 3.f);			// add failed (only 1 component of this type allowed)
		data->AddComponent<MeshRenderer>(MESH_RENDERER, 4.f, 5.f);	// add success
		data->AddComponent<MeshRenderer>(MESH_RENDERER, 4.f, 5.f);	// add failed (only 1 component of this type allowed)
		data->AddComponent<MeshRenderer>(MESH_RENDERER, 4.f, 5.f);	// add failed (only 1 component of this type allowed)
		data->AddComponent<Transform>(TRANSFORM, 6.f, 7.f);			// add success
		data->AddComponent<Transform>(TRANSFORM, 6.f, 7.f);			// add failed (only 1 component of this type allowed)
		data->AddComponent<Transform>(TRANSFORM, 6.f, 7.f);			// add failed (only 1 component of this type allowed)
	
		data->PrintComponents();

		// Testing GetComponent
		{
			auto component = data->GetComponent<Script>(SCRIPT);
			if (component)
				std::cout << "Script: " << component->x << ", " << component->y << std::endl;
		}
		{
			auto component = data->GetComponent<Collider>(COLLIDER);
			if (component)
				std::cout << "Collider: " << component->x << ", " << component->y << std::endl;
		}
		{
			auto component = data->GetComponent<MeshRenderer>(MESH_RENDERER);
			if (component)
				std::cout << "MeshRenderer: " << component->x << ", " << component->y << std::endl;
		}
		{
			auto component = data->GetComponent<Transform>(TRANSFORM);
			if (component)
				std::cout << "Transform: " << component->x << ", " << component->y << std::endl;
		}

		// Updating all the components
		data->Update();

		std::cin.get();

		// Destroy and deallocate the game object and all it's components
		memory::Deallocate(data);

		std::cin.get();
	}

    return 0;
}