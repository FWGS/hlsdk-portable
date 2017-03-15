// -*- c++ -*-
/*
 *  MICO --- a free CORBA implementation
 *  Copyright (C) 1997-98 Kay Roemer & Arno Puder
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Send comments and/or bug reports to:
 *                 mico@informatik.uni-frankfurt.de
 */

#ifndef __ministl_map_h__
#define __ministl_map_h__

#include <ministl/ministl.h>
#include <ministl/simplevec.h>
#include <ministl/function>
#include <ministl/pair>
#ifndef __GNUG__
#include <ministl/bool.h>
#endif

template<class kT, class vT, class cT> class map;
template<class kT, class vT, class cT> class mapConstIterator;

template<class kT, class vT, class cT>
class mapIterator {
    friend class map<kT,vT,cT>;
    friend class mapConstIterator<kT,vT,cT>;
    typedef mapIterator<kT,vT,cT> my_type;
    typedef pair<const kT, vT> value_type;
    typedef simplevec<value_type *> rep_type;
    typedef typename rep_type::iterator repiterator;
    repiterator n;

    mapIterator (repiterator _n)
	: n (_n)
    {
    }
public:
    mapIterator ()
	: n (0)
    {
    }
    bool operator== (const my_type &it) const
    {
	return it.n == n;
    }
    bool operator!= (const my_type &it) const
    {
	return !(it.n == n);
    }
    my_type operator++ ()
    {
        ++n;
	return *this;
    }
    my_type operator++ (int)
    {
        my_type tmp = *this;
        ++n;
	return tmp;
    }
    my_type operator-- ()
    {
        --n;
	return *this;
    }
    my_type operator-- (int)
    {
        my_type tmp = *this;
        --n;
	return tmp;
    }
    value_type &operator* ()
    {
	return **n;
    }
};

template<class kT, class vT, class cT>
class mapConstIterator {
    friend class map<kT,vT,cT>;
    typedef mapConstIterator<kT,vT,cT> my_type;
    typedef pair<const kT, vT> value_type;
    typedef simplevec<value_type *> rep_type;
    typedef typename rep_type::const_iterator repiterator;
    repiterator n;

    mapConstIterator (repiterator _n)
	: n (_n)
    {
    }
public:
    mapConstIterator ()
	: n (0)
    {
    }
    mapConstIterator (const mapIterator<kT,vT,cT> &i)
	: n (i.n)
    {
    }
    bool operator== (const my_type &it) const
    {
	return it.n == n;
    }
    bool operator!= (const my_type &it) const
    {
	return !(it.n == n);
    }
    my_type operator++ ()
    {
        ++n;
	return *this;
    }
    my_type operator++ (int)
    {
        my_type tmp = *this;
        ++n;
	return tmp;
    }
    my_type operator-- ()
    {
        --n;
	return *this;
    }
    my_type operator-- (int)
    {
        my_type tmp = *this;
        --n;
	return tmp;
    }
    const value_type &operator* () const
    {
	return **n;
    }
};

