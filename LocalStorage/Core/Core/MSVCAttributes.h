//
//  GNUCAttributes.h
//
//
//  Created by void on 4/11/15.
//  Copyright (c) 2015 Timur Losev. All rights reserved.
//

#pragma once

/*
* This function attribute informs the compiler that the function does not return.
* The compiler can then perform optimizations by removing the code that is never reached.
*/
#define _AttrNoReturn 

/*The attribute asserts that a call to a function never results in a C++ exception being propagated from the call into the caller.*/
#define _AttrNoThrow 

/*This function attribute specifies function parameters that are not supposed to be null pointers. This enables the compiler to generate a warning on encountering such a parameter.*/
#define _AttrNonNull1 
#define _AttrNonNull2 
#define _AttrNonNull3 
#define _AttrNonNull4 
#define _AttrNonNull5 
#define _AttrNonNull6 
#define _AttrNonNull7 
#define _AttrNonNullAll 

/*This function attribute generates a warning if the specified parameter in a function call is not NULL.*/
#define _AttrSentinel 

/*
Many functions examine only the arguments passed to them, and have no effects except for the return value.
This is a much stricter class than __attribute__((pure)), because a function is not permitted to read global memory.
If a function is known to operate only on its arguments then it can be subject to common sub-expression elimination
and loop optimizations.
*/
#define _AttrConst 

/*Pure functions are candidates for common subexpression elimination.*/
#define _AttrPure 

/*
The warn_unused_result attribute causes a warning to be emitted if a caller of
the function with this attribute does not use its return value. This is useful
for functions where not checking the result is either a security problem or
always a bug, such as realloc.
*/
#define _AttrWarnUnusedResult 

/*
The malloc attribute is used to tell the compiler that a function may be treated as malloc
if any non-NULL pointer it returns cannot alias any other pointer valid when the function returns.
This will often improve optimization. Standard functions with this property include malloc and calloc.
realloc-like functions have this property as long as the old pointer is never referred
to (including comparing it to the new pointer) after the function returns a non-NULL value.
*/
#define _AttrMalloc 

/*
This function attribute indicates that a function must be inlined.

The compiler attempts to inline the function, regardless of the characteristics
of the function. However, the compiler does not inline a function if doing so causes problems.
For example, a recursive function is inlined into itself only once.
*/
#define _AttrAlwaysInline __forceinline

/*This function attribute suppresses the inlining of a function at the call points of the function.*/
#define _AttrNoInline 

/*
This attribute, attached to a function, means that code must be emitted for the function even
if it appears that the function is not referenced.
This is useful, for example, when the function is referenced only in inline assembly.
*/
#define _AttrUsed 

/*
This attribute, attached to a function, means that the function is meant to be
possibly unused. GCC will not produce a warning for this function.
*/

#define _AttrUnused 

/*
The weak attribute causes the declaration to be emitted as a weak symbol rather than a global.
This is primarily useful in defining library functions which can be overridden in user code,
though it can also be used with non-function declarations.
Weak symbols are supported for ELF targets, and also for a.out targets when using the GNU assembler and linker.
*/
#define _AttrWeak

#if __clang__ && __clang_major__ < 3
#undef _AttrNonNullAll
#define _AttrNonNullAll
#endif

#define _AttrFormat(stdf, ...) 

#define _AttrConstructor 

#define PBBuiltInConstant(x) 0

#ifdef __cplusplus
extern "C"
#endif
void * _ReturnAddress(void);

#pragma intrinsic(_ReturnAddress)

#define FReturnAddress(_level) _ReturnAddress()