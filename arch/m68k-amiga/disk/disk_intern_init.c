
#include <exec/types.h>
#include <exec/alerts.h>
#include <exec/lists.h>
#include <exec/execbase.h>

#include <proto/disk.h>
#include <resources/disk.h>
#include <proto/cia.h>

#include <hardware/custom.h>
#include <hardware/cia.h>
#include <hardware/intbits.h>

#include <disk_intern.h>

#define HAVE_NO_DF0_DISK_ID 1

#define DEBUG 0
#include <aros/debug.h>

void readunitid_internal (struct DiscResource *DiskBase, LONG unitNum)
{
	volatile struct CIA *ciaa = (struct CIA*)0xbfe001;
	volatile struct CIA *ciab = (struct CIA*)0xbfd000;
	UBYTE unitmask = 8 << unitNum;
	ULONG id = 0;
	UBYTE i;

	ciab->ciaprb &= ~0x80; // MTR
	ciab->ciaprb &= ~unitmask; // SELx
	ciab->ciaprb |= unitmask; // SELX
	ciab->ciaprb |= 0x80; // MTR
	ciab->ciaprb &= ~unitmask; // SELx
	ciab->ciaprb |= unitmask; // SELX
	for (i = 0; i < 32; i++) {
		ciab->ciaprb &= ~unitmask; // SELx
		id <<= 1;
		if (ciaa->ciapra & 0x20)
			id |= 1;
		ciab->ciaprb |= unitmask; // SELX
	}
	if (unitNum == 0 && HAVE_NO_DF0_DISK_ID && id == DRT_EMPTY)
		id = DRT_AMIGA;
	DiskBase->dr_UnitID[unitNum] = id;
}

static AROS_UFH4(ULONG, disk_index_interrupt,
    AROS_UFHA(ULONG, dummy, A0),
    AROS_UFHA(void *, data, A1),
    AROS_UFHA(ULONG, dummy2, A5),
    AROS_UFHA(struct ExecBase *, mySysBase, A6))
{ 
    AROS_USERFUNC_INIT
 
 	struct DiscResource *DiskBase = (struct DiscResource*)data;
 	if (DiskBase->dr_Current && DiskBase->dr_Current->dru_Index.is_Code) {
 		AROS_UFC2(void, DiskBase->dr_Current->dru_Index.is_Code,
			AROS_UFCA(APTR, DiskBase->dr_Current->dru_Index.is_Data, A1),
			AROS_UFCA(APTR, DiskBase->dr_Current->dru_Index.is_Code, A5));
 	}
 	return 0;
	
	AROS_USERFUNC_EXIT
}
   static AROS_UFH4(ULONG, disk_sync_interrupt,
    AROS_UFHA(ULONG, dummy, A0),
    AROS_UFHA(void *, data, A1),
    AROS_UFHA(ULONG, dummy2, A5),
    AROS_UFHA(struct ExecBase *, mySysBase, A6))
{ 
    AROS_USERFUNC_INIT
 
 	struct DiscResource *DiskBase = (struct DiscResource*)data;
 	volatile struct Custom *custom = (struct Custom*)0xdff000;
 	custom->intreq = 0x1000;
 	if (DiskBase->dr_Current && DiskBase->dr_Current->dru_DiscSync.is_Code) {
 		AROS_UFC2(void, DiskBase->dr_Current->dru_DiscSync.is_Code,
			AROS_UFCA(APTR, DiskBase->dr_Current->dru_DiscSync.is_Data, A1),
			AROS_UFCA(APTR, DiskBase->dr_Current->dru_DiscSync.is_Code, A5));
 	}
 	return 0;
	
	AROS_USERFUNC_EXIT
}
   static AROS_UFH4(ULONG, disk_block_interrupt,
    AROS_UFHA(ULONG, dummy, A0),
    AROS_UFHA(void *, data, A1),
    AROS_UFHA(ULONG, dummy2, A5),
    AROS_UFHA(struct ExecBase *, mySysBase, A6))
{ 
    AROS_USERFUNC_INIT
 
 	struct DiscResource *DiskBase = (struct DiscResource*)data;
   	volatile struct Custom *custom = (struct Custom*)0xdff000;	
 	custom->intreq = 0x0002;
 	if (DiskBase->dr_Current && DiskBase->dr_Current->dru_DiscBlock.is_Code) {
 		AROS_UFC2(void, DiskBase->dr_Current->dru_DiscBlock.is_Code,
			AROS_UFCA(APTR, DiskBase->dr_Current->dru_DiscBlock.is_Data, A1),
			AROS_UFCA(APTR, DiskBase->dr_Current->dru_DiscBlock.is_Code, A5));
 	}
 	return 0;
	
	AROS_USERFUNC_EXIT
}
   


