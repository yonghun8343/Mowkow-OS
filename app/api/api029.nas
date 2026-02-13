[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api029.nas"]

		GLOBAL	_api_fopen_rw

[SECTION .text]

_api_fopen_rw:			; int api_fopen_rw(char *fname, int mode);
		PUSH	EBX
		MOV		EDX,29
		MOV		EBX,[ESP+8]			; fname
		MOV 	ECX,[ESP+12]		; mode
		INT		0x40
		POP		EBX
		RET
