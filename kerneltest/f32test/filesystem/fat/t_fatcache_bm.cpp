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
// f32test\server\t_mount.cpp
// Testing FAT cache performance
//
//

/**
 @file
*/

#define __E32TEST_EXTENSION__

#include <f32file.h>
#include <e32test.h>
#include <e32math.h>
#include <e32property.h>

#include "t_server.h"
//#include "t_std.h"

#include "fat_utils.h"

using namespace Fat_Test_Utils;

#ifdef __VC32__
    // Solve compilation problem caused by non-English locale
    #pragma setlocale("english")
#endif

RTest test(_L("T_FAT_Cache_BM"));

//-- note that this test disables all FAT mount enhancements, like asynchronous mounting and using FSInfo sector.

static void WaitForFatBkGndActivityEnd();

//-------------------------------------------------------------------
//-- debug bit flags that may be set in the property which controls FAT volume mounting

const TUid KThisTestSID={0x10210EB3}; ///< this EXE SID

//const TUint32 KMntProp_EnableALL    = 0x00000000; //-- enable all operations
const TUint32 KMntProp_DisableALL   = 0xFFFFFFFF; //-- disable all operations

//const TUint32 KMntProp_Disable_FsInfo       = 0x00000001; //-- mask for disabling/enabling FSInfo information
//const TUint32 KMntProp_Disable_FatBkGndScan = 0x00000002; //-- mask for disabling/enabling FAT background scanner

//-------------------------------------------------------------------

static TInt     gDriveNum=-1; ///< drive number we are dealing with
static TInt64   gRndSeed;
static TFatBootSector gBootSector;

static void DoRemountFS(TInt aDrive);

//-- Array of integer file tars. Tags are used to identify files (the file name is generated by KFnTemplate template)
typedef CArrayFixFlat<TInt> CFileTagsArray;
static CFileTagsArray *pFTagArray = NULL;


//-------------------------------------------------------------------
const TInt KMaxFiles = 1000; //-- maximal number of files to create

//-- number of unfragmented files that will be left, other files will be merged to bigger ones.
const TInt KUnfragmentedFilesLeave = KMaxFiles/10;

_LIT(KDirName, "\\DIR1\\");  //-- directory name we a working with (FAT12/16 won't allow KMaxFiles in the root dir.)
_LIT(KFirstFileName, "\\First_file_1.nul"); //-- the name of the first file on the volume

//-------------------------------------------------------------------

/**
    Make a file name by its numeric tag
    @param aDes here will be a file name
    @param aFileNameTag numeric tag for the file name generation

*/
void MakeFileName(TDes& aDes, TInt aFileNameTag)
{
    _LIT(KFnTemplate, "F%d.NUL");//-- file name template, use 8.3 names here in order not to stress dir. cache much.
    aDes.Copy(KDirName);
    aDes.AppendFormat(KFnTemplate, aFileNameTag);
}

//-------------------------------------------------------------------
/**
    format the volume and read the boot sector
*/
static void FormatVolume(TBool aQuickFormat)
{
    (void)aQuickFormat;

    #ifndef  __EPOC32__
    test.Printf(_L("This is emulator configuration!!!!\n"));

    //-- FAT32 SPC:1; for the FAT32 testing on the emulator
    //TFatFormatParam fp;
    //fp.iFatType = EFat32;
    //fp.iSecPerCluster = 1;
    //FormatFatDrive(TheFs, CurrentDrive(), ETrue, &fp); //-- always quick; doesn't matter for the emulator

    aQuickFormat = ETrue;
    #endif


    FormatFatDrive(TheFs, CurrentDrive(), aQuickFormat);


    TInt nRes = ReadBootSector(TheFs, gDriveNum, 0x00, gBootSector);
    test_KErrNone(nRes);
}


//-------------------------------------------------------------------

