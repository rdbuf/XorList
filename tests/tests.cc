#include "gtest/gtest.h"

#include "XorList.hpp"
#include "StackAllocator.hpp"

#include <list>
#include <type_traits>
#include <utility>
#include <iterator>
#include <random>

// clang++ -std=c++17 -I../include tests.cc -lgtest -lpthread -lgtest_main

namespace ignore_access_rights {
template<class Ptr, int Id>
Ptr result = nullptr;
template<class Ptr, Ptr Target, int Id>
struct apply {
	struct exec { exec() { result<Ptr, Id> = Target; } };
	static exec var;
};
template<typename Ptr, Ptr Target, int Id>
typename apply<Ptr, Target, Id>::exec apply<Ptr, Target, Id>::var;
} // namespace ignore_access_rights

template struct ignore_access_rights::apply<Node<int>* XorList<int>::*, &XorList<int>::first, 0>;
template struct ignore_access_rights::apply<Node<int>* XorList<int>::*, &XorList<int>::last, 1>;

TEST(XorList, DefCtor) {
	XorList<int> l;
	auto first = ignore_access_rights::result<Node<int>* XorList<int>::*, 0>;
	auto last = ignore_access_rights::result<Node<int>* XorList<int>::*, 1>;
	ASSERT_EQ(l.*first, l.*last);
	ASSERT_EQ(l.*last, nullptr);
	ASSERT_EQ(l.size(), 0);
}

TEST(XorList, FillingCtor) {
	XorList<int> l(5, 8);
	XorList<int> k{8,8,8,8,8};
	ASSERT_EQ(l, k);
}

TEST(XorList, InitListCtor) {
	XorList<int> l{1,2,3,4,5};
	std::list<int> std_l{1,2,3,4,5};
	ASSERT_TRUE(std::equal(l.begin(), l.end(), std_l.begin(), std_l.end()));
}

