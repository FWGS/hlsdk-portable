//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
// A growable memory class.
//===========================================================================//

#ifndef UTLMEMORY_H
#define UTLMEMORY_H

#ifdef _WIN32
#pragma once
#endif

//#include "osconfig.h"
#include <assert.h>
#include <new>
#include <string.h>

#pragma warning (disable:4100)
#pragma warning (disable:4514)

#define Assert(expr)

//-----------------------------------------------------------------------------


#ifdef UTLMEMORY_TRACK
#define UTLMEMORY_TRACK_ALLOC()		MemAlloc_RegisterAllocation( "Sum of all UtlMemory", 0, m_nAllocationCount * sizeof(T), m_nAllocationCount * sizeof(T), 0 )
#define UTLMEMORY_TRACK_FREE()		if ( !m_pMemory ) ; else MemAlloc_RegisterDeallocation( "Sum of all UtlMemory", 0, m_nAllocationCount * sizeof(T), m_nAllocationCount * sizeof(T), 0 )
#else
#define UTLMEMORY_TRACK_ALLOC()		((void)0)
#define UTLMEMORY_TRACK_FREE()		((void)0)
#endif

template <class T>
inline void Construct (T *pMemory)
{
	::new(pMemory) T;
}

template <class T>
inline void CopyConstruct (T *pMemory, T const& src)
{
	::new(pMemory) T (src);
}

template <class T>
inline void Destruct (T *pMemory)
{
	pMemory->~T ();

#ifdef _DEBUG
	memset (pMemory, 0xDD, sizeof (T));
#endif
}


//-----------------------------------------------------------------------------
// The CUtlMemory class:
// A growable memory class which doubles in size by default.
//-----------------------------------------------------------------------------
template< class T, class I = int >
class CUtlMemory
{
public:
	// constructor, destructor
	CUtlMemory (int nGrowSize = 0, int nInitSize = 0);
	CUtlMemory (T* pMemory, int numElements);
	CUtlMemory (const T* pMemory, int numElements);
	~CUtlMemory ();

	// Set the size by which the memory grows
	void Init (int nGrowSize = 0, int nInitSize = 0);

	class Iterator_t
	{
	public:
		Iterator_t (I i) : index (i) {}
		I index;

		bool operator==(const Iterator_t it) const { return index == it.index; }
		bool operator!=(const Iterator_t it) const { return index != it.index; }
	};
	Iterator_t First () const { return Iterator_t (IsIdxValid (0) ? 0 : InvalidIndex ()); }
	Iterator_t Next (const Iterator_t &it) const { return Iterator_t (IsIdxValid (it.index + 1) ? it.index + 1 : InvalidIndex ()); }
	I GetIndex (const Iterator_t &it) const { return it.index; }
	bool IsIdxAfter (I i, const Iterator_t &it) const { return i > it.index; }
	bool IsValidIterator (const Iterator_t &it) const { return IsIdxValid (it.index); }
	Iterator_t InvalidIterator () const { return Iterator_t (InvalidIndex ()); }

	// element access
	T& operator[](I i);
	const T& operator[](I i) const;
	T& Element (I i);
	const T& Element (I i) const;

	// Can we use this index?
	bool IsIdxValid (I i) const;

	// Specify the invalid ('null') index that we'll only return on failure
	static const I INVALID_INDEX = (I)-1; // For use with COMPILE_TIME_ASSERT
	static I InvalidIndex () { return INVALID_INDEX; }

	// Gets the base address (can change when adding elements!)
	T* Base ();
	const T* Base () const;

	// Attaches the buffer to external memory....
	void SetExternalBuffer (T* pMemory, int numElements);
	void SetExternalBuffer (const T* pMemory, int numElements);
	// Takes ownership of the passed memory, including freeing it when this buffer is destroyed.
	void AssumeMemory (T *pMemory, int nSize);

	// Switches the buffer from an external memory buffer to a reallocatable buffer
	// Will copy the current contents of the external buffer to the reallocatable buffer
	void ConvertToGrowableMemory (int nGrowSize);

	// Size
	int NumAllocated () const;
	int Count () const;

	// Grows the memory, so that at least allocated + num elements are allocated
	void Grow (int num = 1);

	// Makes sure we've got at least this much memory
	void EnsureCapacity (int num);

	// Memory deallocation
	void Purge ();

	// Purge all but the given number of elements
	void Purge (int numElements);

	// is the memory externally allocated?
	bool IsExternallyAllocated () const;