template<class keyT, class valT, class cmpT>
class map {
public:
    typedef keyT key_type;
    typedef pair<const keyT, valT> value_type;
    typedef unsigned long size_type;
    typedef simplevec<value_type *> rep_type;
    typedef mapIterator<keyT, valT, cmpT> iterator;
    typedef mapConstIterator<keyT, valT, cmpT> const_iterator;
    // XXX typedefs done to work around g++ bug
    typedef pair<iterator, bool> pair_iterator_bool;
private:
    rep_type _ents;
    cmpT _comp;
public:
    iterator begin ()
    {
	return iterator (_ents.begin());
    }
    const_iterator begin () const
    {
	return const_iterator (_ents.begin());
    }
    iterator end ()
    {
	return iterator (_ents.end());
    }
    const_iterator end () const
    {
	return const_iterator (_ents.end());
    }
    map (const cmpT &comp = cmpT())
	: _comp (comp)
    {
    }
    map (const_iterator first, const_iterator last, const cmpT &comp = cmpT())
	: _comp (comp)
    {
	insert (first, last);
    }
    map (const map<keyT, valT, cmpT> &m)
        : _comp (m._comp)
    {
        insert (m.begin(), m.end());
    }
    map<keyT, valT, cmpT> &operator= (const map<keyT, valT, cmpT> &m)
    {
	if (this != &m) {
	    _comp = m._comp;
	    erase (begin(), end());
	    insert (m.begin(), m.end());
	}
	return *this;
    }
    ~map ()
    {
        erase (begin(), end());
    }
    bool empty () const
    {
	return _ents.empty ();
    }
    size_type size () const
    {
	return _ents.size ();
    }
private:
    // find the iterator position where k should be inserted ...
    bool lookup (const key_type &k, iterator &it);
public:
    pair_iterator_bool insert (const value_type &v)
    {
	iterator i = end();
        if (size() > 0 && lookup (v.first, i))
            return pair_iterator_bool (i, false);
        i = iterator (_ents.insert (i.n, new value_type (v)));
	return pair_iterator_bool (i, true);
    }
#if 0
    iterator insert (iterator pos, const value_type &v)
    {
    }
#endif
    void insert (const_iterator first, const_iterator last)
    {
	for ( ; first != last; ++first)
	    insert (*first);
    }
    void insert (const value_type *first, const value_type *last)
    {
	for ( ; first != last; ++first)
	    insert (*first);
    }
    void erase (iterator pos)
    {
        if (pos != end()) {
            delete *(pos.n);
            _ents.erase (pos.n);
        }
    }
    size_type erase (const key_type &k)
    {
	iterator i = find (k);
	if (i == end())
	    return 0;
	erase (i);
	return 1;
    }
    void erase (iterator first, iterator last)
    {
        for (iterator i = first; i != last; ++i)
            delete *(i.n);
	_ents.erase (first.n, last.n);
    }
    void clear ()
    {
	erase (begin(), end());
    }
    iterator find (const key_type &k)
    {
	if (size() > 0) {
	    int l = 0;
	    int r = size()-1;
	    do {
		int m = (l+r) >> 1;
		if (_comp (_ents[m]->first, k)) {
		    l = m+1;
		} else {
		    // if (k == _ents[m]->first)
		    if (!_comp (k, _ents[m]->first))
			return iterator (_ents.begin()+m);
		    r = m-1;
		}
	    } while (l <= r);
	}
	return end();
    }
    const_iterator find (const key_type &k) const
    {
	if (size() > 0) {
	    int l = 0;
	    int r = size()-1;
	    do {
		int m = (l+r) >> 1;
		if (_comp (_ents[m]->first, k)) {
		    l = m+1;
		} else {
		    // if (k == _ents[m]->first)
		    if (!_comp (k, _ents[m]->first))
			return const_iterator (_ents.begin()+m);
		    r = m-1;
		}
	    } while (l <= r);
	}
	return end();
    }
    size_type count (const key_type &k) const
    {
	return find (k) != end() ? 1 : 0;
    }
    valT &operator[] (const key_type &k)
    {
	iterator i = insert(value_type (k, valT())).first;
	return (*i).second;
    }
};

template<class kT, class vT, class cT>
inline bool
map<kT,vT,cT>::lookup (const kT &k, mapIterator<kT,vT,cT> &it)
{
    int l = 0;
    int r = size();
    while (l < r) {
	int m = (l+r) >> 1;
	ministl_assert (m < r);
	if (_comp (_ents[m]->first, k)) {
	    l = m+1;
	} else {
	    // if (k == _ents[m]->first) {
	    if (!_comp (k, _ents[m]->first)) {
		it = mapIterator<kT,vT,cT> (_ents.begin()+m);
		return true;
	    }
	    r = m;
	}
    }
    ministl_assert (l == r);
    it = mapIterator<kT,vT,cT> (_ents.begin()+l);
    return l < (int)size() &&
        // (*it).first == k;
        !_comp ((*it).first, k) && !_comp (k, (*it).first);
}

template<class kT, class vT, class cT>
bool operator== (const map<kT,vT,cT> &v1, const map<kT,vT,cT> &v2)
{
    if (v1.size() != v2.size())
	return false;
    typename map<kT,vT,cT>::const_iterator i1 = v1.begin();
    typename map<kT,vT,cT>::const_iterator i2 = v2.begin();
    for ( ;i1 != v1.end() && i2 != v2.end(); ++i1, ++i2) {
	if (!(*i1 == *i2))
	    return false;
    }
    return true;
}

template<class kT, class vT, class cT>
bool operator< (const map<kT,vT,cT> &v1, const map<kT,vT,cT> &v2)
{
    long minlast = _min_ (v1.size(), v2.size());
    typename map<kT,vT,cT>::const_iterator i1 = v1.begin();
    typename map<kT,vT,cT>::const_iterator i2 = v2.begin();
    for ( ;i1 != v1.end() && i2 != v2.end(); ++i1, ++i2) {
        if (*i1 < *i2)
	    return true;
	if (*i2 < *i1)
	    return false;
    }
    return v1.size() < v2.size();
}

#endif // __ministl_map_h__
