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

#ifndef __ministl_vector_h__
#define __ministl_vector_h__

#include <ministl/ministl.h>
#ifndef __GNUG__
#include <ministl/bool.h>
#endif
#include <ministl/defalloc.h>


template<class T>
class vector {
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

    // overlapping move to the right
    static void copy_forward (T* d, const T* sstart, const T* send)
    {
	d += send - sstart;
	while (send != sstart)
	    *--d = *--send;
    }
    // overlapping move to the left
    static void copy_backward (T* d, const T* sstart, const T* send)
    {
	for ( ; send != sstart; ++d, ++sstart)
	    *d = *sstart;
    }

    static void construct (T *d, const T &t)
    {
	new (d) T(t);
    }

    static void construct (T *d, const T *sstart, const T *send)
    {
	for ( ; sstart != send; ++sstart, ++d)
	    construct (d, *sstart);
    }

    static void fill (iterator d, size_type n, const T &t)
    {
	for (size_type i = 0; i < n; ++i, ++d)
	    construct (d, t);
    }

    void reserve (iterator where, size_type n)
    {
	if (_last + n <= _size) {
	    if (where+n < end()) {
		construct (end(), end()-n, end());
		copy_forward (where+n, where, end()-n);
		destroy (where, where+n);
	    } else {
		construct (where+n, where, end());
		destroy (where, end());
	    }
	} else {
	    long sz = _last+n;
	    sz = (_size == 0) ? _max_(sz, 5) : _max_(sz, 2*_size);
	    T *nbuf = alloc (sz);
	    if (_buf) {
	        construct (nbuf, begin(), where);
		construct (nbuf + (where-begin()) + n, where, end());
		destroy (begin(), end());
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
		construct (nbuf, begin(), end());
		destroy (begin(), end());
		dealloc (_buf);
	    }
	    _buf = nbuf;
	    _size = sz;
	}
    }
    vector ()
	: _last (0), _size (0), _buf (0)
    {
    }
    vector (size_type n, const T& t = T())
	: _last (0), _size (0), _buf (0)
    {
	insert (begin(), n, t);
    }
    vector (const_iterator first, const_iterator last)
	: _last (0), _size (0), _buf (0)
    {
	insert (begin(), first, last);
    }
    vector (const vector<T> &v)
	: _last (0), _size (0), _buf (0)
    {
	reserve (v._last);
	construct (begin(), v.begin(), v.end());
	_last = v._last;
    }
    vector<T> &operator= (const vector<T> &v)
    {
	if (this != &v) {
	    destroy (begin(), end());
	    _last = 0;
	    reserve (v._last);
	    construct (begin(), v.begin(), v.end());
	    _last = v._last;
	}
        return *this;
    }
    ~vector ()
    {
	destroy (begin(), end());
	dealloc (_buf);
    }
    const T &front () const
    {
	ministl_assert (size() > 0);
	return _buf[0];
    }
    T &front ()
    {
	ministl_assert (size() > 0);
	return _buf[0];
    }
    const T &back () const
    {
	ministl_assert (size() > 0);
	return _buf[_last-1];
    }
    T &back ()
    {
	ministl_assert (size() > 0);
	return _buf[_last-1];
    }
    bool empty () const
    {
	return _last == 0;
    }
    void clear ()
    {
	destroy (begin(), end());
	_last = 0;
    }
    void push_back (const T &t)
    {
	reserve (_last+1);
	construct (end(), t);
	++_last;
    }
    void pop_back ()
    {
	ministl_assert (size() > 0);
	--_last;
	destroy (end());
    }
    const T &operator[] (size_type idx) const
    {
	ministl_assert (idx < size());
	return _buf[idx];
    }
    T &operator[] (size_type idx)
    {
	ministl_assert (idx < size());
	return _buf[idx];
    }
    iterator insert (iterator pos, const T &t)
    {
	ministl_assert (pos <= end());
	long at = pos - begin();
	reserve (pos, 1);
	pos = begin()+at;
	construct (pos, t);
	++_last;
	return pos;
    }
    iterator insert (iterator pos, const_iterator first, const_iterator last)
    {
        ministl_assert (pos <= end());
	long n = last - first;
	long at = pos - begin();
	if (n > 0) {
	    reserve (pos, n);
	    pos = begin()+at;
	    construct (pos, first, last);
	    _last += n;
	}
	return pos;
    }
    iterator insert (iterator pos, size_type n, const T &t)
    {
        ministl_assert (pos <= end());
	long at = pos - begin();
	if (n > 0) {
	    reserve (pos, n);
	    pos = begin()+at;
	    fill (pos, n, t);
	    _last += n;
	}
	return pos;
    }
    void erase (iterator first, iterator last)
    {
	if (last != first) {
	    copy_backward (first, last, end());
	    destroy (end() - (last-first), end());
	    _last -= last - first;
	}
    }
    void erase (iterator pos)
    {
        if (pos != end()) {
            copy_backward (pos, pos+1, end());
            destroy (end()-1);
            --_last;
        }
    }
};

template<class T>
bool operator== (const vector<T> &v1, const vector<T> &v2)
{
    if (v1.size() != v2.size())
	return false;
    for (unsigned long i = 0; i < v1.size(); ++i) {
	if (!(v1[i] == v2[i]))
	    return false;
    }
    return true;
}

template<class T>
bool operator< (const vector<T> &v1, const vector<T> &v2)
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
