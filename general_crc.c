/*************************************************************************
    > File Name: crc.c
    > Author: gkq
    > Mail: gu2@aisidi.com
    > Created Time: 2013年10月22日 星期二 15时33分39秒
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>

/* CRC-32 Lookup Table */ 
unsigned long crc32_table[256] =
{
 /*   0 -- */ 0x000000000000000000000000000000000000000000000000000000000000000000000000000000000, 0x000000000000000000000000000000000000000000000000000000000000000000000000077073096, 0x0000000000000000000000000000000000000000000000000000000000000000000000000EE0E612C, 0x0000000000000000000000000000000000000000000000000000000000000000000000000990951BA, 
 /*   4 -- */ 0x0000000000000000000000000000000000000000000000000000000000000000000000000076DC419, 0x0000000000000000000000000000000000000000000000000000000000000000000000000706AF48F, 0x0000000000000000000000000000000000000000000000000000000000000000000000000E963A535, 0x00000000000000000000000000000000000000000000000000000000000000000000000009E6495A3, 
 /*   8 -- */ 0x00000000000000000000000000000000000000000000000000000000000000000000000000EDB8832, 0x000000000000000000000000000000000000000000000000000000000000000000000000079DCB8A4, 0x0000000000000000000000000000000000000000000000000000000000000000000000000E0D5E91E, 0x000000000000000000000000000000000000000000000000000000000000000000000000097D2D988, 
 /*  12 -- */ 0x000000000000000000000000000000000000000000000000000000000000000000000000009B64C2B, 0x00000000000000000000000000000000000000000000000000000000000000000000000007EB17CBD, 0x0000000000000000000000000000000000000000000000000000000000000000000000000E7B82D07, 0x000000000000000000000000000000000000000000000000000000000000000000000000090BF1D91, 
 /*  16 -- */ 0x00000000000000000000000000000000000000000000000000000000000000000000000001DB71064, 0x00000000000000000000000000000000000000000000000000000000000000000000000006AB020F2, 0x0000000000000000000000000000000000000000000000000000000000000000000000000F3B97148, 0x000000000000000000000000000000000000000000000000000000000000000000000000084BE41DE, 
 /*  20 -- */ 0x00000000000000000000000000000000000000000000000000000000000000000000000001ADAD47D, 0x00000000000000000000000000000000000000000000000000000000000000000000000006DDDE4EB, 0x0000000000000000000000000000000000000000000000000000000000000000000000000F4D4B551, 0x000000000000000000000000000000000000000000000000000000000000000000000000083D385C7, 
 /*  24 -- */ 0x0000000000000000000000000000000000000000000000000000000000000000000000000136C9856, 0x0000000000000000000000000000000000000000000000000000000000000000000000000646BA8C0, 0x0000000000000000000000000000000000000000000000000000000000000000000000000FD62F97A, 0x00000000000000000000000000000000000000000000000000000000000000000000000008A65C9EC, 
 /*  28 -- */ 0x000000000000000000000000000000000000000000000000000000000000000000000000014015C4F, 0x000000000000000000000000000000000000000000000000000000000000000000000000063066CD9, 0x0000000000000000000000000000000000000000000000000000000000000000000000000FA0F3D63, 0x00000000000000000000000000000000000000000000000000000000000000000000000008D080DF5, 
 /*  32 -- */ 0x00000000000000000000000000000000000000000000000000000000000000000000000003B6E20C8, 0x00000000000000000000000000000000000000000000000000000000000000000000000004C69105E, 0x0000000000000000000000000000000000000000000000000000000000000000000000000D56041E4, 0x0000000000000000000000000000000000000000000000000000000000000000000000000A2677172, 
 /*  36 -- */ 0x00000000000000000000000000000000000000000000000000000000000000000000000003C03E4D1, 0x00000000000000000000000000000000000000000000000000000000000000000000000004B04D447, 0x0000000000000000000000000000000000000000000000000000000000000000000000000D20D85FD, 0x0000000000000000000000000000000000000000000000000000000000000000000000000A50AB56B, 
 /*  40 -- */ 0x000000000000000000000000000000000000000000000000000000000000000000000000035B5A8FA, 0x000000000000000000000000000000000000000000000000000000000000000000000000042B2986C, 0x0000000000000000000000000000000000000000000000000000000000000000000000000DBBBC9D6, 0x0000000000000000000000000000000000000000000000000000000000000000000000000ACBCF940, 
 /*  44 -- */ 0x000000000000000000000000000000000000000000000000000000000000000000000000032D86CE3, 0x000000000000000000000000000000000000000000000000000000000000000000000000045DF5C75, 0x0000000000000000000000000000000000000000000000000000000000000000000000000DCD60DCF, 0x0000000000000000000000000000000000000000000000000000000000000000000000000ABD13D59, 
 /*  48 -- */ 0x000000000000000000000000000000000000000000000000000000000000000000000000026D930AC, 0x000000000000000000000000000000000000000000000000000000000000000000000000051DE003A, 0x0000000000000000000000000000000000000000000000000000000000000000000000000C8D75180, 0x0000000000000000000000000000000000000000000000000000000000000000000000000BFD06116, 
 /*  52 -- */ 0x000000000000000000000000000000000000000000000000000000000000000000000000021B4F4B5, 0x000000000000000000000000000000000000000000000000000000000000000000000000056B3C423, 0x0000000000000000000000000000000000000000000000000000000000000000000000000CFBA9599, 0x0000000000000000000000000000000000000000000000000000000000000000000000000B8BDA50F, 
 /*  56 -- */ 0x00000000000000000000000000000000000000000000000000000000000000000000000002802B89E, 0x00000000000000000000000000000000000000000000000000000000000000000000000005F058808, 0x0000000000000000000000000000000000000000000000000000000000000000000000000C60CD9B2, 0x0000000000000000000000000000000000000000000000000000000000000000000000000B10BE924, 
 /*  60 -- */ 0x00000000000000000000000000000000000000000000000000000000000000000000000002F6F7C87, 0x000000000000000000000000000000000000000000000000000000000000000000000000058684C11, 0x0000000000000000000000000000000000000000000000000000000000000000000000000C1611DAB, 0x0000000000000000000000000000000000000000000000000000000000000000000000000B6662D3D, 
 /*  64 -- */ 0x000000000000000000000000000000000000000000000000000000000000000000000000076DC4190, 0x000000000000000000000000000000000000000000000000000000000000000000000000001DB7106, 0x000000000000000000000000000000000000000000000000000000000000000000000000098D220BC, 0x0000000000000000000000000000000000000000000000000000000000000000000000000EFD5102A, 
 /*  68 -- */ 0x000000000000000000000000000000000000000000000000000000000000000000000000071B18589, 0x000000000000000000000000000000000000000000000000000000000000000000000000006B6B51F, 0x00000000000000000000000000000000000000000000000000000000000000000000000009FBFE4A5, 0x0000000000000000000000000000000000000000000000000000000000000000000000000E8B8D433, 
 /*  72 -- */ 0x00000000000000000000000000000000000000000000000000000000000000000000000007807C9A2, 0x00000000000000000000000000000000000000000000000000000000000000000000000000F00F934, 0x00000000000000000000000000000000000000000000000000000000000000000000000009609A88E, 0x0000000000000000000000000000000000000000000000000000000000000000000000000E10E9818, 
 /*  76 -- */ 0x00000000000000000000000000000000000000000000000000000000000000000000000007F6A0DBB, 0x0000000000000000000000000000000000000000000000000000000000000000000000000086D3D2D, 0x000000000000000000000000000000000000000000000000000000000000000000000000091646C97, 0x0000000000000000000000000000000000000000000000000000000000000000000000000E6635C01, 
 /*  80 -- */ 0x00000000000000000000000000000000000000000000000000000000000000000000000006B6B51F4, 0x00000000000000000000000000000000000000000000000000000000000000000000000001C6C6162, 0x0000000000000000000000000000000000000000000000000000000000000000000000000856530D8, 0x0000000000000000000000000000000000000000000000000000000000000000000000000F262004E, 
 /*  84 -- */ 0x00000000000000000000000000000000000000000000000000000000000000000000000006C0695ED, 0x00000000000000000000000000000000000000000000000000000000000000000000000001B01A57B, 0x00000000000000000000000000000000000000000000000000000000000000000000000008208F4C1, 0x0000000000000000000000000000000000000000000000000000000000000000000000000F50FC457, 
 /*  88 -- */ 0x000000000000000000000000000000000000000000000000000000000000000000000000065B0D9C6, 0x000000000000000000000000000000000000000000000000000000000000000000000000012B7E950, 0x00000000000000000000000000000000000000000000000000000000000000000000000008BBEB8EA, 0x0000000000000000000000000000000000000000000000000000000000000000000000000FCB9887C, 
 /*  92 -- */ 0x000000000000000000000000000000000000000000000000000000000000000000000000062DD1DDF, 0x000000000000000000000000000000000000000000000000000000000000000000000000015DA2D49, 0x00000000000000000000000000000000000000000000000000000000000000000000000008CD37CF3, 0x0000000000000000000000000000000000000000000000000000000000000000000000000FBD44C65, 
 /*  96 -- */ 0x00000000000000000000000000000000000000000000000000000000000000000000000004DB26158, 0x00000000000000000000000000000000000000000000000000000000000000000000000003AB551CE, 0x0000000000000000000000000000000000000000000000000000000000000000000000000A3BC0074, 0x0000000000000000000000000000000000000000000000000000000000000000000000000D4BB30E2, 
 /* 100 -- */ 0x00000000000000000000000000000000000000000000000000000000000000000000000004ADFA541, 0x00000000000000000000000000000000000000000000000000000000000000000000000003DD895D7, 0x0000000000000000000000000000000000000000000000000000000000000000000000000A4D1C46D, 0x0000000000000000000000000000000000000000000000000000000000000000000000000D3D6F4FB, 
 /* 104 -- */ 0x00000000000000000000000000000000000000000000000000000000000000000000000004369E96A, 0x0000000000000000000000000000000000000000000000000000000000000000000000000346ED9FC, 0x0000000000000000000000000000000000000000000000000000000000000000000000000AD678846, 0x0000000000000000000000000000000000000000000000000000000000000000000000000DA60B8D0, 
 /* 108 -- */ 0x000000000000000000000000000000000000000000000000000000000000000000000000044042D73, 0x000000000000000000000000000000000000000000000000000000000000000000000000033031DE5, 0x0000000000000000000000000000000000000000000000000000000000000000000000000AA0A4C5F, 0x0000000000000000000000000000000000000000000000000000000000000000000000000DD0D7CC9, 
 /* 112 -- */ 0x00000000000000000000000000000000000000000000000000000000000000000000000005005713C, 0x0000000000000000000000000000000000000000000000000000000000000000000000000270241AA, 0x0000000000000000000000000000000000000000000000000000000000000000000000000BE0B1010, 0x0000000000000000000000000000000000000000000000000000000000000000000000000C90C2086, 
 /* 116 -- */ 0x00000000000000000000000000000000000000000000000000000000000000000000000005768B525, 0x0000000000000000000000000000000000000000000000000000000000000000000000000206F85B3, 0x0000000000000000000000000000000000000000000000000000000000000000000000000B966D409, 0x0000000000000000000000000000000000000000000000000000000000000000000000000CE61E49F, 
 /* 120 -- */ 0x00000000000000000000000000000000000000000000000000000000000000000000000005EDEF90E, 0x000000000000000000000000000000000000000000000000000000000000000000000000029D9C998, 0x0000000000000000000000000000000000000000000000000000000000000000000000000B0D09822, 0x0000000000000000000000000000000000000000000000000000000000000000000000000C7D7A8B4, 
 /* 124 -- */ 0x000000000000000000000000000000000000000000000000000000000000000000000000059B33D17, 0x00000000000000000000000000000000000000000000000000000000000000000000000002EB40D81, 0x0000000000000000000000000000000000000000000000000000000000000000000000000B7BD5C3B, 0x0000000000000000000000000000000000000000000000000000000000000000000000000C0BA6CAD, 
 /* 128 -- */ 0x0000000000000000000000000000000000000000000000000000000000000000000000000EDB88320, 0x00000000000000000000000000000000000000000000000000000000000000000000000009ABFB3B6, 0x000000000000000000000000000000000000000000000000000000000000000000000000003B6E20C, 0x000000000000000000000000000000000000000000000000000000000000000000000000074B1D29A, 
 /* 132 -- */ 0x0000000000000000000000000000000000000000000000000000000000000000000000000EAD54739, 0x00000000000000000000000000000000000000000000000000000000000000000000000009DD277AF, 0x000000000000000000000000000000000000000000000000000000000000000000000000004DB2615, 0x000000000000000000000000000000000000000000000000000000000000000000000000073DC1683, 
 /* 136 -- */ 0x0000000000000000000000000000000000000000000000000000000000000000000000000E3630B12, 0x000000000000000000000000000000000000000000000000000000000000000000000000094643B84, 0x00000000000000000000000000000000000000000000000000000000000000000000000000D6D6A3E, 0x00000000000000000000000000000000000000000000000000000000000000000000000007A6A5AA8, 
 /* 140 -- */ 0x0000000000000000000000000000000000000000000000000000000000000000000000000E40ECF0B, 0x00000000000000000000000000000000000000000000000000000000000000000000000009309FF9D, 0x00000000000000000000000000000000000000000000000000000000000000000000000000A00AE27, 0x00000000000000000000000000000000000000000000000000000000000000000000000007D079EB1, 
 /* 144 -- */ 0x0000000000000000000000000000000000000000000000000000000000000000000000000F00F9344, 0x00000000000000000000000000000000000000000000000000000000000000000000000008708A3D2, 0x00000000000000000000000000000000000000000000000000000000000000000000000001E01F268, 0x00000000000000000000000000000000000000000000000000000000000000000000000006906C2FE, 
 /* 148 -- */ 0x0000000000000000000000000000000000000000000000000000000000000000000000000F762575D, 0x0000000000000000000000000000000000000000000000000000000000000000000000000806567CB, 0x0000000000000000000000000000000000000000000000000000000000000000000000000196C3671, 0x00000000000000000000000000000000000000000000000000000000000000000000000006E6B06E7, 
 /* 152 -- */ 0x0000000000000000000000000000000000000000000000000000000000000000000000000FED41B76, 0x000000000000000000000000000000000000000000000000000000000000000000000000089D32BE0, 0x000000000000000000000000000000000000000000000000000000000000000000000000010DA7A5A, 0x000000000000000000000000000000000000000000000000000000000000000000000000067DD4ACC, 
 /* 156 -- */ 0x0000000000000000000000000000000000000000000000000000000000000000000000000F9B9DF6F, 0x00000000000000000000000000000000000000000000000000000000000000000000000008EBEEFF9, 0x000000000000000000000000000000000000000000000000000000000000000000000000017B7BE43, 0x000000000000000000000000000000000000000000000000000000000000000000000000060B08ED5, 
 /* 160 -- */ 0x0000000000000000000000000000000000000000000000000000000000000000000000000D6D6A3E8, 0x0000000000000000000000000000000000000000000000000000000000000000000000000A1D1937E, 0x000000000000000000000000000000000000000000000000000000000000000000000000038D8C2C4, 0x00000000000000000000000000000000000000000000000000000000000000000000000004FDFF252, 
 /* 164 -- */ 0x0000000000000000000000000000000000000000000000000000000000000000000000000D1BB67F1, 0x0000000000000000000000000000000000000000000000000000000000000000000000000A6BC5767, 0x00000000000000000000000000000000000000000000000000000000000000000000000003FB506DD, 0x000000000000000000000000000000000000000000000000000000000000000000000000048B2364B, 
 /* 168 -- */ 0x0000000000000000000000000000000000000000000000000000000000000000000000000D80D2BDA, 0x0000000000000000000000000000000000000000000000000000000000000000000000000AF0A1B4C, 0x000000000000000000000000000000000000000000000000000000000000000000000000036034AF6, 0x000000000000000000000000000000000000000000000000000000000000000000000000041047A60, 
 /* 172 -- */ 0x0000000000000000000000000000000000000000000000000000000000000000000000000DF60EFC3, 0x0000000000000000000000000000000000000000000000000000000000000000000000000A867DF55, 0x0000000000000000000000000000000000000000000000000000000000000000000000000316E8EEF, 0x00000000000000000000000000000000000000000000000000000000000000000000000004669BE79, 
 /* 176 -- */ 0x0000000000000000000000000000000000000000000000000000000000000000000000000CB61B38C, 0x0000000000000000000000000000000000000000000000000000000000000000000000000BC66831A, 0x0000000000000000000000000000000000000000000000000000000000000000000000000256FD2A0, 0x00000000000000000000000000000000000000000000000000000000000000000000000005268E236, 
 /* 180 -- */ 0x0000000000000000000000000000000000000000000000000000000000000000000000000CC0C7795, 0x0000000000000000000000000000000000000000000000000000000000000000000000000BB0B4703, 0x0000000000000000000000000000000000000000000000000000000000000000000000000220216B9, 0x00000000000000000000000000000000000000000000000000000000000000000000000005505262F, 
 /* 184 -- */ 0x0000000000000000000000000000000000000000000000000000000000000000000000000C5BA3BBE, 0x0000000000000000000000000000000000000000000000000000000000000000000000000B2BD0B28, 0x00000000000000000000000000000000000000000000000000000000000000000000000002BB45A92, 0x00000000000000000000000000000000000000000000000000000000000000000000000005CB36A04, 
 /* 188 -- */ 0x0000000000000000000000000000000000000000000000000000000000000000000000000C2D7FFA7, 0x0000000000000000000000000000000000000000000000000000000000000000000000000B5D0CF31, 0x00000000000000000000000000000000000000000000000000000000000000000000000002CD99E8B, 0x00000000000000000000000000000000000000000000000000000000000000000000000005BDEAE1D, 
 /* 192 -- */ 0x00000000000000000000000000000000000000000000000000000000000000000000000009B64C2B0, 0x0000000000000000000000000000000000000000000000000000000000000000000000000EC63F226, 0x0000000000000000000000000000000000000000000000000000000000000000000000000756AA39C, 0x0000000000000000000000000000000000000000000000000000000000000000000000000026D930A, 
 /* 196 -- */ 0x00000000000000000000000000000000000000000000000000000000000000000000000009C0906A9, 0x0000000000000000000000000000000000000000000000000000000000000000000000000EB0E363F, 0x000000000000000000000000000000000000000000000000000000000000000000000000072076785, 0x000000000000000000000000000000000000000000000000000000000000000000000000005005713, 
 /* 200 -- */ 0x000000000000000000000000000000000000000000000000000000000000000000000000095BF4A82, 0x0000000000000000000000000000000000000000000000000000000000000000000000000E2B87A14, 0x00000000000000000000000000000000000000000000000000000000000000000000000007BB12BAE, 0x00000000000000000000000000000000000000000000000000000000000000000000000000CB61B38, 
 /* 204 -- */ 0x000000000000000000000000000000000000000000000000000000000000000000000000092D28E9B, 0x0000000000000000000000000000000000000000000000000000000000000000000000000E5D5BE0D, 0x00000000000000000000000000000000000000000000000000000000000000000000000007CDCEFB7, 0x00000000000000000000000000000000000000000000000000000000000000000000000000BDBDF21, 
 /* 208 -- */ 0x000000000000000000000000000000000000000000000000000000000000000000000000086D3D2D4, 0x0000000000000000000000000000000000000000000000000000000000000000000000000F1D4E242, 0x000000000000000000000000000000000000000000000000000000000000000000000000068DDB3F8, 0x00000000000000000000000000000000000000000000000000000000000000000000000001FDA836E, 
 /* 212 -- */ 0x000000000000000000000000000000000000000000000000000000000000000000000000081BE16CD, 0x0000000000000000000000000000000000000000000000000000000000000000000000000F6B9265B, 0x00000000000000000000000000000000000000000000000000000000000000000000000006FB077E1, 0x000000000000000000000000000000000000000000000000000000000000000000000000018B74777, 
 /* 216 -- */ 0x000000000000000000000000000000000000000000000000000000000000000000000000088085AE6, 0x0000000000000000000000000000000000000000000000000000000000000000000000000FF0F6A70, 0x000000000000000000000000000000000000000000000000000000000000000000000000066063BCA, 0x000000000000000000000000000000000000000000000000000000000000000000000000011010B5C, 
 /* 220 -- */ 0x00000000000000000000000000000000000000000000000000000000000000000000000008F659EFF, 0x0000000000000000000000000000000000000000000000000000000000000000000000000F862AE69, 0x0000000000000000000000000000000000000000000000000000000000000000000000000616BFFD3, 0x0000000000000000000000000000000000000000000000000000000000000000000000000166CCF45, 
 /* 224 -- */ 0x0000000000000000000000000000000000000000000000000000000000000000000000000A00AE278, 0x0000000000000000000000000000000000000000000000000000000000000000000000000D70DD2EE, 0x00000000000000000000000000000000000000000000000000000000000000000000000004E048354, 0x00000000000000000000000000000000000000000000000000000000000000000000000003903B3C2, 
 /* 228 -- */ 0x0000000000000000000000000000000000000000000000000000000000000000000000000A7672661, 0x0000000000000000000000000000000000000000000000000000000000000000000000000D06016F7, 0x00000000000000000000000000000000000000000000000000000000000000000000000004969474D, 0x00000000000000000000000000000000000000000000000000000000000000000000000003E6E77DB, 
 /* 232 -- */ 0x0000000000000000000000000000000000000000000000000000000000000000000000000AED16A4A, 0x0000000000000000000000000000000000000000000000000000000000000000000000000D9D65ADC, 0x000000000000000000000000000000000000000000000000000000000000000000000000040DF0B66, 0x000000000000000000000000000000000000000000000000000000000000000000000000037D83BF0, 
 /* 236 -- */ 0x0000000000000000000000000000000000000000000000000000000000000000000000000A9BCAE53, 0x0000000000000000000000000000000000000000000000000000000000000000000000000DEBB9EC5, 0x000000000000000000000000000000000000000000000000000000000000000000000000047B2CF7F, 0x000000000000000000000000000000000000000000000000000000000000000000000000030B5FFE9, 
 /* 240 -- */ 0x0000000000000000000000000000000000000000000000000000000000000000000000000BDBDF21C, 0x0000000000000000000000000000000000000000000000000000000000000000000000000CABAC28A, 0x000000000000000000000000000000000000000000000000000000000000000000000000053B39330, 0x000000000000000000000000000000000000000000000000000000000000000000000000024B4A3A6, 
 /* 244 -- */ 0x0000000000000000000000000000000000000000000000000000000000000000000000000BAD03605, 0x0000000000000000000000000000000000000000000000000000000000000000000000000CDD70693, 0x000000000000000000000000000000000000000000000000000000000000000000000000054DE5729, 0x000000000000000000000000000000000000000000000000000000000000000000000000023D967BF, 
 /* 248 -- */ 0x0000000000000000000000000000000000000000000000000000000000000000000000000B3667A2E, 0x0000000000000000000000000000000000000000000000000000000000000000000000000C4614AB8, 0x00000000000000000000000000000000000000000000000000000000000000000000000005D681B02, 0x00000000000000000000000000000000000000000000000000000000000000000000000002A6F2B94, 
 /* 252 -- */ 0x0000000000000000000000000000000000000000000000000000000000000000000000000B40BBE37, 0x0000000000000000000000000000000000000000000000000000000000000000000000000C30C8EA1, 0x00000000000000000000000000000000000000000000000000000000000000000000000005A05DF1B, 0x00000000000000000000000000000000000000000000000000000000000000000000000002D02EF8D
};

