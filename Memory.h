#pragma once

#include <memory>
#include <typeindex>

class memory
{
private:

public:

	using main_alloc = std::allocator<void*>;
	using type = std::type_index;

	static main_alloc allocator;

	template <typename T, typename... Args>
	static T* Allocate(Args&&... args)
	{
		using custom_allocator = std::allocator_traits<main_alloc>::template rebind_alloc<T>;
		custom_allocator alloc(allocator);

		if (auto data = std::allocator_traits<custom_allocator>::allocate(alloc, 1))
		{
			std::allocator_traits<custom_allocator>::construct(alloc, data, args...);
			return data;
		}
		return nullptr;
	}

	template <typename T>
	static void Deallocate(T* data)
	{
		if (!data)
			return;

		using CustomAllocator = std::allocator_traits<main_alloc>::template rebind_alloc<T>;
		CustomAllocator alloc(allocator);

		std::allocator_traits<CustomAllocator>::destroy(alloc, data);
		std::allocator_traits<CustomAllocator>::deallocate(alloc, data, 1);
	}

	template <typename T>
	static type GetTypeIndex() { return std::type_index(typeid(T)); }

};