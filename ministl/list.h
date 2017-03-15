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

#ifndef __ministl_list_h__
#define __ministl_list_h__

#include <ministl/ministl.h>
#ifndef __GNUG__
#include <ministl/bool.h>
#endif

template<class T>
class listNode {
    listNode<T> *_prev, *_next;
    T _data;
public:
    listNode (const T &data = T(), listNode<T> *next = 0,
	      listNode<T> *prev = 0)
	: _prev (prev), _next (next), _data (data)
    {
    }
    ~listNode ()
    {
	ministl_assert (!_next && !_prev);
    }
    void remove ()
    {
	if (_prev)
	    _prev->_next = _next;
	if (_next)
	    _next->_prev = _prev;
	_next = _prev = 0;
    }
    void insert_after (listNode<T> *ln)
    {
	ministl_assert (ln);
	_next = _prev = 0;
	if (ln->_next)
	    ln->_next->_prev = this;
	_next = ln->_next;
	ln->_next = this;
	_prev = ln;
    }
    void insert_before (listNode<T> *ln)
    {
	ministl_assert (ln);
	_next = _prev = 0;
	if (ln->_prev)
	    ln->_prev->_next = this;
	_prev = ln->_prev;
	ln->_prev = this;
	_next = ln;
    }
    const T &data () const
    {
	return _data;
    }
    T &data ()
    {
	return _data;
    }
    listNode<T> *next ()
    {
	return _next;
    }
    listNode<T> *prev ()
    {
	return _prev;
    }
};

template<class T> class list;
template<class T> class listConstIterator;

template<class T>
class listIterator {
    friend class list<T>;
    friend class listConstIterator<T>;
    typedef listNode<T> node;
    node *n;
    listIterator (node *_n)
	: n (_n)
    {
    }
public:
    listIterator ()
	: n (0)
    {
    }
    bool operator== (const listIterator<T> &it) const
    {
	return it.n == n;
    }
    bool operator!= (const listIterator<T> &it) const
    {
	return !(it.n == n);
    }
    listIterator<T> operator++ ()
    {
	n = n->next();
	ministl_assert (n);
	return *this;
    }
    listIterator<T> operator++ (int)
    {
	listIterator<T> tmp = *this;
	n = n->next();
	ministl_assert (n);
	return tmp;
    }
    listIterator<T> operator-- ()
    {
	n = n->prev();
	ministl_assert (n);
	return *this;
    }
    listIterator<T> operator-- (int)
    {
	listIterator<T> tmp = *this;
	n = n->prev();
	ministl_assert (n);
	return tmp;
    }
    T &operator* ()
    {
	return n->data();
    }
};

template<class T>
class listConstIterator {
    friend class list<T>;
    typedef listNode<T> node;
    node *n;
    listConstIterator (node *_n)
	: n (_n)
    {
    }
public:
    listConstIterator ()
	: n (0)
    {
    }
    listConstIterator (const listIterator<T> &i)
	: n (i.n)
    {
    }
    bool operator== (const listConstIterator<T> &it) const
    {
	return it.n == n;
    }
    bool operator!= (const listConstIterator<T> &it) const
    {
	return !(it.n == n);
    }
    listConstIterator<T> operator++ ()
    {
	n = n->next();
	ministl_assert (n);
	return *this;
    }
    listConstIterator<T> operator++ (int)
    {
	listConstIterator<T> tmp = *this;
	n = n->next();
	ministl_assert (n);
	return tmp;
    }
    listConstIterator<T> operator-- ()
    {
	n = n->prev();
	ministl_assert (n);
	return *this;
    }
    listConstIterator<T> operator-- (int)
    {
	listConstIterator<T> tmp = *this;
	n = n->prev();
	ministl_assert (n);
	return tmp;
    }
    const T &operator* () const
    {
	return n->data();
    }
};