	// is the memory read only?
	bool IsReadOnly () const;

	// Set the size by which the memory grows
	void SetGrowSize (int size);

protected:
	void ValidateGrowSize ()
	{
	}

	enum
	{
		EXTERNAL_BUFFER_MARKER = -1,
		EXTERNAL_CONST_BUFFER_MARKER = -2,
	};

	T* m_pMemory;
	int m_nAllocationCount;
	int m_nGrowSize;
};


//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------

template< class T, class I >
CUtlMemory<T, I>::CUtlMemory (int nGrowSize, int nInitAllocationCount) : m_pMemory (0),
m_nAllocationCount (nInitAllocationCount), m_nGrowSize (nGrowSize)
{
	ValidateGrowSize ();
	Assert (nGrowSize >= 0);
	if (m_nAllocationCount)
	{
		UTLMEMORY_TRACK_ALLOC ();
		m_pMemory = (T*)malloc (m_nAllocationCount * sizeof (T));
	}
}

template< class T, class I >
CUtlMemory<T, I>::CUtlMemory (T* pMemory, int numElements) : m_pMemory (pMemory),
m_nAllocationCount (numElements)
{
	// Special marker indicating externally supplied modifyable memory
	m_nGrowSize = EXTERNAL_BUFFER_MARKER;
}

template< class T, class I >
CUtlMemory<T, I>::CUtlMemory (const T* pMemory, int numElements) : m_pMemory ((T*)pMemory),
m_nAllocationCount (numElements)
{
	// Special marker indicating externally supplied modifyable memory
	m_nGrowSize = EXTERNAL_CONST_BUFFER_MARKER;
}

template< class T, class I >
CUtlMemory<T, I>::~CUtlMemory ()
{
	Purge ();
}

template< class T, class I >
void CUtlMemory<T, I>::Init (int nGrowSize /*= 0*/, int nInitSize /*= 0*/)
{
	Purge ();

	m_nGrowSize = nGrowSize;
	m_nAllocationCount = nInitSize;
	ValidateGrowSize ();
	Assert (nGrowSize >= 0);
	if (m_nAllocationCount)
	{
		UTLMEMORY_TRACK_ALLOC ();
		m_pMemory = (T*)malloc (m_nAllocationCount * sizeof (T));
	}
}

//-----------------------------------------------------------------------------
// Switches the buffer from an external memory buffer to a reallocatable buffer
//-----------------------------------------------------------------------------
template< class T, class I >
void CUtlMemory<T, I>::ConvertToGrowableMemory (int nGrowSize)
{
	if (!IsExternallyAllocated ())
		return;

	m_nGrowSize = nGrowSize;
	if (m_nAllocationCount)
	{
		UTLMEMORY_TRACK_ALLOC ();
		//MEM_ALLOC_CREDIT_CLASS ();

		int nNumBytes = m_nAllocationCount * sizeof (T);
		T *pMemory = (T*)malloc (nNumBytes);
		memcpy ((void*)pMemory, (void*)m_pMemory, nNumBytes);
		m_pMemory = pMemory;
	}
	else
	{
		m_pMemory = NULL;
	}
}


//-----------------------------------------------------------------------------
// Attaches the buffer to external memory....
//-----------------------------------------------------------------------------
template< class T, class I >
void CUtlMemory<T, I>::SetExternalBuffer (T* pMemory, int numElements)
{
	// Blow away any existing allocated memory
	Purge ();

	m_pMemory = pMemory;
	m_nAllocationCount = numElements;

	// Indicate that we don't own the memory
	m_nGrowSize = EXTERNAL_BUFFER_MARKER;
}

template< class T, class I >
void CUtlMemory<T, I>::SetExternalBuffer (const T* pMemory, int numElements)
{
	// Blow away any existing allocated memory
	Purge ();

	m_pMemory = const_cast<T*>(pMemory);
	m_nAllocationCount = numElements;

	// Indicate that we don't own the memory
	m_nGrowSize = EXTERNAL_CONST_BUFFER_MARKER;
}

template< class T, class I >
void CUtlMemory<T, I>::AssumeMemory (T* pMemory, int numElements)
{
	// Blow away any existing allocated memory
	Purge ();

	// Simply take the pointer but don't mark us as external
	m_pMemory = pMemory;
	m_nAllocationCount = numElements;
}


