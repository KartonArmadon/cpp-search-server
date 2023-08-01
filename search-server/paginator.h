#pragma once
#include <iostream>
#include <vector>

template<typename PageIterator>
class IteratorRange {
public:
	IteratorRange(PageIterator begin, PageIterator end) :_begin(begin), _end(end) {}
	auto begin() { return _begin; }
	auto end() { return _end; }
	auto size() { return distance(_begin, _end); }

private:
	PageIterator _begin;
	PageIterator _end;
};

template<typename PageIterator>
class Paginator {
public:
	Paginator(PageIterator it_begin, PageIterator it_end, size_t page_size = 3) {
		if (page_size < 1)
			page_size = 1;
		auto size = distance(it_begin, it_end);
		if (size == 0)
			return;
		size_t page_count = 1 + (size / 3);
		_pages.reserve(page_count);
		PageIterator it_current = it_begin;
		size_t current_page_size = {};
		size_t distance_to_end = {};
		do
		{
			distance_to_end = distance(it_current, it_end);
			current_page_size = std::min(distance_to_end, page_size);
			_pages.push_back(IteratorRange(it_current, it_current + current_page_size));
			it_current = next(it_current, current_page_size);
		} while (distance(it_current, it_end) > 0);
	}

	auto begin() const { return _pages.begin(); }

	auto end() const { return _pages.end(); }

	size_t size() const { return _pages.size(); }
private:
	std::vector<IteratorRange<PageIterator>> _pages;
};

template<typename PageIterator>
std::ostream& operator<<(std::ostream& output, IteratorRange<PageIterator> item) {
	for (auto it = item.begin(); it != item.end(); ++it) {
		std::cout << *it;
	}
	return output;
}

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
	return Paginator(begin(c), end(c), page_size);
}