/**
    Helper method. Does one iteration of merging test files into one fragmented one.
    See DoMergeFiles()

    @param  aFTagArray  reference to the file tags array
    @param  aBigFileNo  a sequential number of the result file to be merged from random number of smaller ones (names are taken from the tag array)
    @param  aTimeTaken_us  on return will contain time taken to this operation, in microseconds

    @return number of files merged into one.
*/
TInt DoMergeTestFiles(CFileTagsArray& aFTagArray, TInt aBigFileNo, TInt64&  aTimeTaken_us)
{

    aTimeTaken_us = 0;
    TTime   timeStart;
    TTime   timeEnd;


    //-- merged files' names start with this number
    const TInt KMergedFileFnThreshold = 20000;

    const TInt KMaxMergedFiles = 20;
    const TInt nFilesToMerge = Max((Math::Rand(gRndSeed) % KMaxMergedFiles), 2); //-- how many files to merge into one

    test(aFTagArray.Count() > KMaxMergedFiles);

    TInt selectedFTags[KMaxMergedFiles]; //-- here we will store file tags to be merged to one large fragmented file
    TInt i;
    TInt nRes;

    i=0;
    do
    {
        //-- randomly select a file tag from the global array
        const TInt index = (TUint)Math::Rand(gRndSeed) % aFTagArray.Count();
        const TInt fnTag = aFTagArray[index];

        if(fnTag < 0 || //-- no such file, already deleted
           fnTag >= KMergedFileFnThreshold) //-- this is a big, already merged file
           continue;


        selectedFTags[i] = fnTag;
        aFTagArray.Delete(index);

        ++i;
    }while(i<nFilesToMerge);

    //-- here we have file tags in selectedFNumbers array. delete these files and create one big file
    //-- with the equal size. This file will be perfectly fragmented.
    //-- search for FAT entries had priority to the right, so delete files in reverse order.

    TUint32 newFileSize = 0;
    TBuf<128> buf;
    RFile file;

    timeStart.UniversalTime(); //-- take start time

    for(i=0; i<nFilesToMerge; ++i)
    {
        MakeFileName(buf, selectedFTags[nFilesToMerge-1-i]);

        nRes=file.Open(TheFs, buf, EFileRead);
        test_KErrNone(nRes);

        TInt nSize;
        nRes = file.Size(nSize);
        test_KErrNone(nRes);
        file.Close();

        newFileSize+=(TUint32)nSize; //-- this will be the size of the new file

        //-- delete small file
        nRes = TheFs.Delete(buf);
        test_KErrNone(nRes);

    }

    //-- create a large file taking the place of few smaller. It will be framented.
    const TInt bigFileTag = aBigFileNo+KMergedFileFnThreshold;

    MakeFileName(buf, bigFileTag);
    nRes = CreateEmptyFile(TheFs, buf, newFileSize);
    test_KErrNone(nRes);

    timeEnd.UniversalTime(); //-- take end time
    aTimeTaken_us = (timeEnd.MicroSecondsFrom(timeStart)).Int64();

    //-- put new big file name tag to the global array in order not to forget it for later use.
    aFTagArray.AppendL(bigFileTag);

    return nFilesToMerge;
}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_FATCACHE_BM-0689
//! @SYMTestType        PT
//! @SYMPREQ            PREQ1721
//! @SYMTestCaseDesc    randomly deletes small files and creates larger. Measures time taken by this FAT fragmentation
//!
//! @SYMTestActions
//!                     0   delete a random number of small files
//!                     1   create a random number of larger files, measure time taken
//!
//! @SYMTestExpectedResults successful files deletion/creation
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------