//-----------------------------------------------------------------------------
// element access
//-----------------------------------------------------------------------------
template< class T, class I >
inline T& CUtlMemory<T, I>::operator[](I i)
{
	// Avoid function calls in the asserts to improve debug build performance
	Assert (m_nGrowSize != EXTERNAL_CONST_BUFFER_MARKER); //Assert( !IsReadOnly() );
	Assert ((uint32)i < (uint32)m_nAllocationCount);
	return m_pMemory[(uint32)i];
}

template< class T, class I >
inline const T& CUtlMemory<T, I>::operator[](I i) const
{
	// Avoid function calls in the asserts to improve debug build performance
	Assert ((uint32)i < (uint32)m_nAllocationCount);
	return m_pMemory[(uint32)i];
}

template< class T, class I >
inline T& CUtlMemory<T, I>::Element (I i)
{
	// Avoid function calls in the asserts to improve debug build performance
	Assert (m_nGrowSize != EXTERNAL_CONST_BUFFER_MARKER); //Assert( !IsReadOnly() );
	Assert ((uint32)i < (uint32)m_nAllocationCount);
	return m_pMemory[(uint32)i];
}

template< class T, class I >
inline const T& CUtlMemory<T, I>::Element (I i) const
{
	// Avoid function calls in the asserts to improve debug build performance
	Assert ((uint32)i < (uint32)m_nAllocationCount);
	return m_pMemory[(uint32)i];
}


//-----------------------------------------------------------------------------
// is the memory externally allocated?
//-----------------------------------------------------------------------------
template< class T, class I >
bool CUtlMemory<T, I>::IsExternallyAllocated () const
{
	return (m_nGrowSize < 0);
}


//-----------------------------------------------------------------------------
// is the memory read only?
//-----------------------------------------------------------------------------
template< class T, class I >
bool CUtlMemory<T, I>::IsReadOnly () const
{
	return (m_nGrowSize == EXTERNAL_CONST_BUFFER_MARKER);
}


template< class T, class I >
void CUtlMemory<T, I>::SetGrowSize (int nSize)
{
	Assert (!IsExternallyAllocated ());
	Assert (nSize >= 0);
	m_nGrowSize = nSize;
	ValidateGrowSize ();
}


//-----------------------------------------------------------------------------
// Gets the base address (can change when adding elements!)
//-----------------------------------------------------------------------------
template< class T, class I >
inline T* CUtlMemory<T, I>::Base ()
{
	Assert (!IsReadOnly ());
	return m_pMemory;
}

template< class T, class I >
inline const T *CUtlMemory<T, I>::Base () const
{
	return m_pMemory;
}


//-----------------------------------------------------------------------------
// Size
//-----------------------------------------------------------------------------
template< class T, class I >
inline int CUtlMemory<T, I>::NumAllocated () const
{
	return m_nAllocationCount;
}

template< class T, class I >
inline int CUtlMemory<T, I>::Count () const
{
	return m_nAllocationCount;
}


//-----------------------------------------------------------------------------
// Is element index valid?
//-----------------------------------------------------------------------------
template< class T, class I >
inline bool CUtlMemory<T, I>::IsIdxValid (I i) const
{
	// If we always cast 'i' and 'm_nAllocationCount' to unsigned then we can
	// do our range checking with a single comparison instead of two. This gives
	// a modest speedup in debug builds.
	return (uint32)i < (uint32)m_nAllocationCount;
}

//-----------------------------------------------------------------------------
// Grows the memory
//-----------------------------------------------------------------------------
inline int UtlMemory_CalcNewAllocationCount (int nAllocationCount, int nGrowSize, int nNewSize, int nBytesItem)
{
	if (nGrowSize)
	{
		nAllocationCount = ((1 + ((nNewSize - 1) / nGrowSize)) * nGrowSize);
	}
	else
	{
		if (!nAllocationCount)
		{
			// Compute an allocation which is at least as big as a cache line...
			nAllocationCount = (31 + nBytesItem) / nBytesItem;
		}

		while (nAllocationCount < nNewSize)
		{
#ifndef _X360
			nAllocationCount *= 2;
#else
			int nNewAllocationCount = (nAllocationCount * 9) / 8; // 12.5 %
			if (nNewAllocationCount > nAllocationCount)
				nAllocationCount = nNewAllocationCount;
			else
				nAllocationCount *= 2;
#endif
		}
	}

	return nAllocationCount;
}

