#include <aros/asmcall.h>
#include <exec/types.h>

#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include "wbprojecticonclass.h"

AROS_UFH3(IPTR,wbProjectIconClassDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
{
	ULONG retval;

	switch(msg->MethodID)
	{
		default:
			retval=DoSuperMethodA(cl, obj, msg);
			break;
	}

	return retval;
}