/**
    precondition: We have a lot of (KMaxFiles) files of the same size on the volume. They occupy almost all FAT.
    This method randomly selects several files on the volume, deletes them and creates a bigger file that occupies space from the
    deleted files. So that we got a fragmented big file. Repeat this until we have KUnfragmentedFilesLeave initial files left on the
    volume.
*/
void DoMergeFiles(CFileTagsArray& aFTagArray)
{
    test.Next(_L("Merging small files to larger creating FAT fragmentation...\n"));

    TInt64  usTotalTime=0;
    TInt64  usCurrTime;

    TInt nUnfragmentedLeft = aFTagArray.Count();
    for(TInt i=0; nUnfragmentedLeft > KUnfragmentedFilesLeave; ++i)
    {
        nUnfragmentedLeft -= DoMergeTestFiles(aFTagArray, i, usCurrTime);
        usTotalTime += usCurrTime;
    }

    test.Printf(_L("#--> Merging files :%d ms\n"), (TUint32)usTotalTime/K1mSec);
}

//-------------------------------------------------------------------
/**
    Randomly shuffles entries in file name tags array.
*/
void ShuffleArray(CFileTagsArray& aFTagArray)
{
    const TInt KSwapIterations = 500;
    const TInt arrSize = aFTagArray.Count();


    for(TInt i = 0; i<KSwapIterations; ++i)
    {//-- randomly swap 2 elements in the array
        const TInt idx1 = (TUint)Math::Rand(gRndSeed) % arrSize;
        const TInt idx2 = (TUint)Math::Rand(gRndSeed) % arrSize;

        TInt tmp = aFTagArray[idx1];
        aFTagArray[idx1] = aFTagArray[idx2];;
        aFTagArray[idx2] = tmp;
    }

}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_FATCACHE_BM-0690
//! @SYMTestType        PT
//! @SYMPREQ            PREQ1721
//! @SYMTestCaseDesc    Measure open and seek time for all files that have tags in aFTagArray.
//!
//! @SYMTestActions
//!                     0   remount FS
//!                     1   open all files, that have tags in aFTagArray, measure time taken
//!                     2   seek to the each file end, measure time taken
//!
//! @SYMTestExpectedResults successful files open/seek
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
void MeasureSeekTime(CFileTagsArray& aFTagArray)
{
    test.Next(_L("Measuring seek time...\n"));

    TInt nRes;
    RFile file;
    TBuf<128> buf;

    TTime   timeStart;
    TTime   timeEnd;
    TInt64  usTotalTimeSeek=0;
    TInt64  usTotalTimeOpen=0;

    const TInt KNumRepetitions = 10;

    for(TInt repCnt=0; repCnt<KNumRepetitions; ++repCnt)
    {
        //-- remount file system, reset caches
        DoRemountFS(gDriveNum);

        for(TInt i = 0; i<aFTagArray.Count(); ++i)
        {
            MakeFileName(buf, aFTagArray[i]);

            //-- 1. open the file
            timeStart.UniversalTime(); //-- take start time

                nRes = file.Open(TheFs, buf, EFileRead);

            timeEnd.UniversalTime(); //-- take end time
            usTotalTimeOpen += (timeEnd.MicroSecondsFrom(timeStart)).Int64();

            test_KErrNone(nRes);


            //-- 2. goto the end of the file. This operation shall traverse whole file's cluster chain in FAT
            timeStart.UniversalTime(); //-- take start time

                TInt seekPos = 0;
                nRes = file.Seek(ESeekEnd, seekPos);

            timeEnd.UniversalTime(); //-- take end time
            usTotalTimeSeek += (timeEnd.MicroSecondsFrom(timeStart)).Int64();

            test_KErrNone(nRes);
            file.Close();
        }
    }

    test.Printf(_L("#--> Total open time for %d files is %d ms\n"), aFTagArray.Count(), (TUint32)usTotalTimeOpen/(K1mSec*KNumRepetitions));
    test.Printf(_L("#--> Total seek time for %d files is %d ms\n"), aFTagArray.Count(), (TUint32)usTotalTimeSeek/(K1mSec*KNumRepetitions));
}


