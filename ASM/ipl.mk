; harinote-ipl
; TAB=4

(불러오기 "common.mk")

; (정의 머리글자 0육7ㄷ) ; MBR		EQU		0x7c0
; (정의 바이트_섹터 512) ; BYTES_PER_SECTOR	EQU		512
; (정의 섹터_클러스터 1) ; SECTORS_PER_CLUSTER	EQU		1
; (정의 예약된_섹터 1) ; RESERVED_SECTORS	EQU		1
; (정의 FAT_개수 2) ; NUM_FATS	EQU		2
; (정의 루트_디렉토리_엔트리 224) ; ROOT_DIR_ENTRIES	EQU		224
; (정의 총_섹터_16 2880) ; TOTAL_SECTORS_16	EQU		2880
; (정의 미디어_종류 0xF0) ; MEDIA_TYPE	EQU		0xF0
; (정의 섹터당_FAT_16 9) ; SECTORS_PER_FAT_16	EQU		9
; (정의 섹터당_트랙 18) ; SECTORS_PER_TRACK	EQU		18
; (정의 헤드_수 2) ; NUMBER_OF_HEADS	EQU		2

(정의 실린더 9) ; CYLS	EQU		9
(정의 시작점 0육7ㄷ00) ; ORG		0x7c00

; FAT12 Format
; JMP entry 는 마지막에 앞에 끼워넣는 방식으로 (계산 필요)
(정의 헤더 `(
  0육90 ; DB 0x90 (NOP)
  ,(문을육 'H)
  ,(문을육 'A)
  ,(문을육 'R)
  ,(문을육 'I)
  ,(문을육 'N)
  ,(문을육 'O)
  ,(문을육 'T)
  ,(문을육 'E) ; DB 'HARINOTE'
  ,@(2바이트 512) ; DW 512
  0육01 ; DB 1
  ,@(2바이트 1) ; DW 1
  0육02 ; DB 2
  ,@(2바이트 224) ; DW 224
  ,@(2바이트 2880) ; DW 2880
  0육ㅂ0 ; DB 0xF0
  ,@(2바이트 9) ; DW 9
  ,@(2바이트 18) ; DW 18
  ,@(2바이트 2) ; DW 2
  ,@(4바이트 0) ; DD 0
  ,@(4바이트 2880) ; DD 2880
  0육00 ; DB 0
  0육00 ; DB 0
  0육29 ; DB 0
  ,@(4바이트 0육ㅂㅂㅂㅂㅂㅂㅂㅂ)
  ,(문을육 'H)
  ,(문을육 'A)
  ,(문을육 'R)
  ,(문을육 'I)
  ,(문을육 'N)
  ,(문을육 'O)
  ,(문을육 'T)
  ,(문을육 'E)
  ,(문을육 'O)
  ,(문을육 'S)
  ,(문을육 '공백) ; DB "HARINOTEOS "
  ,(문을육 'F)
  ,(문을육 'A)
  ,(문을육 'T)
  ,(문을육 '1)
  ,(문을육 '2)
  ,(문을육 '공백)
  ,(문을육 '공백)
  ,(문을육 '공백) ; DB "FAT12   "
  ,@(채워넣기 0육00 18) ;RESB 18
))

(정의 헤더
  (짝 뛰기 (짝 (길이 헤더) 헤더))
) ; JMP Entry

(정의 진입점 `(
  (이동_셈 0) ; MOV		AX, 0
  (이동_레지스터 _쌓음판 _셈) ; MOV		SS, AX
  (이동_레지스터 _쌓음 0육7c00) ; MOV		SP, 0x7c00
  (이동_레지스터 _자료판 _셈) ; MOV		DS, AX

  (이동_레지스터 _셈)
))



; Program body