#pragma once

#include <memory>
#include <cassert>
#include <algorithm>
#include <utility>
#include <cstddef>

namespace std { enum class byte : unsigned char; }
struct validate_byte {
	// <cstddef> must be included in order for this check to work properly
	template<
		class T, T Val = std::declval<T>(),
		class Op1 = decltype(Val <<= Val),
		class Op2 = decltype(Val >>= Val),
		class Op3 = decltype(Val << Val),
		class Op4 = decltype(Val >> Val),
		class Op5 = decltype(Val |= Val),
		class Op6 = decltype(Val &= Val),
		class Op7 = decltype(Val ^= Val),
		class Op8 = decltype(Val | Val),
		class Op9 = decltype(Val & Val),
		class Op10 = decltype(Val ^ Val),
		class Op11 = decltype(~Val)>
	static std::true_type test(int);
	template<class T>
	static std::false_type test(...);
	using type = std::conditional_t<decltype(test<std::byte>(0))::value, std::byte, unsigned char>;
};
using byte_t = typename validate_byte::type;

template<class It>
struct is_input_iterator : std::is_base_of<std::input_iterator_tag, typename It::iterator_category> {};
template<class It>
using is_input_iterator_t = is_input_iterator<It>;

struct Storage {
	struct Chunk {
		const std::unique_ptr<Chunk> pred;
		const std::unique_ptr<byte_t[]> space;
		size_t capacity;
		byte_t* avail;
		Chunk(size_t size, std::unique_ptr<Chunk>&& pred)
			: pred(std::move(pred))
			, space(std::make_unique<byte_t[]>(size))
			, capacity(size)
			, avail(space.get()) {}
		byte_t* allocate(size_t amount) { 
			if (amount > capacity) throw std::bad_alloc();
			return capacity -= amount, std::exchange(avail, avail + amount);
		}
	};
	const size_t chunk_size;
	std::unique_ptr<Chunk> top_chunk = std::make_unique<Chunk>(chunk_size, nullptr);

