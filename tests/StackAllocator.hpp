#pragma once

#include <cstddef>
#include <memory>
#include <utility>
#include <algorithm>

struct Storage {
	struct Chunk {
		const std::unique_ptr<Chunk> pred;
		const std::unique_ptr<std::byte[]> space;
		size_t capacity;
		std::byte* avail;
		Chunk(size_t size, std::unique_ptr<Chunk>&& pred)
			: pred(std::move(pred))
			, space(std::make_unique<std::byte[]>(size))
			, capacity(size)
			, avail(space.get()) {}
		std::byte* allocate(size_t amount) {
			if (amount > capacity) throw std::bad_alloc();
			return capacity -= amount, std::exchange(avail, avail + amount);
		}
	};
	const size_t chunk_size;
	std::unique_ptr<Chunk> top_chunk = std::make_unique<Chunk>(chunk_size, nullptr);

	Storage(size_t chunk_size) : chunk_size(chunk_size) {}
	std::byte* allocate(size_t n, size_t type_size) try {
		return top_chunk->allocate(n * type_size);
	} catch (std::bad_alloc&) {
		top_chunk = std::make_unique<Chunk>(chunk_size, std::move(top_chunk));
		return top_chunk->allocate(n * type_size);
	}
};

template<class T>
struct StackAllocator {
	const size_t chunk_size;
	const std::shared_ptr<Storage> storage;

	using value_type = T;
	using pointer = T*;
	using const_pointer = const pointer;
	using reference = T&;
	using const_reference = const T&;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	template<class U>
	struct rebind { typedef StackAllocator<U> other; };

	explicit StackAllocator(size_t chunk_size = 1e8)
		: chunk_size(chunk_size)
		, storage(std::make_shared<Storage>(chunk_size)) {}
	template<class> friend class StackAllocator;
	template<class U>
	StackAllocator(const StackAllocator<U>& other) : chunk_size(other.chunk_size), storage(other.storage) {}

	T* allocate(size_t n) { return reinterpret_cast<T*>(storage->allocate(n, sizeof(T))); }
	void deallocate(T*, size_t) {}
	size_t max_size() const { return chunk_size; }
};