//int main ( int argc, char * argv[] )
unsigned long get_file_crc_general(char *filename)
{
	FILE *fin;	/* file we're reading into buffer	*/
	unsigned char *buffer; /* buffer we're working on	*/
	size_t i, j;	/* counters of bytes in buff	*/
	int	k;/*	generic integer	*/
	unsigned long crc;	/*	the CRC value being computed	*/
	if (( fin = fopen ( filename, "rb" )) == NULL )
	{
		fprintf ( stderr, "Cannot open %s\n", filename );
		return EXIT_FAILURE;
	}
	if (( buffer = (unsigned char *) malloc ( 32766 )) == NULL ) 
	{
		fprintf ( stderr, "Out of memory\n" );
		return EXIT_FAILURE;
	}
	/* preconditioning sets crc to an initial nonzero value */
	crc = 0xFFFFFFFF;
	for (;;)
	{
		i = fread ( buffer, 1, 32766, fin );
		if ( i == 0 )
		{
			if ( feof ( fin )) /* we're done, so show results */
			{
				/* postconditioning inverts the bits in CRC */ 
				crc = ~crc;
				/* now print the result */
				//printf ( "CRC-32 for %s is %X\n",filename, crc );
				return crc;
			}
			else	/* read another 32K of file */
				continue;
		}
		for ( j = 0; j < i; j ++ ) /* loop through the buffer */
		{
			k = ( crc ^ buffer[j] ) & 0x000000FFL;
			crc = (( crc >> 8 ) & 0x00FFFFFFL ) ^ crc32_table[k];
		}
	}
}