//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_FATCACHE_BM-0692
//! @SYMTestType        PT
//! @SYMPREQ            PREQ1721
//! @SYMTestCaseDesc    Detetes all files that have name tags in name tags array and measures time taken.
//!
//! @SYMTestActions
//!                     0   remount the FS
//!                     1   delete files and measure time taken.
//!
//! @SYMTestExpectedResults successful creating files
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
void DeleteAllFiles(CFileTagsArray& aFTagArray)
{
    TInt nRes;
    TBuf<128> buf;

    TTime   timeStart;
    TTime   timeEnd;
    TInt64  usTotalTime=0;

    test.Next(_L("Deleting all files...\n"));

    //-- remount file system, reset caches
    DoRemountFS(gDriveNum);

    for(TInt i = 0; i<aFTagArray.Count(); ++i)
    {
        MakeFileName(buf, aFTagArray[i]);

        timeStart.UniversalTime(); //-- take start time

        nRes = TheFs.Delete(buf);

        timeEnd.UniversalTime(); //-- take end time
        usTotalTime += (timeEnd.MicroSecondsFrom(timeStart)).Int64();

        test_KErrNone(nRes);
    }

    test.Printf(_L("#--> Deleted %d files in %d ms\n"), aFTagArray.Count(), (TUint32)usTotalTime/K1mSec);
}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_FATCACHE_BM-0687
//! @SYMTestType        PT
//! @SYMPREQ            PREQ1721
//! @SYMTestCaseDesc    Create KMaxFiles files to fill in space in FAT table and measure time taken.
//!
//! @SYMTestActions
//!                     0   Create KMaxFiles files and measure time taken.
//!
//! @SYMTestExpectedResults successful creating files
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
/**
    Create KMaxFiles files to fill in space in FAT table and measure time taken.
    @return EFalse if it is impossible to create test files
*/
TBool DoCreateFiles(CFileTagsArray& aFTagArray)
{
    TInt nRes;
    TBuf<128> buf;

    TTime   timeStart;
    TTime   timeEnd;
    TInt64  usTotalTime=0;

    test.Next(_L("Creating many files\n"));
    test.Printf(_L("Number of files:%d\n"), KMaxFiles);

    aFTagArray.Reset();

    TVolumeInfo volInfo;
    nRes = TheFs.Volume(volInfo, gDriveNum);
    test_KErrNone(nRes);


    test(gBootSector.IsValid());

    const TUint32 clustSz = gBootSector.SectorsPerCluster() * gBootSector.BytesPerSector();
    const TUint32 maxClusters = (TUint32)(volInfo.iFree / clustSz);

    if(KMaxFiles > 0.8 * maxClusters)
    {
        test.Printf(_L("Can't create %d files; skipping the test!\n"), KMaxFiles);
        return EFalse;
    }

    //-- adjust file size for very small volumes
    TUint32 fillFileSz = (maxClusters/KMaxFiles)*clustSz;
    if(fillFileSz*KMaxFiles >= 0.8*volInfo.iFree) //-- take into account the size of the directory with 1000 files.
    {
        if(fillFileSz <= clustSz)
        {
            test.Printf(_L("Can't create %d files; skipping the test!\n"), KMaxFiles);
            return EFalse;
        }

        fillFileSz -= clustSz;
    }

    //-- create the first file on the volume. It will be deleted then in order to create 1 free FAT entry
    //-- in the very beginnng of the FAT. So, the size of this file shan't be more that 1 sector/cluster.
    nRes = CreateEmptyFile(TheFs, KFirstFileName, 100);
    test_KErrNone(nRes);


    //-- to avoid affected timings in UREL mode.
    WaitForFatBkGndActivityEnd();

    //-- disable FAT test utils print out, it affects measured time when we create empty files
    EnablePrintOutput(EFalse);

    //-- create KMaxFiles files on the volume
    for(TInt i=0; i<KMaxFiles; ++i)
    {
        //-- create empty file
        MakeFileName(buf, i);

        timeStart.UniversalTime(); //-- take start time

        nRes = CreateEmptyFile(TheFs, buf, fillFileSz);

        timeEnd.UniversalTime(); //-- take end time
        usTotalTime += (timeEnd.MicroSecondsFrom(timeStart)).Int64();

        test_KErrNone(nRes);
        //-- put its id number to the array.
        aFTagArray.AppendL(i);

        if((i % 100) == 0)
            test.Printf(_L("*"));

    }

    EnablePrintOutput(ETrue); //-- Enable FAT test utils print out

    test.Printf(_L("\n#--> Created %d files in %d ms\n"), KMaxFiles, (TUint32)usTotalTime/K1mSec);

    return ETrue;
}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_FATCACHE_BM-0691
//! @SYMTestType        PT
//! @SYMPREQ            PREQ1721
//! @SYMTestCaseDesc    Check that all FAT copies are the same.
//!
//! @SYMTestActions
//!                     0   read all available FAT copies and compare them
//!
//! @SYMTestExpectedResults all FAT copies on the vollume must be the same.
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
void CheckFatCopies()
{
    test.Next(_L("Comparing FATs...\n"));

    TFatBootSector bootSec;

    TInt nRes = ReadBootSector(TheFs, gDriveNum, 0x00, bootSec);
    test_KErrNone(nRes);

    const TInt numFATs = bootSec.NumberOfFats();
    if(numFATs < 2)
    {//-- only one FAT, nothing to compare with.
        test.Printf(_L("The volume has only 1 FAT. Nothing to do.\n"));
        return;
    }

    const TUint32 bytesPerSec = bootSec.BytesPerSector();
    const TUint32 posFat1Start =  bootSec.FirstFatSector() * bytesPerSec;
    const TUint32 fatSize = bootSec.TotalFatSectors() * bytesPerSec;

    RBuf8 fatBuf1;
    RBuf8 fatBuf2;

    nRes = fatBuf1.CreateMax(bytesPerSec);
    test_KErrNone(nRes);

    nRes = fatBuf2.CreateMax(bytesPerSec);
    test_KErrNone(nRes);

    //-- read FAT sector by sector comparing all copies
    TUint32 currPos = posFat1Start;
    for(TUint cntSectors=0; cntSectors<bootSec.TotalFatSectors(); ++cntSectors)
    {
        //-- read a sector of FAT#0
        nRes = MediaRawRead(TheFs, gDriveNum, currPos, bytesPerSec, fatBuf1);
        test_KErrNone(nRes);

        //-- read the same sector from other copies of FAT and compare with FAT#0
        for(TInt currFat=1; currFat<numFATs; ++currFat)
        {
            nRes = MediaRawRead(TheFs, gDriveNum, (currFat*fatSize + currPos), bytesPerSec, fatBuf2);
            test_KErrNone(nRes);

            //-- compare the buffer to FAT#0
            if(fatBuf1.CompareF(fatBuf2) !=0)
            {//-- current FAT is different from FAT0
             test.Printf(_L("FAT#%d differs from FAT#0! FAT sector:%d, media Sector:%d\n"), currFat, cntSectors, cntSectors+bootSec.FirstFatSector());
             test(0);
            }

        }


        currPos+=bytesPerSec;
    }


    fatBuf1.Close();
    fatBuf2.Close();

}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_FATCACHE_BM-0688
//! @SYMTestType        PT
//! @SYMPREQ            PREQ1721
//! @SYMTestCaseDesc    Create the file whose FAT entries will go to the end of the FAT table and measure the time taken.
//!
//! @SYMTestActions
//!                     0   delete a file in the very beginning of the FAT
//!                     1   remount file system
//!                     2   create a larger file finishing at the end of FAT, measure time taken
//!
//! @SYMTestExpectedResults successful files creation
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------

