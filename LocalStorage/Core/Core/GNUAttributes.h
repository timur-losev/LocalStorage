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
#define _AttrNoReturn __attribute__((__noreturn__))

/*The attribute asserts that a call to a function never results in a C++ exception being propagated from the call into the caller.*/
#define _AttrNoThrow __attribute__((__nothrow__))

/*This function attribute specifies function parameters that are not supposed to be null pointers. This enables the compiler to generate a warning on encountering such a parameter.*/
#define _AttrNonNull1 __attribute__((__nonnull__(1)))
#define _AttrNonNull2 __attribute__((__nonnull__(2)))
#define _AttrNonNull3 __attribute__((__nonnull__(3)))
#define _AttrNonNull4 __attribute__((__nonnull__(4)))
#define _AttrNonNull5 __attribute__((__nonnull__(5)))
#define _AttrNonNull6 __attribute__((__nonnull__(6)))
#define _AttrNonNull7 __attribute__((__nonnull__(7)))
#define _AttrNonNullAll __attribute__((__nonnull__))

/*This function attribute generates a warning if the specified parameter in a function call is not NULL.*/
#define _AttrSentinel __attribute__((__sentinel__))

/*
 Many functions examine only the arguments passed to them, and have no effects except for the return value.
 This is a much stricter class than __attribute__((pure)), because a function is not permitted to read global memory.
 If a function is known to operate only on its arguments then it can be subject to common sub-expression elimination
 and loop optimizations.
 */
#define _AttrConst __attribute__((__const__))

/*Pure functions are candidates for common subexpression elimination.*/
#define _AttrPure __attribute__((__pure__))

/*
 The warn_unused_result attribute causes a warning to be emitted if a caller of
 the function with this attribute does not use its return value. This is useful
 for functions where not checking the result is either a security problem or
 always a bug, such as realloc.
 */
#define _AttrWarnUnusedResult __attribute__((__warn_unused_result__))

/*
 The malloc attribute is used to tell the compiler that a function may be treated as malloc
 if any non-NULL pointer it returns cannot alias any other pointer valid when the function returns.
 This will often improve optimization. Standard functions with this property include malloc and calloc.
 realloc-like functions have this property as long as the old pointer is never referred
 to (including comparing it to the new pointer) after the function returns a non-NULL value.
 */
#define _AttrMalloc __attribute__((__malloc__))

/*
 This function attribute indicates that a function must be inlined.

 The compiler attempts to inline the function, regardless of the characteristics
 of the function. However, the compiler does not inline a function if doing so causes problems.
 For example, a recursive function is inlined into itself only once.
 */
#define _AttrAlwaysInline __attribute__((__always_inline__)) __inline__

/*This function attribute suppresses the inlining of a function at the call points of the function.*/
#define _AttrNoInline __attribute__((__noinline__))

/*
 This attribute, attached to a function, means that code must be emitted for the function even
 if it appears that the function is not referenced.
 This is useful, for example, when the function is referenced only in inline assembly.
 */
#define _AttrUsed __attribute__((__used__))

/*
 This attribute, attached to a function, means that the function is meant to be
 possibly unused. GCC will not produce a warning for this function.
 */

#define _AttrUnused __attribute__((__unused__))

/*
 The weak attribute causes the declaration to be emitted as a weak symbol rather than a global.
 This is primarily useful in defining library functions which can be overridden in user code,
 though it can also be used with non-function declarations.
 Weak symbols are supported for ELF targets, and also for a.out targets when using the GNU assembler and linker.
 */
#define _AttrWeak __attribute__((__weak__))

#if __clang__ && __clang_major__ < 3
#undef _AttrNonNullAll
#define _AttrNonNullAll
#endif

#define _AttrFormat(stdf, ...) __attribute__((__format__(stdf, __VA_ARGS__)))

#define _AttrConstructor __attribute__((constructor))

#define FBuiltInConstant(x) __builtin_constant_p((x))
#define FReturnAddress(_level) __builtin_return_address(_level)