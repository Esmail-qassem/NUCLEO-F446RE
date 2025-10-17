/***********************************************************************/
/*Aauthor     : Esamil Qassem                                          */
/*Data        : 17 Nov 2023                                            */
/*Version     : V.1                                                    */                 
/***********************************************************************/

#ifndef STD_TYPES_H_
#define STD_TYPES_H_

typedef enum
{
	E_Ok,
	E_Not_Ok,
	E_Null_Pointer,
	E_TIME_OUT
}Status_t;

enum Ability
{
	Disable,
	Enable
};
typedef signed char sint8;
typedef signed short int sint16;
typedef signed long int sint32;
typedef unsigned char uint8;
typedef unsigned short int uint16;
typedef unsigned long int uint32;
typedef float float32;
typedef double float64;
typedef long double float128;
typedef unsigned long long int uint128;
typedef signed long long int sint128;
#define NULL 0
#endif
