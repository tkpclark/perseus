//---------------------------------------------------------------------------

#ifndef ThreedesH
#define ThreedesH
//---------------------------------------------------------------------------
/* d3des.h -
*
*	Headers and defines for d3des.c
*	Graven Imagery, 1992.
*
* Copyright (c) 1988,1989,1990,1991,1992 by Richard Outerbridge
*	(GEnie : OUTER; CIS : [71755,204])
*/
#define EN0	0	/* MODE == encrypt */
#define DE1	1	/* MODE == decrypt */
#define bool int
#define true 1
#define false 0

/* A useful alias on 68000-ish machines, but NOT USED HERE. */
typedef union {
        unsigned long blok[2]; unsigned short word[4]; unsigned char byte[8];
} M68K;
extern void deskey(unsigned char *, short);
/*	hexkey[8]	MODE
* Sets the internal key register according to the hexadecimal
* key contained in the 8 bytes of hexkey, according to the DES,
* for encryption or decryption according to MODE.
*/
extern void usekey(unsigned long *);
/*	cookedkey[32]
* Loads the internal key register with the data in cookedkey.
*/
extern void cpkey(unsigned long *);
/*	cookedkey[32]
* Copies the contents of the internal key register into the storage
* located at &cookedkey[0].
*/
extern void des(unsigned char *, unsigned char *);
/*	from[8]	to[8]
* Encrypts/Decrypts (according to the key currently loaded in the
* internal key register) one block of eight bytes at address 'from'
* into the block at address 'to'.  They can be the same.
*/
extern void des2key(unsigned char *, short);
/*	hexkey[16]	MODE
* Sets the internal key registerS according to the hexadecimal
* keyS contained in the 16 bytes of hexkey, according to the DES,
* for DOUBLE encryption or decryption according to MODE.
* NOTE: this clobbers all three key registers!
*/
extern void Ddes(unsigned char *, unsigned char *);
/*	from[8]	to[8]
* Encrypts/Decrypts (according to the keyS currently loaded in the
* internal key registerS) one block of eight bytes at address 'from'
* into the block at address 'to'.  They can be the same.
*/
bool T_3DES(bool bEnspot, unsigned char* pbyKey,int nLength, unsigned char*pbySource, unsigned char* pbyTarget);
bool T_DES(bool bEnspot, unsigned char* pbyKey,int nLength, unsigned char*pbySource, unsigned char* pbyTarget);
/* TFCA_3DES进行DES加密，TFCA_DES进行DES加密。参数含义如下： bEnspot：true表示加密，false表示解密 pbyKey：指向进行加密的密钥
nLength：需要进行加密的数据长度，以字节为单位，需要为8的倍数
pbySource：指向需要进行加密的数据首指针
pbyTarget：指向返回的加密后的数据首指针
*/
#endif