BOOL disk_internal_init (struct DiscResource *DiskBase)
{
	DiskBase->dr_SysLib = (struct Library*)SysBase;
	
	volatile struct Custom *custom = (struct Custom*)0xdff000;
	volatile struct CIA *ciaa = (struct CIA*)0xbfe001;
	volatile struct CIA *ciab = (struct CIA*)0xbfd000;
	struct Interrupt *inter;
	UBYTE i;
	
	DiskBase->dr_CiaResource = OpenResource("ciab.resource");
	if (!DiskBase->dr_CiaResource)
		Alert(AT_DeadEnd | AG_OpenRes | AO_DiskRsrc);

	NEWLIST(&DiskBase->dr_Waiting);

	ciaa->ciaddra &= ~(0x20 | 0x10 | 0x08 | 0x04); // RDY TK0 WPRO CHNG = input
	ciab->ciaprb = 0xff; // inactive
	ciab->ciaddrb = 0xff; // MTR SELx SIDE DIR STEP = inactive and output
	
	custom->dmacon = 0x0010; // disk dma off
	custom->dsklen = 0x4000; // dsklen idle
	custom->dsksync = 0x4489; // sync
	custom->adkcon = 0x7f00;
	custom->adkcon = 0x8000 | 0x1000 | 0x0400 | 0x0100; // mfm, wordsync, fast

	inter = &DiskBase->dr_Index;
	inter->is_Node.ln_Pri = 0;
	inter->is_Node.ln_Type = NT_INTERRUPT;
	inter->is_Node.ln_Name = "disk index";
	inter->is_Code = (APTR)disk_index_interrupt;
	inter->is_Data = DiskBase;
	Disable();
	AddICRVector(DiskBase->dr_CiaResource, 4, inter); // CIA-B F = index
	// don't want index interrupts unless requested
	AbleICR(DiskBase->dr_CiaResource, 1 << 4);
	Enable();

	inter = &DiskBase->dr_DiscSync;
	inter->is_Node.ln_Pri = 0;
	inter->is_Node.ln_Type = NT_INTERRUPT;
	inter->is_Node.ln_Name = "disk sync";
	inter->is_Code = (APTR)disk_sync_interrupt;
	inter->is_Data = DiskBase;
	Disable();
	SetIntVector(INTB_DSKSYNC, inter);
	custom->intena = 0x1000;
	Enable();

	inter = &DiskBase->dr_DiscBlock;
	inter->is_Node.ln_Pri = 0;
	inter->is_Node.ln_Type = NT_INTERRUPT;
	inter->is_Node.ln_Name = "disk dma done";
	inter->is_Code = (APTR)disk_block_interrupt;
	inter->is_Data = DiskBase;
	Disable();
	SetIntVector(INTB_DSKBLK, inter);
	custom->intena = 0x0002;
	Enable();
	
	// detect drives
	for (i = 0; i < 4; i++) {
		readunitid_internal(DiskBase, i);
		D(bug("DF%d: %08x\n", i, DiskBase->dr_UnitID[i]));
	}
	
	return TRUE;
}