/**
    Create the file whose FAT entries will go to the end of the FAT table and measure the time taken.
    preconditions:
    1. The FAT table must be almost full already. This is done previously in DoCreateFiles().
    2. There shall be a small file in the very beginning of the FAT which this test deletes and remounts the FS.
       this will cause "the last known free cluster"  in the FAT to be set to the beginning of the FAT.

    Then when we create a larger file, whole occupied FAT region will be scanned for the free cluster.
*/
void CreateLastFile()
{
    test.Next(_L("Create a file in the end of FAT\n"));

    TTime   timeStart;
    TTime   timeEnd;
    TInt64  usTotalTime=0;

    TInt nRes;


    //-- 1. delete the first file on the volume (see KFirstFileName & DoCreateFiles()
    //-- it will create free FAT entry in the very beginning.
    nRes = TheFs.Delete(KFirstFileName);
    test_KErrNone(nRes);


    //-- 2. remount file system, reset caches etc. The first known free cluster will be number 2 or 3, because of the point 1.
    //-- the rest of the FAT is filled, because we've created plenty of files in the DoCreateFiles()
    DoRemountFS(gDriveNum);

    //-- 3. create a file that will occupy more that 2 clusters; the 1st cluster of it will be in the very beginning of the FAT,
    //-- and the rest will be in the very end of the FAT. Measure the time taken to walk all FAT when searching free entry.
    _LIT(KLastFn, "\\last-last.file");

    test(gBootSector.IsValid());

    const TUint32 clustSz = gBootSector.SectorsPerCluster() * gBootSector.BytesPerSector();


    //-- disable FAT test utils print out, it affects measured time
    EnablePrintOutput(EFalse);

    timeStart.UniversalTime(); //-- take start time

    //-- create an empty file, it is supposed to be placed in the very end of FAT (FAT table is almost full because of the
    //-- previous test)
    nRes = CreateEmptyFile(TheFs, KLastFn, 7*clustSz);

    timeEnd.UniversalTime(); //-- take end time
    usTotalTime = (timeEnd.MicroSecondsFrom(timeStart)).Int64();

    test_KErrNone(nRes);

    test.Printf(_L("#--> file at the end of FAT created in %d ms\n"), (TUint32)usTotalTime/K1mSec);

    //-- delete the file
    nRes = TheFs.Delete(KLastFn);
    test_KErrNone(nRes);

    EnablePrintOutput(ETrue); //-- Enable FAT test utils print out
}

