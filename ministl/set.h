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

#ifndef __ministl_set_h__
#define __ministl_set_h__

#include <ministl/ministl.h>
#include <ministl/simplevec.h>
#include <ministl/function>
#include <ministl/pair>
#ifndef __GNUG__
#include <ministl/bool.h>
#endif

template<class vT, class cT> class set;
template<class vT, class cT> class setConstIterator;

template<class vT, class cT>
class setIterator {
    friend class set<vT,cT>;
    friend class setConstIterator<vT,cT>;
    typedef setIterator<vT,cT> my_type;
    typedef vT value_type;
    typedef simplevec<value_type *> rep_type;
    typedef typename rep_type::iterator repiterator;
    repiterator n;

    setIterator (repiterator _n)
	: n (_n)
    {
    }
public:
    setIterator ()
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

template<class vT, class cT>
class setConstIterator {
    friend class set<vT,cT>;
    typedef setConstIterator<vT,cT> my_type;
    typedef vT value_type;
    typedef simplevec<value_type *> rep_type;
    typedef typename rep_type::const_iterator repiterator;
    repiterator n;

    setConstIterator (repiterator _n)
	: n (_n)
    {
    }
public:
    setConstIterator ()
	: n (0)
    {
    }
    setConstIterator (const setIterator<vT,cT> &i)
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

template<class valT, class cmpT>
class set {
public:
    typedef valT value_type;
    typedef unsigned long size_type;
    typedef simplevec<value_type *> rep_type;
    typedef setIterator<valT, cmpT> iterator;
    typedef setConstIterator<valT, cmpT> const_iterator;
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
    set (const cmpT &comp = cmpT())
	: _comp (comp)
    {
    }
    set (const_iterator first, const_iterator last, const cmpT &comp = cmpT())
	: _comp (comp)
    {
	insert (first, last);
    }
    set (const set<valT, cmpT> &m)
        : _comp (m._comp)
    {
        insert (m.begin(), m.end());
    }
    set<valT, cmpT> &operator= (const set<valT, cmpT> &m)
    {
	if (this != &m) {
	    _comp = m._comp;
	    erase (begin(), end());
	    insert (m.begin(), m.end());
	}
	return *this;
    }
    ~set ()
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
    bool lookup (const value_type &k, iterator &it);
public:
    pair_iterator_bool insert (const value_type &v)
    {
	iterator i = end();
        if (size() > 0 && lookup (v, i))
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
    size_type erase (const value_type &k)
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
    iterator find (const value_type &k)
    {
	if (size() > 0) {
	    int l = 0;
	    int r = size()-1;
	    do {
		int m = (l+r) >> 1;
		if (_comp (*_ents[m], k)) {
		    l = m+1;
		} else {
		    // if (k == *_ents[m])
		    if (!_comp (k, *_ents[m]))
			return iterator (_ents.begin()+m);
		    r = m-1;
		}
	    } while (l <= r);
	}
	return end();
    }
    const_iterator find (const value_type &k) const
    {
	if (size() > 0) {
	    int l = 0;
	    int r = size()-1;
	    do {
		int m = (l+r) >> 1;
		if (_comp (*_ents[m], k)) {
		    l = m+1;
		} else {
		    // if (k == *_ents[m])
		    if (!_comp (k, *_ents[m]))
			return const_iterator (_ents.begin()+m);
		    r = m-1;
		}
	    } while (l <= r);
	}
	return end();
    }
    size_type count (const value_type &k) const
    {
	return find (k) != end() ? 1 : 0;
    }
};

template<class vT, class cT>
inline bool
set<vT, cT>::lookup (const vT &k, setIterator<vT,cT> &it)
{
    int l = 0;
    int r = size();
    while (l < r) {
	int m = (l+r) >> 1;
	ministl_assert (m < r);
	if (_comp (*_ents[m], k)) {
	    l = m+1;
	} else {
	    // if (k == *_ents[m]) {
	    if (!_comp (k, *_ents[m])) {
		it = setIterator<vT,cT> (_ents.begin()+m);
		return true;
	    }
	    r = m;
	}
    }
    ministl_assert (l == r);
    it = setIterator<vT,cT> (_ents.begin()+l);
    return l < (int)size() &&
        // k == *it
        !_comp (*it, k) && !_comp (k, *it);
}

template<class vT, class cT>
bool operator== (const set<vT,cT> &v1, const set<vT,cT> &v2)
{
    if (v1.size() != v2.size())
	return false;
    typename set<vT,cT>::const_iterator i1 = v1.begin();
    typename set<vT,cT>::const_iterator i2 = v2.begin();
    for ( ;i1 != v1.end() && i2 != v2.end(); ++i1, ++i2) {
	if (!(*i1 == *i2))
	    return false;
    }
    return true;
}

template<class vT, class cT>
bool operator< (const set<vT,cT> &v1, const set<vT,cT> &v2)
{
    long minlast = _min_ (v1.size(), v2.size());
    typename set<vT,cT>::const_iterator i1 = v1.begin();
    typename set<vT,cT>::const_iterator i2 = v2.begin();
    for ( ;i1 != v1.end() && i2 != v2.end(); ++i1, ++i2) {
        if (*i1 < *i2)
	    return true;
	if (*i2 < *i1)
	    return false;
    }
    return v1.size() < v2.size();
}

#endif // __ministl_set_h__
