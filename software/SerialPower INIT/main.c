#include <SetupA4.h>#include <GestaltEqu.h>#include <Folders.h>#include <Power.h>void newSerialPower();Handle myINITHandle;long oldSerialPower;const int kInitIcon = 128;const int kBadMachineIcon = 129;const int kDisabledIcon = 130;#define PREF_FILE "\pPB AirTalk Preferences"#define PREF_MAGIC 'PBAT'#define FLAGS_ACTIVE 0x00struct cdevPrefs {	unsigned long magic;	unsigned short version;	unsigned short flags;};Boolean IsPatchEnabled();void newSerialPower() {	long saveD1;		// The docs suggest to save D1	// since SetUpA4 could mess it	// still a copy is useful so	// we can and it	asm {		move.L D1,saveD1	}		SetUpA4();		asm {		move.L saveD1,D1				// Copied from PowerMgtPatches.a		// jump away if this is not 0xA650,		// the SerialPower trap		and.w #0x0600,D1		cmp.w #0x0600,D1		bne.s @doOldTrap			// Ok SerialPower was really called		// Transform AOnIgnoreModem to AOn		cmp.w #0x05,D0		bne.s @doOldTrap		move.w #0x04,D0@doOldTrap:		move.L saveD1,D1		move.L oldSerialPower,A1		jsr (A1)	}		RestoreA4();}main () {	Ptr myINITPtr;	Handle showInitIconFunction;	long powerManagerGestalt;	Boolean doPatch;		asm {		move.L A0,myINITPtr;	}		RememberA0();	SetUpA4();		myINITHandle = RecoverHandle(myINITPtr);	DetachResource(myINITHandle);		// Load the code for ShowInitIcon	showInitIconFunction = GetResource('Code', 7000);		// Make sure this machine has the Power Manager	Gestalt(gestaltPowerMgrAttr, &powerManagerGestalt);		doPatch = false;		if (powerManagerGestalt & (1 << gestaltPMgrExists) != 0) {		// Supported on this machine		// Read pref and see if disabled		if (IsPatchEnabled()) {			doPatch = true;						// Show the icon if the CODE loaded properly			if (showInitIconFunction)				((pascal void(*)(short, Boolean)) *showInitIconFunction)(kInitIcon, true);		} else {			// We are disabled, show the disabled icon and do nothing.			if (showInitIconFunction)				((pascal void(*)(short, Boolean)) *showInitIconFunction)(kDisabledIcon, true);		}			} else {		// Not supported on this machine. Show bad icon		if (showInitIconFunction)			((pascal void(*)(short, Boolean)) *showInitIconFunction)(kBadMachineIcon, true);	}			if (doPatch) {		// Apply the patch		oldSerialPower = GetOSTrapAddress(0xA685);		SetOSTrapAddress((long)newSerialPower, 0xA685);		// Turn power on so AirTalk has time to boot		AOn();	}		RestoreA4();}// Note by default the patch is enabledBoolean IsPatchEnabled() {OSErr res;	short fnr = -1;	long prefDir;	short prefVol;	long count;	struct cdevPrefs* prefs;	Boolean retVal = true;		prefs = (struct cdevPrefs*)NewPtr(sizeof(struct cdevPrefs));	if (prefs == 0L)		return true;		// We should run only on system 7 so assume it is here	res = FindFolder(kOnSystemDisk, kPreferencesFolderType,			   kCreateFolder, &prefVol, &prefDir);			   	if (res != noErr) {		goto cleanup3;	}			res = HOpen(prefVol, prefDir, PREF_FILE, fsRdWrPerm, &fnr);	if (res != noErr) {		goto cleanup3;	}			SetFPos(fnr, fsFromStart, 0L);		count = sizeof(struct cdevPrefs);	res = FSRead(fnr, &count, prefs);		if (res != noErr) {		goto cleanup2;	}		// Is the data in the right format?	if (prefs->magic != PREF_MAGIC) {		goto cleanup2;	}		// Enabled by default. It is actually disabled?	if ((prefs->flags & (1<<FLAGS_ACTIVE)) == 0)		retVal = false;cleanup2:	FSClose(fnr);cleanup3:	DisposPtr(prefs);	return retVal;}