//-------------------------------------------------------------------
/**
    Create 100 directories in the root and measure time
*/
void DoCreateDirsInRoot()
{
    test.Next(_L("Measure time to create many directories in the Root.\n"));

    if(!Is_Fat32(TheFs, gDriveNum))
    {
        test.Printf(_L("This test requires FAT32, skipping\n"));
        return;
    }

    TTime   timeStart;
    TTime   timeEnd;
    TInt64  usTotalTime=0;
    TFileName dirName;

    //-- remount file system, reset caches etc. The first known free cluster will be number 2 or 3
    DoRemountFS(gDriveNum);

    //-- disable FAT test utils print out, it affects measured time
    EnablePrintOutput(EFalse);

    timeStart.UniversalTime(); //-- take start time

    //-- create some subdirectories in the root dir and measure time
    const TInt KMaxDirs = 100;
    for(TInt i=0; i<KMaxDirs; ++i)
    {
        dirName.Format(_L("\\directory%04d\\"), i);
        TInt nRes = TheFs.MkDir(dirName);
        test_KErrNone(nRes);
    }

    timeEnd.UniversalTime(); //-- take end time
    usTotalTime = (timeEnd.MicroSecondsFrom(timeStart)).Int64();

    test.Printf(_L("#--> %d Dirs. created in %d ms\n"), KMaxDirs, (TUint32)usTotalTime/K1mSec);

    EnablePrintOutput(ETrue); //-- Enable FAT test utils print out
}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_FATCACHE_BM-0693
//! @SYMTestType        PT
//! @SYMPREQ            PREQ1721
//! @SYMTestCaseDesc    Create a large file (2G max) and measure the time. Then delete this file and measure time taken
//!
//! @SYMTestActions
//!                     0   quick format the volume
//!                     1   create emply file that takes either 80% of the volume of 2G max, masure time taken
//!                     2   remount FS
//!                     3   delete this file and measure time taken
//!
//! @SYMTestExpectedResults successful files creation/deletion
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
static void CreateLargeFile()
{
    test.Next(_L("Create a large file and measure time\n"));

    FormatVolume(ETrue); //-- quick format the volume.

    _LIT(KBigFileName, "\\BigFile.big");

    test(gBootSector.IsValid());

    //-- calculate the size of the file, it shall be max 2G or take almost all volume
    TVolumeInfo volInfo;
    TInt nRes;

    nRes = TheFs.Volume(volInfo, gDriveNum);
    test_KErrNone(nRes);

    const TUint32 clustSz = gBootSector.SectorsPerCluster() * gBootSector.BytesPerSector();
    const TUint32 maxClusters = (TUint32)(volInfo.iFree / clustSz);

    const TUint32 clustersPer1G = K1GigaByte / clustSz;
    const TUint32 clustersPer2G = 2*clustersPer1G;

    TUint32 fileClusters=0;
    if(maxClusters*0.8 < clustersPer2G)
        fileClusters = (TUint32)(maxClusters*0.8);
    else
        fileClusters = (TUint32)(clustersPer2G*0.8);

    const TUint32 fileSize = fileClusters*clustSz;

    //-- create empty file and measure time
    TTime   timeStart;
    TTime   timeEnd;
    TInt64  usTimeCreate=0;
    TInt64  usTimeDelete=0;

    timeStart.UniversalTime(); //-- take start time
    nRes = CreateEmptyFile(TheFs, KBigFileName, fileSize);
    timeEnd.UniversalTime(); //-- take end time
    test_KErrNone(nRes);

    usTimeCreate = (timeEnd.MicroSecondsFrom(timeStart)).Int64();

    //-- remount file system, reset caches etc.
    DoRemountFS(gDriveNum);

    //-- delete the file
    timeStart.UniversalTime(); //-- take start time
    nRes = TheFs.Delete(KBigFileName);
    timeEnd.UniversalTime(); //-- take end time

    test_KErrNone(nRes);
    usTimeDelete = (timeEnd.MicroSecondsFrom(timeStart)).Int64();

    test.Printf(_L("#--> Big file sz:%u created:%d ms, deleted:%d ms\n"), fileSize, (TUint32)(usTimeCreate/K1mSec) , (TUint32)(usTimeDelete/K1mSec));

}


