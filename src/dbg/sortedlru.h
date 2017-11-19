#ifndef _SORTEDLRU_H_
#define _SORTEDLRU_H_

#include <list>
#include <map>
#include <unordered_map>
#include <iterator>

template<typename _K, typename _V, typename _Pr = std::less<_K>, size_t _Max = 100000>
class SortedLRU
{
private:
	struct EntryElem_t;

	typedef std::map<_K, EntryElem_t, _Pr> Map_t;
	typedef std::list<typename Map_t::iterator> List_t;

	struct EntryElem_t
	{
		_V value;
		typename List_t::iterator it;
	};

	List_t _list;
	Map_t _map;

public:
	class iterator
	{
	public:
		typedef std::pair<_K, _V> val_type;
		typedef std::pair<_K, _V>& ref_type;
		typedef std::pair<_K, _V>* ptr_type;
		typedef std::forward_iterator_tag iterator_category;
		typedef size_t difference_type;

		iterator(typename Map_t::iterator it) : _it(it) {}
		iterator operator++()
		{
			auto temp = *this;
			std::advance(_it, 1);
			return temp;
		}
		iterator operator++(int)
		{
			std::advance(_it, 1);
			return *this;
		}
		ref_type operator*()
		{
			return reinterpret_cast<ref_type>(*_it);
		}

		ptr_type operator->()
		{
			return reinterpret_cast<ref_type>(&(*_it));
		}
		bool operator==(const iterator& rhs) const { return _it == rhs._it; }
		bool operator!=(const iterator& rhs) const { return _it != rhs._it; }

		typename Map_t::iterator internal_iter() const { return _it; }

	private:
		typename Map_t::iterator _it;
	};

public:
	void insert(const _K& key, const _V& val)
	{
		// Do we require room?
		if (_map.size() + 1 >= _Max)
		{
			auto itBack = std::prev(_list.end(), 1);

			// Erase map element.
			_map.erase(*itBack);

			// Erase list element.
			_list.erase(itBack);
		}

		_list.push_front(_map.end());

		EntryElem_t elem;
		elem.value = val;
		elem.it = _list.begin();

		auto itNew = _map.insert(std::make_pair(key, elem));
		_list.front() = itNew.first;
	}

	iterator find(const _K& key)
	{
		auto itr = _map.find(key);
		if (itr == _map.end())
		{
			return end();
		}

		const EntryElem_t& elem = itr->second;

		return iterator(itr);
	}

	iterator end()
	{
		return iterator(_map.end());
	}

	bool acquire(const iterator& it)
	{
		if (it == end())
			return false;

		const EntryElem_t& elem = it.internal_iter()->second;

		// Move to start.
		_list.splice(_list.begin(), _list, elem.it);

		return true;
	}
};

#endif _SORTEDLRU_H_
