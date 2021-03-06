// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// f32test\math\t_vals.h
// SPECVALS.H - special values for TReal32 and TReal64 tests
// Generated by \E32\TMATH\T_GEN.MAK 
// 
//


#include <e32std.h>

class TInt32x
	{
public:
	TInt32x(TInt32 aInt) {iInt=aInt;}
public:
	TInt32 iInt;
	};

#if defined (__DOUBLE_WORDS_SWAPPED__)

// for ARM (big-endian doubles)

const TInt64 minTReal32in64 = MAKE_TINT64(0x0,0x38100000);
const TInt64 maxTReal32in64 = MAKE_TINT64(0xe0000000,0x47efffff);
const TInt64 sqrtMaxTReal64 = MAKE_TINT64(0xffd9c605,0x5fefffff);
const TInt64 sqrtMinTReal64 = MAKE_TINT64(0x0,0x20000000);
const TInt64 negZeroTReal64 = MAKE_TINT64(0x0,0x80000000);
const TInt64 posInfTReal64 = MAKE_TINT64(0x0,0x7ff00000);
const TInt64 negInfTReal64 = MAKE_TINT64(0x0,0xfff00000);
const TInt64 NaNTReal64 = MAKE_TINT64(0xffffffff,0x7fffffff);
const TInt32x sqrtMaxTReal32 = TInt32x(0x5f7ffffd);
const TInt32x sqrtMinTReal32 = TInt32x(0x20000000);
const TInt32x negZeroTReal32 = TInt32x(0x80000000);
const TReal64 KMinTReal32in64 = *(TReal64*)&minTReal32in64;
const TReal64 KMaxTReal32in64 = *(TReal64*)&maxTReal32in64;
const TReal64 KSqrtMaxTReal64 = *(TReal64*)&sqrtMaxTReal64;
const TReal64 KSqrtMinTReal64 = *(TReal64*)&sqrtMinTReal64;
const TReal64 KNegZeroTReal64 = *(TReal64*)&negZeroTReal64;
const TReal64 KPosInfTReal64 = *(TReal64*)&posInfTReal64;
const TReal64 KNegInfTReal64 = *(TReal64*)&negInfTReal64;
const TReal64 KNaNTReal64 = *(TReal64*)&NaNTReal64;
const TReal32 KSqrtMaxTReal32 = *(TReal32*)&sqrtMaxTReal32;
const TReal32 KSqrtMinTReal32 = *(TReal32*)&sqrtMinTReal32;
const TReal32 KNegZeroTReal32 = *(TReal32*)&negZeroTReal32;
const TReal KMinTReal32inTReal = TReal(KMinTReal32in64);
const TReal KMaxTReal32inTReal = TReal(KMaxTReal32in64);
const TReal KNegZeroTReal = TReal(KNegZeroTReal64);

#else	// NOT #if defined (__DOUBLE_WORDS_SWAPPED__)

// for WINS and X86 (little-endian doubles) 

const TInt64 minTReal32in64 = MAKE_TINT64(0x38100000,0x0);
const TInt64 maxTReal32in64 = MAKE_TINT64(0x47efffff,0xe0000000);
const TInt64 sqrtMaxTReal64 = MAKE_TINT64(0x5fefffff,0xffd9c605);
const TInt64 sqrtMinTReal64 = MAKE_TINT64(0x20000000,0x0);
const TInt64 negZeroTReal64 = MAKE_TINT64(0x80000000,0x0);
const TInt64 posInfTReal64 = MAKE_TINT64(0x7ff00000,0x0);
const TInt64 negInfTReal64 = MAKE_TINT64(0xfff00000,0x0);
const TInt64 NaNTReal64 = MAKE_TINT64(0x7fffffff,0xffffffff);
const TInt32x sqrtMaxTReal32 = TInt32x(0x5f7ffffd);
const TInt32x sqrtMinTReal32 = TInt32x(0x20000000);
const TInt32x negZeroTReal32 = TInt32x(0x80000000);
const TReal64 KMinTReal32in64 = *(TReal64*)&minTReal32in64;
const TReal64 KMaxTReal32in64 = *(TReal64*)&maxTReal32in64;
const TReal64 KSqrtMaxTReal64 = *(TReal64*)&sqrtMaxTReal64;
const TReal64 KSqrtMinTReal64 = *(TReal64*)&sqrtMinTReal64;
const TReal64 KNegZeroTReal64 = *(TReal64*)&negZeroTReal64;
const TReal64 KPosInfTReal64 = *(TReal64*)&posInfTReal64;
const TReal64 KNegInfTReal64 = *(TReal64*)&negInfTReal64;
const TReal64 KNaNTReal64 = *(TReal64*)&NaNTReal64;
const TReal32 KSqrtMaxTReal32 = *(TReal32*)&sqrtMaxTReal32;
const TReal32 KSqrtMinTReal32 = *(TReal32*)&sqrtMinTReal32;
const TReal32 KNegZeroTReal32 = *(TReal32*)&negZeroTReal32;
const TReal KMinTReal32inTReal = TReal(KMinTReal32in64);
const TReal KMaxTReal32inTReal = TReal(KMaxTReal32in64);
const TReal KNegZeroTReal = TReal(KNegZeroTReal64);

#endif