	Storage(size_t chunk_size) : chunk_size(chunk_size) {}
	byte_t* allocate(size_t n, size_t type_size) try {
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
 
template <class T>
struct Node {
	uintptr_t xor_;
	T data;

	template<class U>
	explicit Node(U&& data, Node* left, Node* right) 
		: xor_(reinterpret_cast<uintptr_t>(left) ^ reinterpret_cast<uintptr_t>(right))
		, data(std::forward<U>(data)) {}
	Node* upd_sibling(Node* target, Node* replacement) { // xor is associative
		xor_ ^= reinterpret_cast<uintptr_t>(target) ^ reinterpret_cast<uintptr_t>(replacement);
		return this;
	}
	Node* get_complement(Node* ptr) const {
		const uintptr_t value = reinterpret_cast<uintptr_t>(ptr) ^ xor_;
		return reinterpret_cast<Node*>(value);
	}
};
 
template<class T, class Allocator = std::allocator<T>>
class XorList {
	Node<T>* first = nullptr;
	Node<T>* last = nullptr;
	size_t size_ = 0;
	using node_alloc_t = typename std::allocator_traits<Allocator>::template rebind_traits<Node<T>>::allocator_type;
	using node_alloc_traits = typename std::allocator_traits<node_alloc_t>;
	node_alloc_t node_alloc;// = typename std::allocator_traits<Allocator>::template rebind_alloc<Node<T>>(alloc);
	template<bool IsConst>
	struct iterator_t {
		using iterator_category = std::bidirectional_iterator_tag;
		using value_type = std::conditional_t<IsConst, const T, T>;
		using difference_type = ptrdiff_t;
		using pointer = value_type*;
		using reference = value_type&;

		iterator_t() = default;
		explicit iterator_t(Node<T>* me, Node<T>* prev) : node(me), prev_node(prev) {}
		template<bool IsOtherConst, class = std::enable_if_t<IsConst || !IsOtherConst, int>>
		iterator_t(const iterator_t<IsOtherConst>& other) : node(other.node) , prev_node(other.prev_node) {}
		template<bool IsOtherConst>
		bool operator==(const iterator_t<IsOtherConst>& it) const { return node == it.node; }
		template<bool IsOtherConst>
		bool operator!=(const iterator_t<IsOtherConst>& it) const { return !(*this == it); }
		iterator_t& operator++() {
			prev_node = std::exchange(node, node->get_complement(prev_node));
			return *this;
		}
		iterator_t operator++(int) {
			const iterator_t original(node, prev_node);
			++*this;
			return original;
		}
		iterator_t& operator--() {
			node = std::exchange(prev_node, prev_node ? prev_node->get_complement(node) : nullptr);
			return *this;
		}
		iterator_t operator--(int) {
			const iterator_t original(node, prev_node);
			--*this;
			return original;
		}
		reference operator*() const { 
			assert(node);
			return node->data;
		}
		pointer operator->() const { return &node->data; }
		std::conditional_t<IsConst, const Node<T>, Node<T>>* get_node() const { return node; }
		std::conditional_t<IsConst, const Node<T>, Node<T>>* get_prev_node() const { return prev_node; }
		explicit operator bool() const { return node; }
	private:
		Node<T>* node = nullptr;
		Node<T>* prev_node = nullptr;
		template<bool> friend class iterator_t;
	};
public:
	using iterator = iterator_t<false>;
	using const_iterator = iterator_t<true>;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;
	using value_type = T;
	using allocator_type = Allocator;
	using size_type = size_t;
	using difference_type = ptrdiff_t;
	using reference = value_type; 
	using const_reference = const value_type&;
	using pointer = typename std::allocator_traits<Allocator>::pointer;
	using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;

	explicit XorList(const Allocator& alloc = Allocator()) : node_alloc(alloc) {}
	XorList(std::size_t count, const T& value, const Allocator& alloc = Allocator()) : node_alloc(alloc) {
		std::fill_n(std::back_inserter(*this), count, value);
	}
	XorList(const std::initializer_list<T>& init, const Allocator& alloc = Allocator()) : node_alloc(alloc) {
		std::move(init.begin(), init.end(), std::back_inserter(*this));
	}
	XorList(const XorList& other)
       		: node_alloc(node_alloc_traits::select_on_container_copy_construction(other.node_alloc)) { 
		std::copy(other.begin(), other.end(), std::back_inserter(*this));
	}
	XorList(XorList&& other)
		: first(other.first)
		, last(other.last)
		, size_(other.size_)
		, node_alloc(std::move(other.node_alloc)) {
		other.size_ = 0,
		other.first = other.last = nullptr;
	}
	XorList& operator=(const XorList& other) {
		if (this != &other) {
			if constexpr(node_alloc_traits::propagate_on_container_copy_assignment::value) {
				if (node_alloc != other.node_alloc) clear();
				node_alloc = other.node_alloc;
			}
			assign(other.begin(), other.end());
		}
		return *this;
	}
	XorList& operator=(XorList&& other) {
		if (node_alloc_traits::propagate_on_container_move_assignment::value
			|| node_alloc_traits::is_always_equal::value
		   	|| node_alloc == other.node_alloc) {
			clear();
			node_alloc = std::move(other.node_alloc);
			splice(end(), std::move(other));
		} else assign(std::make_move_iterator(other.begin()), std::make_move_iterator(other.end()));
		return *this;
	}
	~XorList() { clear(); }
	bool operator==(const XorList& other) const {
		return size() == other.size() && std::equal(begin(), end(), other.begin(), other.end());
	}
	bool operator!=(const XorList& other) const { return !(*this == other); }
	void splice(iterator pos, XorList&& other) {
		using std::prev;
		other.begin().get_node()->upd_sibling(nullptr, prev(pos).get_node());
		prev(other.end()).get_node()->upd_sibling(nullptr, pos.get_node());
		prev(pos).get_node()->upd_sibling(pos.get_node(), other.begin().get_node());
		pos.get_node()->upd_sibling(prev(pos).get_node(), prev(other.end()).get_node());
		size_ += other.size_;
		other.size_ = 0;
		other.first = other.last = nullptr;
	}
	template<class U, class InputIterator, class = std::enable_if_t<is_input_iterator_t<InputIterator>::value>>
	iterator insert(InputIterator it, U&& value) {
		Node<T>* const node = node_alloc_traits::allocate(node_alloc, 1);
		node_alloc_traits::construct(
			node_alloc, node, std::forward<U>(value), std::prev(it).get_node(), it.get_node());
		if (it == begin()) first = node;
		else std::prev(it).get_node()->upd_sibling(it.get_node(), node);
		if (it == end()) last = node;
		else it.get_node()->upd_sibling(it.get_prev_node(), node);
		++size_;
		return iterator(node, node->get_complement(it.get_node()));
	}
	template<class InputIterator, class = std::enable_if_t<is_input_iterator_t<InputIterator>::value>>
	void assign(InputIterator beg_in, InputIterator end_in) {
		iterator out = begin();
		InputIterator in = beg_in;
		for (const iterator cont_end = end(); out != end() && in != end_in; *out++ = *in++) {}
		if (out == end()) std::copy(in, end_in, std::back_inserter(*this));
		else while (out != end()) out = erase(out);
	}
	template<class U>
	void push_back(U&& value) { insert(end(), std::forward<U>(value)); }
	template<class U>
	void push_front(U&& value) { insert(begin(), std::forward<U>(value)); }
	iterator erase(iterator it) {
		if (it != begin())
			std::prev(it).get_node()->upd_sibling(it.get_node(), std::next(it).get_node());
		else first = std::next(it).get_node();
		if (it != std::prev(end()))
			std::next(it).get_node()->upd_sibling(it.get_node(), std::prev(it).get_node());
		else last = std::prev(it).get_node();

		const iterator next = iterator(std::next(it).get_node(), std::prev(it).get_node());
		std::allocator_traits<node_alloc_t>::destroy(node_alloc, it.get_node());
		std::allocator_traits<node_alloc_t>::deallocate(node_alloc, it.get_node(), 1);
		--size_;
		return next;
	}
	void swap(XorList& other) {
		std::swap(first, other.first);
		std::swap(last, other.last);
		std::swap(size_, other.size_);
		if constexpr(node_alloc_traits::propagate_on_container_swap::value) std::swap(node_alloc, other.node_alloc);
	}
	void pop_back() { erase(std::prev(end())); }
	void pop_front() { erase(begin()); }
	void clear() { for (auto it = begin(); it != end(); ) it = erase(it); }
	T& front() { return *begin(); }
	T& back() { return *std::prev(end()); }
	const T& front() const { return *begin(); }
	const T& back() const { return *std::prev(end()); }
	iterator begin() { return iterator(first, nullptr); }
	iterator end() { return iterator(nullptr, last); }
	const_iterator begin() const { return const_iterator(first, nullptr); }
	const_iterator end() const { return const_iterator(nullptr, last); }
	const_iterator cbegin() const { return begin(); }
	const_iterator cend() const { return end(); }
	reverse_iterator rbegin() { return reverse_iterator(end()); }
	reverse_iterator rend() { return reverse_iterator(begin()); }
	const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
	const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
	const_reverse_iterator crbegin() const { return rbegin(); }
	const_reverse_iterator crend() const { return rend(); }
	size_t size() const { return size_; }
};
