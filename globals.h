#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#ifndef cbi
	#define cbi(sfr, bit)		(_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
	#define sbi(sfr, bit)		(_SFR_BYTE(sfr) |= _BV(bit))
#endif
#define GLUE(a, b)				a##b
#define SET_(what, p, m)		GLUE(what, p) |= (1 << (m))
#define CLR_(what, p, m)		GLUE(what, p) &= ~(1 << (m))
#define GET_(/* PIN, */ p, m)	(GLUE(PIN, p) & (1 << (m))) >> (m)
#define SET(what, x)			SET_(what, x)
#define CLR(what, x)			CLR_(what, x)
#define GET(/* PIN, */ x)		GET_(x)

#endif /* _GLOBALS_H_ */
