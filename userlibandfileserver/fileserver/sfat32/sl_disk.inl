// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfat\inc\sl_disk.inl
// 
//

/**
 @file
 @internalTechnology
*/

#if !defined(__SL_DISK_INL__)
#define __SL_DISK_INL__


//---------------------------------------------------------------------------------------------------------------------------------
/**
    @return pointer to the directory cache interface
*/
MWTCacheInterface* CAtaDisk::DirCacheInterface() 
    {
        return ipDirCache;
    }






#endif //__SL_DISK_INL__





