template<class T>
class list {
    typedef listNode<T> node;
public:
    typedef unsigned long size_type;
    typedef listIterator<T> iterator;
    typedef listConstIterator<T> const_iterator;
private:
    node *_begin;
    node *_end;
    size_type _length;
public:
#if 0
    void __check ()
    {
	node *n = _begin;
        while (n->next())
	    n = n->next();
	assert (n == _end);
    }
#endif
    iterator begin ()
    {
	return iterator (_begin);
    }
    const_iterator begin () const
    {
	return const_iterator (_begin);
    }
    iterator end ()
    {
	return iterator (_end);
    }
    const_iterator end () const
    {
	return const_iterator (_end);
    }
    list ()
	: _length (0)
    {
	_begin = _end = new node ();
    }
    list (size_type n, const T &t = T())
	: _length (0)
    {
	_begin = _end = new node ();
	insert (begin(), n, t);
    }
    list (const T *first, const T *last)
	: _length (0)
    {
	_begin = _end = new node ();
	insert (begin(), first, last);
    }
    list (const_iterator first, const_iterator last)
	: _length (0)
    {
	_begin = _end = new node ();
	insert (begin(), first, last);
    }
    
    /*
	typedef list<T> list_T;
    list (const list_T &list)
	: _length (0)
    {
	_begin = _end = new node ();
	insert (begin(), list.begin(), list.end());
    }
    list<T> &operator= (const list<T> &list)
    {
	if (this != &list) {
	    erase (begin(), end());
	    insert (begin(), list.begin(), list.end());
	}
  
	return *this;
    }
    */
    ~list ()
    {
	erase (begin(), end());
	delete _end;
    }
    T &front ()
    {
	return _begin->data();
    }
    const T &front () const
    {
	return _begin->data();
    }
    T &back ()
    {
	ministl_assert (_end->prev());
	return _end->prev()->data();
    }
    const T &back () const
    {
	ministl_assert (_end->prev());
	return _end->prev()->data();
    }
    bool empty () const
    {
	return _length == 0;
    }
    void clear ()
    {
	erase (begin(), end());
    }
    size_type size () const
    {
	return _length;
    }
    void push_front (const T &t)
    {
	insert (begin(), t);
    }
    void pop_front ()
    {
	ministl_assert (size() > 0);
	erase (begin());
    }
    void push_back (const T &t)
    {
	insert (end(), t);
    }
    void pop_back ()
    {
	ministl_assert (size() > 0);
	erase (--end());
    }
    iterator insert (iterator pos, const T &t)
    {
	node *n = new node (t);
	n->insert_before (pos.n);
	if (pos.n == _begin)
	    _begin = n;
	++_length;
	return iterator (n);
    }
    void insert (iterator pos, size_type n, const T &t)
    {
	for (size_type i = 0; i < n; ++i)
	    insert (pos, t);
    }
    void insert (iterator pos, const T *first, const T *last)
    {
	for ( ; first != last; ++first)
	    insert (pos, *first);
    }
    void insert (iterator pos, const_iterator first, const_iterator last)
    {
	for ( ; first != last; ++first)
	    insert (pos, *first);
    }
    void erase (iterator pos)
    {
        if (pos != end()) {
            ministl_assert (pos.n != _end);
            if (pos.n == _begin)
                _begin = _begin->next();
            pos.n->remove ();
            delete pos.n;
            pos.n = 0;
            --_length;
        }
    }
    void erase (iterator first, iterator last)
    {
	iterator next;
	while (first != last) {
	    next = first;
	    ++next;
	    // XXX first must be incremented before erasing!
	    erase (first);
	    first = next;
	}
    }
};

template<class T>
bool operator== (const list<T> &v1, const list<T> &v2)
{
    if (v1.size() != v2.size())
	return false;
    typename list<T>::const_iterator i1 = v1.begin();
    typename list<T>::const_iterator i2 = v2.begin();
    for ( ;i1 != v1.end() && i2 != v2.end(); ++i1, ++i2) {
	if (!(*i1 == *i2))
	    return false;
    }
    return true;
}

template<class T>
bool operator< (const list<T> &v1, const list<T> &v2)
{
    long minlast = _min_ (v1.size(), v2.size());
    typename list<T>::const_iterator i1 = v1.begin();
    typename list<T>::const_iterator i2 = v2.begin();
    for ( ;i1 != v1.end() && i2 != v2.end(); ++i1, ++i2) {
	if (!(*i1 == *i2))
	    return *i1 < *i2;
    }
    return v1.size() < v2.size();
}

#endif // __ministl_list_h__