//-------------------------------------------------------------------
/**
    Start tests.
*/
void RunTest()
{
    test.Printf(_L("Prepare the volume for BM testing...\n"));

    test(pFTagArray != NULL);
    CFileTagsArray &fTagArray = *pFTagArray;
    fTagArray.Reset();

    //-- full format the drive
    FormatVolume(EFalse);

    test.Printf(_L("\n#--> t_fatcache_bm\n"));

    //-- create test directory.
    MakeDir(KDirName);

    //-- 1. create KMaxFiles files to fill in space in FAT table.
    if(!DoCreateFiles(fTagArray))
        return; //-- test is inconsistent


    //-- 1.1 create a file in the very end of the full volume (FAT table is almost full). Measure time taken
    CreateLastFile();

    //-- 1.2 create multiple directories in the root and measure time
//    DoCreateDirsInRoot();

    //-- 2. randomly merge some small files to bigger ones that will be fragmented
    DoMergeFiles(fTagArray);

    //-- 3. randomly shuffle file tags in the array
    ShuffleArray(fTagArray);

    //-- 4. measure file open and seek time
    MeasureSeekTime(fTagArray);

    //-- 4.5 Check that all copies of FAT are the same.
    CheckFatCopies();

    //-- 5. delete all files and print out time taken
    DeleteAllFiles(fTagArray);

    //-- 6. Create a large file (2G max) and measure the time
    //!!!!
    CreateLargeFile();

}