template<class T1, class Alloc1, class T2, class Alloc2>
bool operator==(const XorList<T1, Alloc1>& lhs, const std::list<T2, Alloc2>& rhs) {
	return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

TEST(XorList, CopyCtor) {
	XorList<int> l{1,2,3,4,5};
	XorList<int> copy_cted_l(l);
	ASSERT_EQ(l, copy_cted_l);
}

TEST(XorList, MoveCtor) {
	XorList<int> l{1,2,3,4,5};
	XorList<int> copy_cted_l(l);
	XorList<int> moved_to_l(std::move(l));
	auto first = ignore_access_rights::result<Node<int>* XorList<int>::*, 0>;
	auto last = ignore_access_rights::result<Node<int>* XorList<int>::*, 1>;
	ASSERT_EQ(moved_to_l, copy_cted_l);
	ASSERT_EQ(l.*first, l.*last);
	ASSERT_EQ(l.*last, nullptr);
	ASSERT_EQ(l.size(), 0);
}

TEST(XorList, CopyAssign) {
	XorList<int> l{1,2,3,4,5};
	XorList<int> copy_assigned_l = l;
	ASSERT_EQ(l, copy_assigned_l);
}

TEST(XorList, MoveAssign) {
	XorList<int> l{1,2,3,4,5};
	XorList<int> copy_cted_l(l);
	XorList<int> move_assigned_l = std::move(l);
	auto first = ignore_access_rights::result<Node<int>* XorList<int>::*, 0>;
	auto last = ignore_access_rights::result<Node<int>* XorList<int>::*, 1>;
	ASSERT_EQ(move_assigned_l, copy_cted_l);
	ASSERT_EQ(l.*first, l.*last);
	ASSERT_EQ(l.*last, nullptr);
	ASSERT_EQ(l.size(), 0);
}

TEST(XorList, SpliceRval) {
	using std::next, std::move;
	XorList<int> l{1,2,3,4,5};
	XorList<int> k{6,7,8,9,0};
	std::list<int> std_l{1,2,3,4,5};
	std::list<int> std_k{6,7,8,9,0};
	k.splice(next(k.begin(), 2), move(l));
	std_k.splice(next(std_k.begin(), 2), move(std_l));
	auto first = ignore_access_rights::result<Node<int>* XorList<int>::*, 0>;
	auto last = ignore_access_rights::result<Node<int>* XorList<int>::*, 1>;
	std::cerr << k.size() << " " << std_k.size() << std::endl;
	ASSERT_EQ(k, std_k);
	ASSERT_EQ(l.*first, l.*last);
	ASSERT_EQ(l.*last, nullptr);
	ASSERT_EQ(l.size(), 0);
}

struct S {
	enum class State { default_cted, copy_cted, move_cted, moved_from };
	State state;
	S() : state(State::default_cted) {}
	S(const S& other) : state(State::copy_cted) {}
	S(S&& other) : state(State::move_cted) { other.state = State::moved_from; }
	S& operator=(const S& other) = default;
	S& operator=(S&& other) = default;
};

TEST(XorList, InsertLval) {
	XorList<S> l;
	S s;
	l.insert(l.begin(), s);
	ASSERT_EQ(l.front().state, S::State::copy_cted);
}

TEST(XorList, InsertRval) {
	XorList<S> l;
	l.insert(l.begin(), S());
	ASSERT_EQ(l.front().state, S::State::move_cted);
}

template<class T1, class T2, class = void>
constexpr bool insert_of_such_parameters_exists = false;

template<class T1, class T2>
constexpr bool insert_of_such_parameters_exists<T1, T2, decltype(&XorList<int>::insert<T1, T2>)> = true;

TEST(XorList, InsertSupportsInputIteratorsOnly) {
	ASSERT_FALSE((insert_of_such_parameters_exists<int, std::insert_iterator<std::list<int>>>));
}

TEST(XorList, PushBackLval) {
	XorList<S> l;
	S s;
	l.push_back(s);
	ASSERT_EQ(l.back().state, S::State::copy_cted);
}

TEST(XorList, PushBackRval) {
	XorList<S> l;
	l.push_back(S());
	ASSERT_EQ(l.back().state, S::State::move_cted);
}

TEST(XorList, PushFrontLval) {
	XorList<S> l;
	S s;
	l.push_front(s);
	ASSERT_EQ(l.front().state, S::State::copy_cted);
}

TEST(XorList, PushFrontRval) {
	XorList<S> l;
	l.push_front(S());
	ASSERT_EQ(l.front().state, S::State::move_cted);
}

TEST(XorList, PopBack) {
	XorList<int> l{1};
	l.pop_back();
	auto first = ignore_access_rights::result<Node<int>* XorList<int>::*, 0>;
	auto last = ignore_access_rights::result<Node<int>* XorList<int>::*, 1>;
	ASSERT_EQ(l.*first, l.*last);
	ASSERT_EQ(l.*last, nullptr);
	ASSERT_EQ(l.size(), 0);
}

TEST(XorList, PopFront) {
	XorList<int> l{1};
	l.pop_front();
	auto first = ignore_access_rights::result<Node<int>* XorList<int>::*, 0>;
	auto last = ignore_access_rights::result<Node<int>* XorList<int>::*, 1>;
	ASSERT_EQ(l.*first, l.*last);
	ASSERT_EQ(l.*last, nullptr);
	ASSERT_EQ(l.size(), 0);
}

TEST(XorList, AssignCopy) {
	XorList<S> l{S()};
	XorList<S> k;
	k.assign(l.begin(), l.end());
	ASSERT_EQ(k.front().state, S::State::copy_cted);
}

TEST(XorList, AssignMove) {
	using std::make_move_iterator;
	XorList<S> l{S()};
	XorList<S> k;
	k.assign(make_move_iterator(l.begin()), make_move_iterator(l.end()));
	ASSERT_EQ(k.front().state, S::State::move_cted);
}

template<class T1, class = void>
constexpr bool assign_of_such_parameters_exists = false;

template<class T1>
constexpr bool assign_of_such_parameters_exists<T1, decltype(&XorList<int>::assign<T1>)> = true;

TEST(XorList, AssignSupportsInputIteratorsOnly) {
	ASSERT_FALSE((assign_of_such_parameters_exists<std::insert_iterator<std::list<int>>>));
}

TEST(XorList, Swap) {
	using std::equal;
	XorList<int> l{1,2,3,4,5};
	XorList<int> k{6,7,8,9,0};
	XorList<int> l_copy = l;
	XorList<int> k_copy = k;
	l.swap(k);
	ASSERT_EQ(l, k_copy);
	ASSERT_EQ(k, l_copy);
}

TEST(XorList, Clear) {
	XorList<int> l{1,2,3,4,5};
	auto first = ignore_access_rights::result<Node<int>* XorList<int>::*, 0>;
	auto last = ignore_access_rights::result<Node<int>* XorList<int>::*, 1>;
	l.clear();
	ASSERT_EQ(l.*first, l.*last);
	ASSERT_EQ(l.*last, nullptr);
	ASSERT_EQ(l.size(), 0);
}

TEST(XorList, ReturnTypeCorrectness) {
	using std::is_same_v;
	XorList<int> l;
	const XorList<int> k;
	ASSERT_TRUE((is_same_v<decltype(l.begin()), XorList<int>::iterator>));
	ASSERT_TRUE((is_same_v<decltype(l.end()), XorList<int>::iterator>));
	ASSERT_TRUE((is_same_v<decltype(l.cbegin()), XorList<int>::const_iterator>));
	ASSERT_TRUE((is_same_v<decltype(l.cend()), XorList<int>::const_iterator>));
	ASSERT_TRUE((is_same_v<decltype(l.rbegin()), XorList<int>::reverse_iterator>));
	ASSERT_TRUE((is_same_v<decltype(l.rend()), XorList<int>::reverse_iterator>));
	ASSERT_TRUE((is_same_v<decltype(l.crbegin()), XorList<int>::const_reverse_iterator>));
	ASSERT_TRUE((is_same_v<decltype(l.crend()), XorList<int>::const_reverse_iterator>));
	ASSERT_TRUE((is_same_v<decltype(k.begin()), XorList<int>::const_iterator>));
	ASSERT_TRUE((is_same_v<decltype(k.end()), XorList<int>::const_iterator>));
	ASSERT_TRUE((is_same_v<decltype(k.cbegin()), XorList<int>::const_iterator>));
	ASSERT_TRUE((is_same_v<decltype(k.cend()), XorList<int>::const_iterator>));
	ASSERT_TRUE((is_same_v<decltype(k.rbegin()), XorList<int>::const_reverse_iterator>));
	ASSERT_TRUE((is_same_v<decltype(k.rend()), XorList<int>::const_reverse_iterator>));
	ASSERT_TRUE((is_same_v<decltype(k.crbegin()), XorList<int>::const_reverse_iterator>));
	ASSERT_TRUE((is_same_v<decltype(k.crend()), XorList<int>::const_reverse_iterator>));
	ASSERT_TRUE((is_same_v<decltype(l.front()), int&>));
	ASSERT_TRUE((is_same_v<decltype(l.back()), int&>));
	ASSERT_TRUE((is_same_v<decltype(k.front()), const int&>));
	ASSERT_TRUE((is_same_v<decltype(k.back()), const int&>));
}

TEST(XorList, OperationalCorrectness) {
	using std::next, std::equal, std::uniform_int_distribution;
	std::mt19937 gen((std::random_device()()));
	uniform_int_distribution<> op(0, 4);
	uniform_int_distribution<> value;
	const size_t test_length = uniform_int_distribution<>(1e4, 1e4 * 2)(gen);

	XorList<int, StackAllocator<int>> l;
	std::list<int> k;

	for (int i = 0; i < test_length; ++i) switch (op(gen)) {
		case 0: {
			const int val = value(gen);
			const int d = l.size() ? value(gen) % l.size() : 0;
			l.insert(next(l.begin(), d), val);
			k.insert(next(k.begin(), d), val);
			break;
		}
		case 1: {
			const int val = value(gen);
			l.push_back(val);
			k.push_back(val);
			break;
		}
		case 2: {
			if (l.size()) { l.pop_back(); k.pop_back(); }
			break;
		}
		case 3: {
			const int val = value(gen);
			l.push_front(val);
			k.push_front(val);
			break;
		}
		case 4: {
			if (l.size()) { l.pop_front(); k.pop_front(); }
			break;
		}
	}

	ASSERT_EQ(l, k);
	ASSERT_TRUE(equal(l.rbegin(), l.rend(), k.rbegin(), k.rend()));
	ASSERT_TRUE(equal(l.cbegin(), l.cend(), k.cbegin(), k.cend()));
	ASSERT_TRUE(equal(l.crbegin(), l.crend(), k.crbegin(), k.crend()));
}


template<class T, class = void>
constexpr bool has_preinc = false;
template<class T>
constexpr bool has_preinc<T, std::void_t<decltype(std::declval<T>().operator++())>> = true;

template<class T, class = void>
constexpr bool has_postinc = false;
template<class T>
constexpr bool has_postinc<T, std::void_t<decltype(std::declval<T>().operator++(0))>> = true;

template<class T, class = void>
constexpr bool has_predec = false;
template<class T>
constexpr bool has_predec<T, std::void_t<decltype(std::declval<T>().operator--())>> = true;

template<class T, class = void>
constexpr bool has_postdec = false;
template<class T>
constexpr bool has_postdec<T, std::void_t<decltype(std::declval<T>().operator--(0))>> = true;

TEST(Iterators, Iteration) {
	using list_t = XorList<int>;
	ASSERT_TRUE(has_preinc<list_t::iterator>);
	ASSERT_TRUE(has_postinc<list_t::iterator>);
	ASSERT_TRUE(has_predec<list_t::iterator>);
	ASSERT_TRUE(has_postdec<list_t::iterator>);
	ASSERT_TRUE(has_preinc<list_t::const_iterator>);
	ASSERT_TRUE(has_postinc<list_t::const_iterator>);
	ASSERT_TRUE(has_predec<list_t::const_iterator>);
	ASSERT_TRUE(has_postdec<list_t::const_iterator>);
	ASSERT_FALSE(has_preinc<const list_t::iterator>);
	ASSERT_FALSE(has_postinc<const list_t::iterator>);
	ASSERT_FALSE(has_predec<const list_t::iterator>);
	ASSERT_FALSE(has_postdec<const list_t::iterator>);
}

TEST(Iterators, Access) {
	using list_t = XorList<int>;
	using std::is_const_v, std::declval, std::remove_reference_t, std::remove_pointer_t;
	ASSERT_FALSE(is_const_v<remove_reference_t<decltype(declval<list_t::iterator>().operator*())>>);
	ASSERT_FALSE(is_const_v<remove_reference_t<decltype(declval<list_t::iterator>().operator->())>>);
	ASSERT_TRUE(is_const_v<remove_reference_t<decltype(declval<list_t::const_iterator>().operator*())>>);
	ASSERT_TRUE(is_const_v<remove_pointer_t<decltype(declval<list_t::const_iterator>().operator->())>>);
	ASSERT_FALSE(is_const_v<remove_reference_t<decltype(declval<const list_t::iterator>().operator*())>>);
	ASSERT_FALSE(is_const_v<remove_reference_t<decltype(declval<const list_t::iterator>().operator->())>>);
	ASSERT_TRUE(is_const_v<remove_reference_t<decltype(declval<const list_t::const_iterator>().operator*())>>);
	ASSERT_TRUE(is_const_v<remove_pointer_t<decltype(declval<const list_t::const_iterator>().operator->())>>);

	ASSERT_FALSE(is_const_v<remove_pointer_t<decltype(declval<list_t::iterator>().get_node())>>);
	ASSERT_FALSE(is_const_v<remove_pointer_t<decltype(declval<list_t::iterator>().get_prev_node())>>);
	ASSERT_TRUE(is_const_v<remove_pointer_t<decltype(declval<list_t::const_iterator>().get_node())>>);
	ASSERT_TRUE(is_const_v<remove_pointer_t<decltype(declval<list_t::const_iterator>().get_prev_node())>>);
	ASSERT_FALSE(is_const_v<remove_pointer_t<decltype(declval<const list_t::iterator>().get_node())>>);
	ASSERT_FALSE(is_const_v<remove_pointer_t<decltype(declval<const list_t::iterator>().get_prev_node())>>);
	ASSERT_TRUE(is_const_v<remove_pointer_t<decltype(declval<const list_t::const_iterator>().get_node())>>);
	ASSERT_TRUE(is_const_v<remove_pointer_t<decltype(declval<const list_t::const_iterator>().get_prev_node())>>);
}

TEST(Iterators, Conversion) {
	using list_t = XorList<int>;
	using std::is_constructible_v, std::is_assignable_v;
	ASSERT_TRUE((is_constructible_v<list_t::const_iterator, list_t::iterator>));
	ASSERT_TRUE((is_assignable_v<list_t::const_iterator, list_t::iterator>));
	ASSERT_FALSE((is_constructible_v<list_t::iterator, list_t::const_iterator>));
	ASSERT_FALSE((is_assignable_v<list_t::iterator, list_t::const_iterator>));
	ASSERT_TRUE((is_constructible_v<list_t::const_iterator, list_t::const_iterator>));
	ASSERT_TRUE((is_assignable_v<list_t::const_iterator, list_t::const_iterator>));
	ASSERT_TRUE((is_constructible_v<list_t::iterator, list_t::iterator>));
	ASSERT_TRUE((is_assignable_v<list_t::iterator, list_t::iterator>));
}
