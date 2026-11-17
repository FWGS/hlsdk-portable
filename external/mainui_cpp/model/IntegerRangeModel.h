#pragma once
#ifndef INTEGERRANGEMODEL_H
#define INTEGERRANGEMODEL_H

#include "BaseArrayModel.h"

class CIntegerRangeModel : public CMenuBaseArrayModel
{
public:
	CIntegerRangeModel( int start, int count ) : m_iStart( start ), m_iCount( count ) { }

	void Update() override { }
	int GetRows() const override { return m_iCount; }
	const char *GetText( int line ) override
	{
		static char buf[12];
		snprintf( buf, sizeof( buf ), "%d", m_iStart + line );
		return buf;
	}

private:
	int m_iStart;
	int m_iCount;
};

#endif // INTEGERRANGEMODEL_H