//-------------------------------------------------------------------
static void WaitForFatBkGndActivityEnd()
{
    //-- if we work in release mode, we need to use a hardcore solution to wait until possible FAT background activity finishes
    //-- because transient FAT threads can affect timings (e.g. creating a file may need waiting to FAT background thread to
    //-- parse some portion of FAT) etc.
    //-- for debug mode background FAT activity is disabled in InitGlobals() and timings are not precise anyway
    //-- for release mode we just need to wait some time
    #ifndef _DEBUG
    const TInt KWaitTimeSec = 10;
    test.Printf(_L("waiting %d sec...\n"), KWaitTimeSec);
    User::After(KWaitTimeSec*K1Sec);
    #endif

}



//-------------------------------------------------------------------

/**
    Dismounts and mounts the FS on a drive aDrive
    This will cause resetting "last known free cluster number" value in the FAT table implementation in FSY.
    (Mounting enhancements are disabled.) Empty the caches etc.
*/
static void DoRemountFS(TInt aDrive)
{
    TInt nRes = RemountFS(TheFs, aDrive);
    test_KErrNone(nRes);

    //-- get volume info, this is a trick that can help avoiding asynchronous FAT mount in UREL mode
    //TVolumeInfo v;
    //nRes = TheFs.Volume(v);

    //-- to avoid affected timings in UREL mode.
    WaitForFatBkGndActivityEnd();
}


//-------------------------------------------------------------------

/** initialise test global objects */
static void InitGlobals()
{
    //-- initialise random generator
    gRndSeed = 0x67fc1a9;

    //-- construct file numbers array
    pFTagArray = new CFileTagsArray(KMaxFiles);
    test(pFTagArray != NULL);
    pFTagArray->SetReserveL(KMaxFiles);

    //-- define a propery which will control mount process in the fsy.
    //-- The property key is a drive number being tested
    _LIT_SECURITY_POLICY_PASS(KTestPropPolicy);
    TInt nRes = RProperty::Define(KThisTestSID, gDriveNum, RProperty::EInt, KTestPropPolicy, KTestPropPolicy);
    test(nRes == KErrNone || nRes == KErrAlreadyExists);

    //-- disable all volume mount enhancements, like asynch mount and FSInfo.
    //-- this works only in debug mode.
    nRes = RProperty::Set(KThisTestSID, gDriveNum, (TInt)KMntProp_DisableALL);
    test_KErrNone(nRes);

}

/** destroy test global objects */
static void DestroyGlobals()
{
    delete pFTagArray;

    //-- delete test property
    RProperty::Delete(KThisTestSID, gDriveNum);

}


//-------------------------------------------------------------------
void CallTestsL()
    {

    //-- set up console output
    Fat_Test_Utils::SetConsole(test.Console());

    TInt nRes=TheFs.CharToDrive(gDriveToTest, gDriveNum);
    test(nRes==KErrNone);

    //-- check if this is FAT
    if(!Is_Fat(TheFs, gDriveNum))
    {
        test.Printf(_L("Skipping. This test requires FAT drive.\n"));
        return;
    }

    //-- check this is not the internal ram drive
    TVolumeInfo v;
    nRes = TheFs.Volume(v);
    test(nRes==KErrNone);
    if(v.iDrive.iMediaAtt & KMediaAttVariableSize)
        {
        test.Printf(_L("Skipping. Internal ram drive not tested.\n"));
        return;
        }


    //-------------------------------------
    PrintDrvInfo(TheFs, gDriveNum);

    InitGlobals();

    RunTest();

    //-------------------------------------
    DestroyGlobals();

    }

















