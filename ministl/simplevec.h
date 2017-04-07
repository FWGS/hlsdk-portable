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

#ifndef __ministl_simplevec_h__
#define __ministl_simplevec_h__

#include <ministl/ministl.h>
#ifndef __GNUG__
#include <ministl/bool.h>
#endif
#include <ministl/defalloc.h>


template<class T>
class simplevec {
public:
    typedef T* iterator;
    typedef const T* const_iterator;
    typedef unsigned long size_type;
private:
    size_type _last, _size;
    T *_buf;

public:
    const_iterator begin () const
    {
	return &_buf[0];
    }
    iterator begin ()
    {
	return &_buf[0];
    }
    const_iterator end () const
    {
	return &_buf[_last];
    }
    iterator end ()
    {
	return &_buf[_last];
    }
    size_type capacity () const
    {
	return _size;
    }
    size_type size () const
    {
	return _last;
    }

private:
    static T *alloc (size_type n)
    {
	return (T *)::operator new ((size_t)(n * sizeof (T)));
    }
    static void dealloc (T *buf)
    {
	if (buf)
	    ::operator delete (buf);
    }

    void reserve (iterator where, size_type n)
    {
	if (_last + n <= _size) {
	    memmove (where+n, where, (end()-where)*sizeof(T));
	} else {
	    long sz = _last+n;
	    sz = (_size == 0) ? _max_(sz, 5) : _max_(sz, 2*_size);
	    T *nbuf = alloc (sz);
	    if (_buf) {
		memcpy (nbuf, begin(), (where-begin())*sizeof(T));
		memcpy (nbuf + (where-begin()) + n, where,
			(end()-where)*sizeof(T));
		dealloc (_buf);
	    }
	    _buf = nbuf;
	    _size = sz;
	}
    }
public:
    void reserve (size_type sz)
    {
	if (_size < sz) {
	    sz = (_size == 0) ? _max_(sz, 5) : _max_(sz, 2*_size);
	    T *nbuf = alloc (sz);
	    if (_buf) {
		memcpy (nbuf, begin(), size()*sizeof(T));
		dealloc (_buf);
	    }
	    _buf = nbuf;
	    _size = sz;
	}
    }
    simplevec ()
	: _last (0), _size (0), _buf (0)
    {
    }
    simplevec (size_type n, const T& t = T())
	: _last (0), _size (0), _buf (0)
    {
	insert (begin(), n, t);
    }
    simplevec (const_iterator first, const_iterator last)
	: _last (0), _size (0), _buf (0)
    {
	insert (begin(), first, last);
    }
    simplevec (const simplevec<T> &v)
	: _last (0), _size (0), _buf (0)
    {
	reserve (v._last);
	memcpy (_buf, v.begin(), v.size()*sizeof(T));
	_last = v._last;
    }
    simplevec<T> &operator= (const simplevec<T> &v)
    {
	if (this != &v) {
	    _last = 0;
	    reserve (v._last);
	    memcpy (_buf, v.begin(), v.size()*sizeof(T));
	    _last = v._last;
	}
        return *this;
    }
    ~simplevec ()
    {
	dealloc (_buf);
    }
    const T &front () const
    {
	//ministl_assert (size() > 0);
	return _buf[0];
    }
    T &front ()
    {
	//ministl_assert (size() > 0);
	return _buf[0];
    }
    const T &back () const
    {
	//ministl_assert (size() > 0);
	return _buf[_last-1];
    }
    T &back ()
    {
	//ministl_assert (size() > 0);
	return _buf[_last-1];
    }
    bool empty () const
    {
	return _last == 0;
    }
    void clear ()
    {
	_last = 0;
    }
    void push_back (const T &t)
    {
	reserve (_last+1);
	*end() = t;
	++_last;
    }
    void pop_back ()
    {
	//ministl_assert (size() > 0);
	--_last;
    }
    const T &operator[] (size_type idx) const
    {
	//ministl_assert (idx < size());
	return _buf[idx];
    }
    T &operator[] (size_type idx)
    {
	//ministl_assert (idx < size());
	return _buf[idx];
    }
    iterator insert (iterator pos, const T &t)
    {
	//ministl_assert (pos <= end());
	long at = pos - begin();
	reserve (pos, 1);
	pos = begin()+at;
	*pos = t;
	++_last;
	return pos;
    }
    iterator insert (iterator pos, const_iterator first, const_iterator last)
    {
        //ministl_assert (pos <= end());
	long n = last - first;
	long at = pos - begin();
	if (n > 0) {
	    reserve (pos, n);
	    pos = begin()+at;
	    memcpy (pos, first, (last-first)*sizeof(T));
	    _last += n;
	}
	return pos;
    }
    iterator insert (iterator pos, size_type n, const T &t)
    {
        //ministl_assert (pos <= end());
	long at = pos - begin();
	if (n > 0) {
	    reserve (pos, n);
	    pos = begin()+at;
	    for (int i = 0; i < n; ++i)
		pos[i] = t;
	    _last += n;
	}
	return pos;
    }
    void erase (iterator first, iterator last)
    {
	if (last != first) {
	    memmove (first, last, (end()-last)*sizeof(T));
	    _last -= last - first;
	}
    }
    void erase (iterator pos)
    {
        if (pos != end()) {
	    memmove (pos, pos+1, (end()-(pos+1))*sizeof(T));
            --_last;
        }
    }
};

template<class T>
bool operator== (const simplevec<T> &v1, const simplevec<T> &v2)
{
    if (v1.size() != v2.size())
	return false;
    return !v1.size() || !memcmp (&v1[0], &v2[0], v1.size()*sizeof(T));
}

template<class T>
bool operator< (const simplevec<T> &v1, const simplevec<T> &v2)
{
    unsigned long minlast = _min_ (v1.size(), v2.size());
    for (unsigned long i = 0; i < minlast; ++i) {
        if (v1[i] < v2[i])
	    return true;
	if (v2[i] < v1[i])
	    return false;
    }
    return v1.size() < v2.size();
}

#endif
