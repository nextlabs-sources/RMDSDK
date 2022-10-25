#pragma once

#ifdef  __cplusplus
extern "C" {
#endif //  __cplusplus


#define ROUND_TO_SIZE(Length, Alignment)        ((ULONG_PTR)(((ULONG_PTR)Length + ((ULONG_PTR)Alignment - 1)) & ~((ULONG_PTR)Alignment - 1)))
#define IS_ALIGNED(Pointer, Alignment)          ((((ULONG_PTR) (Pointer)) & ((Alignment) - 1)) == 0)

#define SWAP_ENDIAN(d)	((((d)&0xFF) << 24) | (((d)&0xFF00) << 8) | (((d)&0xFF0000) >> 8) | (((d)&0xFF000000) >> 24))

#define OneKB			1024LL
#define OneMB			1048576LL
#define OneGB			1073741824LL
#define OneTB			1099511627776LL

#define BytesToKB(x)		((x)/1024)
#define BytesToMB(x)		((x)/1048576)
#define BytesToGB(x)		((x)/1073741824)
#define BytesToTB(x)		((x)/1099511627776)
#define BytesToFloatKB(x)	((x)/1024.0)
#define BytesToFloatMB(x)	((x)/1048576.0)
#define BytesToFloatGB(x)	((x)/1073741824.0)
#define BytesToFloatTB(x)	((x)/1099511627776.0)

#ifndef FlagOn
#define FlagOn(_F,_SF)        ((_F) & (_SF))
#endif

#ifndef BoolFlagOn
#define BoolFlagOn(F,SF)   ((bool)(((F) & (SF)) != 0))
#endif

#ifndef SetFlag
#define SetFlag(_F,_SF)       ((_F) |= (_SF))
#endif

#ifndef ClearFlag
#define ClearFlag(_F,_SF)     ((_F) &= ~(_SF))
#endif

typedef struct _FORMATTEDSIZE {
	int tb;
	int gb;
	int mb;
	int kb;
	int bytes;
} FORMATTEDSIZE, *PFORMATTEDSIZE;

__forceinline void FormatSize(__int64 total_bytes, PFORMATTEDSIZE fmtsize)
{
	memset(fmtsize, 0, sizeof(FORMATTEDSIZE));

	if (total_bytes >= OneTB) {
		fmtsize->tb = (int)BytesToTB(total_bytes);
		total_bytes = (total_bytes % OneTB);
	}

	if (total_bytes >= OneGB) {
		fmtsize->gb = (int)BytesToGB(total_bytes);
		total_bytes = (total_bytes % OneGB);
	}

	if (total_bytes >= OneMB) {
		fmtsize->mb = (int)BytesToMB(total_bytes);
		total_bytes = (total_bytes % OneMB);
	}

	if (total_bytes >= OneKB) {
		fmtsize->kb = (int)BytesToKB(total_bytes);
		total_bytes = (total_bytes % OneKB);
	}
	
	fmtsize->bytes = (int)total_bytes;
}


#define SecondsPerMinute	60L
#define SecondsPerHour		3600L
#define SecondsPerDay 		86400L

#define SecondsToMinutes(x)	((x)/SecondsPerMinute)
#define SecondsToHours(x)	((x)/SecondsPerHour)
#define SecondsToDays(x)	((x)/SecondsPerDay)


#ifdef  __cplusplus
}
#endif //  __cplusplus