template< class T, class I >
void CUtlMemory<T, I>::Grow (int num)
{
	Assert (num > 0);

	if (IsExternallyAllocated ())
	{
		// Can't grow a buffer whose memory was externally allocated 
		Assert (0);
		return;
	}

	// Make sure we have at least numallocated + num allocations.
	// Use the grow rules specified for this memory (in m_nGrowSize)
	int nAllocationRequested = m_nAllocationCount + num;

	UTLMEMORY_TRACK_FREE ();

	int nNewAllocationCount = UtlMemory_CalcNewAllocationCount (m_nAllocationCount, m_nGrowSize, nAllocationRequested, sizeof (T));

	// if m_nAllocationRequested wraps index type I, recalculate
	if ((int)(I)nNewAllocationCount < nAllocationRequested)
	{
		if ((int)(I)nNewAllocationCount == 0 && (int)(I)(nNewAllocationCount - 1) >= nAllocationRequested)
		{
			--nNewAllocationCount; // deal w/ the common case of m_nAllocationCount == MAX_USHORT + 1
		}
		else
		{
			if ((int)(I)nAllocationRequested != nAllocationRequested)
			{
				// we've been asked to grow memory to a size s.t. the index type can't address the requested amount of memory
				Assert (0);
				return;
			}
			while ((int)(I)nNewAllocationCount < nAllocationRequested)
			{
				nNewAllocationCount = (nNewAllocationCount + nAllocationRequested) / 2;
			}
		}
	}

	m_nAllocationCount = nNewAllocationCount;

	UTLMEMORY_TRACK_ALLOC ();

	if (m_pMemory)
	{
		m_pMemory = (T*)realloc (m_pMemory, m_nAllocationCount * sizeof (T));
		Assert (m_pMemory);
	}
	else
	{
		m_pMemory = (T*)malloc (m_nAllocationCount * sizeof (T));
		Assert (m_pMemory);
	}
}


//-----------------------------------------------------------------------------
// Makes sure we've got at least this much memory
//-----------------------------------------------------------------------------
template< class T, class I >
inline void CUtlMemory<T, I>::EnsureCapacity (int num)
{
	if (m_nAllocationCount >= num)
		return;

	if (IsExternallyAllocated ())
	{
		// Can't grow a buffer whose memory was externally allocated 
		Assert (0);
		return;
	}

	UTLMEMORY_TRACK_FREE ();

	m_nAllocationCount = num;

	UTLMEMORY_TRACK_ALLOC ();

	if (m_pMemory)
	{
		//MEM_ALLOC_CREDIT_CLASS ();
		m_pMemory = (T*)realloc (m_pMemory, m_nAllocationCount * sizeof (T));
	}
	else
	{
		//MEM_ALLOC_CREDIT_CLASS ();
		m_pMemory = (T*)malloc (m_nAllocationCount * sizeof (T));
	}
}


//-----------------------------------------------------------------------------
// Memory deallocation
//-----------------------------------------------------------------------------
template< class T, class I >
void CUtlMemory<T, I>::Purge ()
{
	if (!IsExternallyAllocated ())
	{
		if (m_pMemory)
		{
			UTLMEMORY_TRACK_FREE ();
			free ((void*)m_pMemory);
			m_pMemory = 0;
		}
		m_nAllocationCount = 0;
	}
}

template< class T, class I >
void CUtlMemory<T, I>::Purge (int numElements)
{
	Assert (numElements >= 0);

	if (numElements > m_nAllocationCount)
	{
		// Ensure this isn't a grow request in disguise.
		Assert (numElements <= m_nAllocationCount);
		return;
	}

	// If we have zero elements, simply do a purge:
	if (numElements == 0)
	{
		Purge ();
		return;
	}

	if (IsExternallyAllocated ())
	{
		// Can't shrink a buffer whose memory was externally allocated, fail silently like purge 
		return;
	}

	// If the number of elements is the same as the allocation count, we are done.
	if (numElements == m_nAllocationCount)
	{
		return;
	}


	if (!m_pMemory)
	{
		// Allocation count is non zero, but memory is null.
		Assert (m_pMemory);
		return;
	}

	UTLMEMORY_TRACK_FREE ();

	m_nAllocationCount = numElements;

	UTLMEMORY_TRACK_ALLOC ();

	// Allocation count > 0, shrink it down.
	//MEM_ALLOC_CREDIT_CLASS ();
	m_pMemory = (T*)realloc (m_pMemory, m_nAllocationCount * sizeof (T));
}

#endif // UTLMEMORY_H
