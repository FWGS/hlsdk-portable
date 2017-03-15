// Defining TRUE and FALSE is usually a Bad Idea,
// because you will probably be inconsistent with anyone
// else who had the same clever idea.
// Therefore:  DON'T USE THIS FILE.

#ifndef _bool_h
#define _bool_h 1

// make sure a config.h has been included before

#if defined(__GNUG__) || defined(HAVE_BOOL_TYPE)
#undef TRUE
#undef FALSE
#define TRUE true
#define FALSE false
#else
class bool {
    int rep;
public:
    bool ()
        : rep(0)
    {}

    bool (int i)
        : rep(!!i)
    {}

    bool (const bool &b)
        : rep(b.rep)
    {}

    bool &operator= (const bool b)
    { rep = b.rep; return *this; }

    bool &operator= (int i)
    { rep = !!i; return *this; }

    operator int ()
    { return rep; }
    operator int() const
    { return rep; }
};

#undef true
#undef false
#define true (bool(1))
#define false (bool(0))

#endif

#endif
