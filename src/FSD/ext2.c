/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             Ext2.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://ext2.yeah.net
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include "ntifs.h"
#include "ext2fs.h"

/* GLOBALS ***************************************************************/

extern PEXT2_GLOBAL g_gExt2Global;

/* DEFINITIONS *************************************************************/

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, Ext2LoadSuper)
#pragma alloc_text(PAGE, Ext2SaveSuper)

#pragma alloc_text(PAGE, Ext2LoadGroup)
#pragma alloc_text(PAGE, Ext2SaveGroup)

#pragma alloc_text(PAGE, Ext2GetInodeLba)
#pragma alloc_text(PAGE, Ext2LoadInode)
#pragma alloc_text(PAGE, Ext2SaveInode)

#pragma alloc_text(PAGE, Ext2LoadBlock)
#pragma alloc_text(PAGE, Ext2SaveBlock)

#pragma alloc_text(PAGE, Ext2SaveBuffer)

#pragma alloc_text(PAGE, Ext2GetBlock)
#pragma alloc_text(PAGE, Ext2BlockMap)

#pragma alloc_text(PAGE, Ext2BuildBDL)

#pragma alloc_text(PAGE, Ext2NewBlock)
#pragma alloc_text(PAGE, Ext2FreeBlock)

#pragma alloc_text(PAGE, Ext2ExpandBlock)
#pragma alloc_text(PAGE, Ext2ExpandInode)

#pragma alloc_text(PAGE, Ext2NewInode)
#pragma alloc_text(PAGE, Ext2FreeInode)

#pragma alloc_text(PAGE, Ext2AddEntry)
#pragma alloc_text(PAGE, Ext2RemoveEntry)

#pragma alloc_text(PAGE, Ext2TruncateBlock)
#pragma alloc_text(PAGE, Ext2TruncateInode)

#pragma alloc_text(PAGE, Ext2AddMcbEntry)
#pragma alloc_text(PAGE, Ext2RemoveMcbEntry)
#pragma alloc_text(PAGE, Ext2LookupMcbEntry)

#endif

/* FUNCTIONS ***************************************************************/

PEXT2_SUPER_BLOCK
Ext2LoadSuper(IN PDEVICE_OBJECT pDeviceObject)
{
    PVOID       Buffer;
    NTSTATUS    Status;
    
    Buffer = ExAllocatePool(NonPagedPool, 2 * SECTOR_SIZE);
    if (!Buffer)
    {
        Ext2DbgPrint(D_EXT2, "Ext2LoadSuper: no enough memory.\n");
        return NULL;
    }

    Status = Ext2ReadDisk(pDeviceObject, 2, 0, SECTOR_SIZE * 2, Buffer);
    if (!NT_SUCCESS(Status))
    {
        Ext2DbgPrint(D_EXT2, "Ext2ReadDisk: Read Block Device error.\n");
                ExFreePool(Buffer);
        return NULL;
    }

    return (struct ext2_super_block *)Buffer;
}


BOOLEAN
Ext2SaveSuper(  IN PEXT2_IRP_CONTEXT    IrpContext,
                IN PEXT2_VCB            Vcb )
{
    LONGLONG    Offset;

    Offset = (LONGLONG) (2 * SECTOR_SIZE);

    return Ext2SaveBuffer(IrpContext, Vcb, Offset, (2 * SECTOR_SIZE), Vcb->ext2_super_block);
}

PEXT2_GROUP_DESC
Ext2LoadGroup(IN PEXT2_VCB vcb)
{
    ULONG       size;
    PVOID       Buffer;
    LONGLONG    lba;
    NTSTATUS    Status;

    PDEVICE_OBJECT pDeviceObject = vcb->TargetDeviceObject;
    struct ext2_super_block * sb = vcb->ext2_super_block;

    vcb->ext2_block = EXT2_MIN_BLOCK_SIZE << sb->s_log_block_size;
    vcb->ext2_frag = EXT2_MIN_FRAG_SIZE << sb->s_log_frag_size;

    vcb->ext2_groups = (sb->s_blocks_count - sb->s_first_data_block +
        sb->s_blocks_per_group - 1) / sb->s_blocks_per_group;

    size = sizeof(struct ext2_group_desc) * vcb->ext2_groups;

    Buffer = ExAllocatePool(PagedPool, size);
    if (!Buffer)
    {
        Ext2DbgPrint(D_EXT2, "Ext2LoadSuper: no enough memory.\n");
        return NULL;
    }

    if (vcb->ext2_block == EXT2_MIN_BLOCK_SIZE)
    {
        lba = (LONGLONG)2 * vcb->ext2_block;
    }

    if (vcb->ext2_block > EXT2_MIN_BLOCK_SIZE)
    {
        lba = (LONGLONG) (vcb->ext2_block);
    }

    Status = Ext2ReadDisk(vcb->TargetDeviceObject, (ULONG)(lba / SECTOR_SIZE), 0, size, Buffer);

    if (!NT_SUCCESS(Status))
    {
        ExFreePool(Buffer);
        Buffer = NULL;
    }

    return (PEXT2_GROUP_DESC)Buffer;
}


BOOLEAN
Ext2SaveGroup(  IN PEXT2_IRP_CONTEXT    IrpContext,
                IN PEXT2_VCB            Vcb )
{
    LONGLONG    Offset;
    ULONG       Len;

    if (Vcb->ext2_block == EXT2_MIN_BLOCK_SIZE) {

        Offset = (LONGLONG) (2 * Vcb->ext2_block);

    } else {

        Offset = (LONGLONG) (Vcb->ext2_block);
    }

    Len = (ULONG)(sizeof(struct ext2_group_desc) * Vcb->ext2_groups);

    return Ext2SaveBuffer(IrpContext, Vcb, Offset, Len, Vcb->ext2_group_desc);
}

BOOLEAN
Ext2GetInodeLba (IN PEXT2_VCB   vcb,
         IN  ULONG  inode,
         OUT PLONGLONG offset)
{
    LONGLONG loc;

    if (inode < 1 || inode > vcb->ext2_super_block->s_inodes_count)
    {
        Ext2DbgPrint(D_EXT2, "Ext2GetInodeLba: Inode value %xh is invalid.\n",inode);
        *offset = 0;
        return FALSE;
    }

    loc = (vcb->ext2_group_desc[(inode - 1) / vcb->ext2_super_block->s_inodes_per_group].bg_inode_table);
    loc = loc * vcb->ext2_block;
    loc = loc + ((inode - 1) % (vcb->ext2_super_block->s_inodes_per_group)) * sizeof(EXT2_INODE);

    *offset = loc;

//  Ext2DbgPrint(D_EXT2, "Ext2GetInodeLba: inode=%xh lba=%xh offset=%xh\n",
//      inode, *lba, *offset);
    return TRUE;
}

BOOLEAN
Ext2LoadInode (IN PEXT2_VCB vcb,
           IN ULONG     inode,
           IN PEXT2_INODE ext2_inode)
{
    IO_STATUS_BLOCK     IoStatus;
    LONGLONG            Offset; 

    if (!Ext2GetInodeLba(vcb, inode, &Offset))
    {
        Ext2DbgPrint(D_EXT2, "Ext2LoadInode: error get inode(%xh)'s addr.\n", inode);
        return FALSE;
    }

    if (!Ext2CopyRead(
            vcb->StreamObj,
            (PLARGE_INTEGER)&Offset,
            sizeof(EXT2_INODE),
            TRUE,
            (PVOID)ext2_inode,
            &IoStatus ));

    if (!NT_SUCCESS(IoStatus.Status))
    {
        return FALSE;
    }

    return TRUE;
}

/*
BOOLEAN
Ext2SaveInode (IN PEXT2_VCB vcb,
           IN ULONG inode,
           IN struct ext2_inode *ext2_inode)
{
    ULONG       lba;
    ULONG       offset;
    NTSTATUS    Status;

    if (!Ext2GetInodeLba(vcb, inode, &lba, &offset))
    {
        Ext2DbgPrint(D_EXT2, "Ext2LoadInode: error get inode(%xh)'s addr.\n", inode);
        return FALSE;
    }

    Status = Ext2WriteDisk(vcb->TargetDeviceObject,
        lba,
        offset,
        sizeof(EXT2_INODE),
        (PVOID)ext2_inode);

    if (!NT_SUCCESS(Status))
    {
        return FALSE;
    }

    return TRUE;
}
*/

BOOLEAN
Ext2SaveInode ( IN PEXT2_IRP_CONTEXT IrpContext,
                IN PEXT2_VCB Vcb,
                IN ULONG Inode,
                IN PEXT2_INODE Ext2Inode)
{
    LONGLONG        Offset = 0;
    LARGE_INTEGER   CurrentTime;

    KeQuerySystemTime(&CurrentTime);
    Ext2Inode->i_mtime = Ext2Inode->i_atime = 
                      (ULONG)(Ext2InodeTime(CurrentTime));
#if 0
    Ext2DbgPrint(D_EXT2, "Ext2SaveInode: Saving Inode %xh: Mode=%xh Size=%xh\n", Inode, Ext2Inode->i_mode, Ext2Inode->i_size);
#endif

    if (!Ext2GetInodeLba(Vcb, Inode, &Offset))
    {
        Ext2DbgPrint(D_EXT2, "Ext2SaveInode: error get inode(%xh)'s addr.\n", Inode);
        return FALSE;
    }

    return Ext2SaveBuffer(IrpContext, Vcb, Offset, sizeof(EXT2_INODE), Ext2Inode);
}


BOOLEAN
Ext2LoadBlock (IN PEXT2_VCB Vcb,
           IN ULONG     dwBlk,
           IN PVOID     Buffer )
{
    IO_STATUS_BLOCK     IoStatus;
    LONGLONG            Offset; 

    Offset = (LONGLONG) dwBlk;
    Offset = Offset * Vcb->ext2_block;

    if (!Ext2CopyRead(
            Vcb->StreamObj,
            (PLARGE_INTEGER)&Offset,
            Vcb->ext2_block,
            TRUE,
            Buffer,
            &IoStatus ));

    if (!NT_SUCCESS(IoStatus.Status))
    {
        return FALSE;
    }

    return TRUE;
}


BOOLEAN
Ext2SaveBlock ( IN PEXT2_IRP_CONTEXT    IrpContext,
                IN PEXT2_VCB            Vcb,
                IN ULONG                dwBlk,
                IN PVOID                Buf )
{
    LONGLONG Offset;

    Offset = (LONGLONG) dwBlk;
    Offset = Offset * Vcb->ext2_block;

    return Ext2SaveBuffer(IrpContext, Vcb, Offset, Vcb->ext2_block, Buf);
}


BOOLEAN
Ext2SaveBuffer( IN PEXT2_IRP_CONTEXT    IrpContext,
                IN PEXT2_VCB            Vcb,
                IN LONGLONG             Offset,
                IN ULONG                Size,
                IN PVOID                Buf )
{
    PBCB        Bcb;
    PVOID       Buffer;

    if( !CcPinRead( Vcb->StreamObj,
                    (PLARGE_INTEGER) (&Offset),
                    Size,
                    TRUE,
                    &Bcb,
                    &Buffer ))
    {
        Ext2DbgPrint(D_EXT2, "Ext2SaveBuffer: PinReading error ...\n");
        return FALSE;
    }

#if 0
    Ext2DbgPrint(D_EXT2, "Ext2SaveBuffer: Off=%I64xh Len=%xh Bcb=%xh\n", Offset, Size, (ULONG)Bcb);
#endif

    RtlCopyMemory(Buffer, Buf, Size);
    CcSetDirtyPinnedData(Bcb, NULL );

    Ext2RepinBcb(IrpContext, Bcb);

    CcUnpinData(Bcb);

    SetFlag(Vcb->StreamObj->Flags, FO_FILE_MODIFIED);

    Ext2AddMcbEntry(Vcb, Offset, (LONGLONG)Size);

    return TRUE;
}

ULONG Ext2GetBlock(IN PEXT2_VCB vcb,
           ULONG dwContent,
           ULONG Index,
           int   layer )
{
    ULONG       *pData = NULL;
    ULONG       i = 0, j = 0, temp = 1;
    ULONG       dwBlk = 0;

    if (layer == 0)
    {
        dwBlk = dwContent;
    }
    else if (layer <= 3)
    {
        pData = (ULONG *)ExAllocatePool(PagedPool,
                    vcb->ext2_block);
        if (!pData)
        {
            Ext2DbgPrint(D_EXT2, "Ext2GetBlock: no enough memory.\n");
            return dwBlk;
        }

        if (!Ext2LoadBlock(vcb, dwContent, pData))
        {
            return 0;
        }

        temp = 1 << ((10 + vcb->ext2_super_block->s_log_block_size - 2) * (layer - 1));
        
        i = Index / temp;
        j = Index % temp;

        dwBlk = pData[i];

        ExFreePool(pData);

        dwBlk = Ext2GetBlock(vcb, dwBlk, j, layer - 1);
    }
    
    return dwBlk;
}

ULONG Ext2BlockMap(IN PEXT2_VCB vcb,
           IN PEXT2_INODE ext2_inode,
           IN ULONG Index)
{
    ULONG   dwSizes[4] = {EXT2_NDIR_BLOCKS, 1, 1, 1};
    int     i;
    ULONG   dwBlk;

    for (i = 0; i < 4; i++)
    {
        dwSizes[i] = dwSizes[i] << ((10 + vcb->ext2_super_block->s_log_block_size - 2) * i);
    }

    if ((Index * (vcb->ext2_block / SECTOR_SIZE)) >= ext2_inode->i_blocks)
    {
        Ext2DbgPrint(D_EXT2, "Ext2BlockMap: error input paramters.\n");
        return 0;
    }

    for (i = 0; i < 4; i++)
    {
        if (Index < dwSizes[i])
        {
            dwBlk = ext2_inode->i_block[i==0 ? (Index):(i + EXT2_NDIR_BLOCKS - 1)];
#if DBG
            {   
                ULONG dwRet = Ext2GetBlock(vcb, dwBlk, Index , i);

                Ext2DbgPrint(D_EXT2, "Ext2BlockMap: i=%xh index=%xh dwBlk=%xh (%xh)\n", i, Index, dwRet, dwBlk);

                return dwRet;
            }
#else
            return Ext2GetBlock(vcb, dwBlk, Index , i);
#endif
        }
	Index -= dwSizes[i];
    }

    return 0;
}


ULONG Ext2BuildBDL( IN PEXT2_IRP_CONTEXT    IrpContext,
                    IN PEXT2_VCB Vcb,
                    IN PEXT2_INODE ext2_inode,
                    IN ULONG offset, 
                    IN ULONG size, 
                    OUT PEXT2_BDL *ext2_bdl )
{
    ULONG   nBeg, nEnd, nBlocks;
    ULONG   dwBlk;
    ULONG   i;
    ULONG   dwBytes = 0;
    LONGLONG lba;

    PEXT2_BDL   ext2bdl;

    *ext2_bdl = NULL;

    if (offset >= ext2_inode->i_size)
    {
        Ext2DbgPrint(D_EXT2, "Ext2BuildBDL: beyond the file range.\n");
        return 0;
    }

/*
    if (offset + size > ext2_inode->i_size)
    {
        size = ext2_inode->i_size - offset;
    }
*/

    nBeg = offset / Vcb->ext2_block;
    nEnd = (size + offset + Vcb->ext2_block - 1) / Vcb->ext2_block;

    nBlocks = 0;

    if ((nEnd - nBeg) > 0)
    {
        ext2bdl = ExAllocatePool(PagedPool, sizeof(EXT2_BDL) * (nEnd - nBeg));

        if (ext2bdl)
        {

            RtlZeroMemory(ext2bdl, sizeof(EXT2_BDL) * (nEnd - nBeg));
            
            for (i = nBeg; i < nEnd; i++)
            {
                dwBlk = Ext2BlockMap(Vcb, ext2_inode, i);

                if (dwBlk > 0)
                {
                
                    lba = (LONGLONG) dwBlk;
                    lba = lba * Vcb->ext2_block;
                
                    if (nBeg == nEnd - 1) // ie. (nBeg == nEnd - 1)
                    {
                        dwBytes = size;
                        ext2bdl[nBlocks].Lba = lba + (LONGLONG)(offset % (Vcb->ext2_block));
                        ext2bdl[nBlocks].Length = dwBytes;
                        ext2bdl[nBlocks].Offset = 0;

                        nBlocks++;
                    }
                    else
                    {
                        if (i == nBeg)
                        {
                            dwBytes = Vcb->ext2_block - (offset % (Vcb->ext2_block));
                            ext2bdl[nBlocks].Lba = lba + (LONGLONG)(offset % (Vcb->ext2_block));
                            ext2bdl[nBlocks].Length = dwBytes;
                            ext2bdl[nBlocks].Offset = 0;

                            nBlocks++;
                        }
                        else if (i == nEnd - 1)
                        {
                            if (ext2bdl[nBlocks - 1].Lba + ext2bdl[nBlocks - 1].Length == lba)
                            {
                                ext2bdl[nBlocks - 1].Length += size - dwBytes;
                            }
                            else
                            {
                                ext2bdl[nBlocks].Lba = lba;
                                ext2bdl[nBlocks].Length = size - dwBytes;
                                ext2bdl[nBlocks].Offset = dwBytes;
                                nBlocks++;
                            }

                            dwBytes = size;

                        }
                        else
                        {
                            if (ext2bdl[nBlocks - 1].Lba + ext2bdl[nBlocks - 1].Length == lba)
                            {
                                ext2bdl[nBlocks - 1].Length += Vcb->ext2_block;
                            }
                            else
                            {
                                ext2bdl[nBlocks].Lba = lba;
                                ext2bdl[nBlocks].Length = Vcb->ext2_block;
                                ext2bdl[nBlocks].Offset = dwBytes;
                                nBlocks++;
                            }

                            dwBytes +=  Vcb->ext2_block;
                        }
                    }
                }
                else
                {
                    break;
                }
            }

            *ext2_bdl = ext2bdl;
            return nBlocks;
        }
    }

    // Error
    return 0;
}

BOOLEAN Ext2NewBlock(   PEXT2_IRP_CONTEXT IrpContext,
                        PEXT2_VCB Vcb,
                        ULONG     GroupHint,
                        ULONG     BlockHint,  
                        PULONG    dwRet )
{
    RTL_BITMAP      BlockBitmap;
    LARGE_INTEGER   Offset;
    ULONG           Length;

    PBCB            BitmapBcb;
    PVOID           BitmapCache;

    ULONG           Group = 0, dwBlk, dwHint = 0;

    *dwRet = 0;
    dwBlk = 0XFFFFFFFF;

    if (GroupHint > Vcb->ext2_groups)
        GroupHint = Vcb->ext2_groups - 1;

    if (BlockHint != 0)
    {
        GroupHint = (BlockHint - EXT2_FIRST_DATA_BLOCK) / Vcb->ext2_super_block->s_blocks_per_group;
        dwHint = (BlockHint - EXT2_FIRST_DATA_BLOCK) % Vcb->ext2_super_block->s_blocks_per_group;
    }
  
ScanBitmap:
  
    // Perform Prefered Group
    if (Vcb->ext2_group_desc[GroupHint].bg_free_blocks_count)
    {
        Offset.QuadPart = (LONGLONG) Vcb->ext2_block;
        Offset.QuadPart = Offset.QuadPart * Vcb->ext2_group_desc[GroupHint].bg_block_bitmap;

        if (GroupHint == Vcb->ext2_groups - 1)
            Length = Vcb->ext2_super_block->s_blocks_count % Vcb->ext2_super_block->s_blocks_per_group;
        else
            Length = Vcb->ext2_super_block->s_blocks_per_group;

        if (!CcPinRead( Vcb->StreamObj,
                        &Offset,
                        Vcb->ext2_block,
                        TRUE,
                        &BitmapBcb,
                        &BitmapCache ) )
        {
            Ext2DbgPrint(D_EXT2, "Ext2NewBlock: PinReading error ...\n");
            return FALSE;
        }

        RtlInitializeBitMap( &BlockBitmap,
                             BitmapCache,
                             Length );

        Group = GroupHint;

        if (RtlCheckBit(&BlockBitmap, dwHint) == 0)
        {
            dwBlk = dwHint;
        }
        else
        {
            dwBlk = RtlFindClearBits(&BlockBitmap, 1, dwHint);
        }

        // We could not get new block in the prefered group.
        if (dwBlk == 0xFFFFFFFF)
        {
            CcUnpinData(BitmapBcb);
            BitmapBcb = NULL;
            BitmapCache = NULL;

            RtlZeroMemory(&BlockBitmap, sizeof(RTL_BITMAP));
        }
    }

    if (dwBlk == 0xFFFFFFFF)
    {
        for(Group = 0; Group < Vcb->ext2_groups; Group++)
        if (Vcb->ext2_group_desc[Group].bg_free_blocks_count)
        {

            if (Group == GroupHint)
                continue;

            Offset.QuadPart = (LONGLONG) Vcb->ext2_block;
            Offset.QuadPart = Offset.QuadPart * Vcb->ext2_group_desc[Group].bg_block_bitmap;

            if (Vcb->ext2_groups == 1)
            {
                Length = Vcb->ext2_super_block->s_blocks_count;
            }
            else
            {
                if (Group == Vcb->ext2_groups - 1)
                    Length = Vcb->ext2_super_block->s_blocks_count % Vcb->ext2_super_block->s_blocks_per_group;
                else
                    Length = Vcb->ext2_super_block->s_blocks_per_group;
            }

            if (!CcPinRead( Vcb->StreamObj,
                            &Offset,
                            Vcb->ext2_block,
                            TRUE,
                            &BitmapBcb,
                            &BitmapCache ) )
            {
                Ext2DbgPrint(D_EXT2, "Ext2NewBlock: PinReading error ...\n");
                return FALSE;
            }

            RtlInitializeBitMap( &BlockBitmap,
                                 BitmapCache,
                                 Length );

            dwBlk = RtlFindClearBits(&BlockBitmap, 1, 0);

            if (dwBlk != 0xFFFFFFFF)
            {
                break;
            }
            else
            {
                CcUnpinData(BitmapBcb);
                BitmapBcb = NULL;
                BitmapCache = NULL;

                RtlZeroMemory(&BlockBitmap, sizeof(RTL_BITMAP));
            }
        }
    }
        
    if (dwBlk < Length)
    {
        RtlSetBits(&BlockBitmap, dwBlk, 1);

	    CcSetDirtyPinnedData(BitmapBcb, NULL );

        Ext2RepinBcb(IrpContext, BitmapBcb);

        CcUnpinData(BitmapBcb);

        Ext2AddMcbEntry(Vcb, Offset.QuadPart, (LONGLONG)Vcb->ext2_block);

        *dwRet = dwBlk + EXT2_FIRST_DATA_BLOCK + Group * Vcb->ext2_super_block->s_blocks_per_group;

        //Updating Group Desc / Superblock
        Vcb->ext2_group_desc[Group].bg_free_blocks_count--;
        Ext2SaveGroup(IrpContext, Vcb);

        Vcb->ext2_super_block->s_free_blocks_count--;
        Ext2SaveSuper(IrpContext, Vcb);

        {
            ULONG i=0;
            for (i=0; i < Vcb->ext2_groups; i++)
            {
                if ((Vcb->ext2_group_desc[i].bg_block_bitmap == *dwRet) ||
                    (Vcb->ext2_group_desc[i].bg_inode_bitmap == *dwRet) ||
                    (Vcb->ext2_group_desc[i].bg_inode_table == *dwRet) )
                {
                    Ext2DbgBreakPoint();
                    GroupHint = Group;
                    goto ScanBitmap;
                }
            }
        }

        return TRUE;
    }

    return FALSE;
}

BOOLEAN Ext2FreeBlock(  PEXT2_IRP_CONTEXT IrpContext,
                        PEXT2_VCB Vcb,
                        ULONG     Block )
{
    RTL_BITMAP      BlockBitmap;
    LARGE_INTEGER   Offset;
    ULONG           Length;

    PBCB            BitmapBcb;
    PVOID           BitmapCache;

    ULONG           Group, dwBlk;
    BOOLEAN         bModified = FALSE;

    if (Block < EXT2_FIRST_DATA_BLOCK || Block > (Vcb->ext2_super_block->s_blocks_per_group * Vcb->ext2_groups))
    {
        Ext2DbgBreakPoint();
        return TRUE;
    }

    Ext2DbgPrint(D_EXT2, "Ext2FreeBlock: Block %xh to be freed.\n", Block);

    Group = (Block - EXT2_FIRST_DATA_BLOCK) / (Vcb->ext2_super_block->s_blocks_per_group);

    dwBlk = (Block - EXT2_FIRST_DATA_BLOCK) % Vcb->ext2_super_block->s_blocks_per_group;
    
    {
        Offset.QuadPart = (LONGLONG) Vcb->ext2_block;
        Offset.QuadPart = Offset.QuadPart * Vcb->ext2_group_desc[Group].bg_block_bitmap;

        if (Group == Vcb->ext2_groups - 1)
            Length = Vcb->ext2_super_block->s_blocks_count % Vcb->ext2_super_block->s_blocks_per_group;
        else
            Length = Vcb->ext2_super_block->s_blocks_per_group;

        if (!CcPinRead( Vcb->StreamObj,
                        &Offset,
                        Vcb->ext2_block,
                        TRUE,
                        &BitmapBcb,
                        &BitmapCache ) )
        {
            Ext2DbgPrint(D_EXT2, "Ext2DeleteBlock: PinReading error ...\n");
            return FALSE;
        }

        RtlInitializeBitMap( &BlockBitmap,
                             BitmapCache,
                             Length );

        if (RtlCheckBit(&BlockBitmap, dwBlk) == 0)
        {
            
        }
        else
        {
            RtlClearBits(&BlockBitmap, dwBlk, 1);
            bModified = TRUE;
        }

        if (!bModified)
        {
            CcUnpinData(BitmapBcb);
            BitmapBcb = NULL;
            BitmapCache = NULL;

            RtlZeroMemory(&BlockBitmap, sizeof(RTL_BITMAP));
        }
    }
        
    if (bModified)
    {
	    CcSetDirtyPinnedData(BitmapBcb, NULL );

        Ext2RepinBcb(IrpContext, BitmapBcb);

        CcUnpinData(BitmapBcb);

        Ext2AddMcbEntry(Vcb, Offset.QuadPart, (LONGLONG)Vcb->ext2_block);

        //Updating Group Desc / Superblock
        Vcb->ext2_group_desc[Group].bg_free_blocks_count++;
        Ext2SaveGroup(IrpContext, Vcb);

        Vcb->ext2_super_block->s_free_blocks_count++;
        Ext2SaveSuper(IrpContext, Vcb);

        return TRUE;
    }

    return FALSE;
}


BOOLEAN Ext2ExpandBlock(PEXT2_IRP_CONTEXT IrpContext,
                        PEXT2_VCB   Vcb,
                        PEXT2_FCB   Fcb,
                        ULONG   dwContent,
                        ULONG   Index,
                        ULONG   layer,
                        BOOLEAN bNew,
                        ULONG   *dwRet)
{
	ULONG		*pData = NULL;
	ULONG		i = 0, j = 0, temp = 1;
	ULONG		dwNewBlk = 0, dwBlk = 0;
    BOOLEAN     bDirty = FALSE;
    BOOLEAN     bRet = TRUE;

    PEXT2_SUPER_BLOCK pExt2Sb = Vcb->ext2_super_block;

    pData = (ULONG *) ExAllocatePool(PagedPool, Vcb->ext2_block);

    if (!pData)
    {
        return FALSE;
    }

    RtlZeroMemory(pData, Vcb->ext2_block);

    if (bNew)
    {
        if (layer == 0 && !IsFlagOn(Fcb->Ext2Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY))
        {
            LARGE_INTEGER   Offset;
            
            Offset.QuadPart  = (LONGLONG) dwContent;
            Offset.QuadPart = Offset.QuadPart * Vcb->ext2_block;

            Ext2RemoveMcbEntry(Vcb, Offset.QuadPart, (LONGLONG)Vcb->ext2_block);

/*
            if (Vcb->SectionObject)
            {
                CcFlushCache(   &(Vcb->SectionObject),
                                (PLARGE_INTEGER)&(Offset),
                                Vcb->ext2_block,
                                NULL);

                if (Vcb->SectionObject.DataSectionObject != NULL)
                {
                    ExAcquireSharedStarveExclusive(&Vcb->PagingIoResource, TRUE);
                    ExReleaseResource(&Vcb->PagingIoResource);
                           
   			        CcPurgeCacheSection( &(Vcb->SectionObject),
                                         (PLARGE_INTEGER)(&Offset),
								         Vcb->ext2_block,
                                         FALSE );
                }
            }
*/
        }
        else
        {
          if (!Ext2SaveBlock(IrpContext, Vcb, dwContent, (PVOID)pData))
            {
                bRet = FALSE;
                goto errorout;
            }
        }
    }

	if (layer == 0)
	{
		dwNewBlk = dwContent;
	}
	else if (layer <= 3)
	{
        if (!bNew)
        {
            bRet = Ext2LoadBlock(Vcb, dwContent, (void *)pData);
            if (!bRet) goto errorout;
        }

        temp = 1 << ((10 + pExt2Sb->s_log_block_size - 2) * (layer - 1));

		i = Index / temp;
		j = Index % temp;

        dwBlk = pData[i];

        if (dwBlk == 0)
        {
            if (!Ext2NewBlock(IrpContext, Vcb, 0, dwContent, &dwBlk))
            {
                bRet = FALSE;
                Ext2DbgPrint(D_EXT2, "Ext2ExpandBlock: get new block error.\n");
                goto errorout;
            }

            pData[i] = dwBlk;
            bDirty = TRUE;
        }

		if (!Ext2ExpandBlock(IrpContext, Vcb, Fcb, dwBlk, j, layer - 1, bDirty, &dwNewBlk))
        {
            bRet = FALSE;
            Ext2DbgPrint(D_EXT2, "Ext2ExpandBlockk: ... error recuise...\n");
            goto errorout;
        }
        
        if (bDirty)
        {
            bRet = Ext2SaveBlock(IrpContext, Vcb, dwContent, (void *)pData);
        }
	}

errorout:

    if (pData)
        ExFreePool(pData);

    if (bRet && dwRet)
        *dwRet = dwNewBlk;

	return bRet;
}


BOOLEAN
Ext2ExpandInode(
                PEXT2_IRP_CONTEXT IrpContext,
                PEXT2_VCB Vcb,
                PEXT2_FCB Fcb,
                ULONG *dwRet )
{
	ULONG dwSizes[4] = {EXT2_NDIR_BLOCKS, 1, 1, 1};
    ULONG Index = 0;
    ULONG dwTotal = 0;
    ULONG dwBlk = 0, dwNewBlk = 0;
    PEXT2_SUPER_BLOCK pExt2Sb = Vcb->ext2_super_block;
	ULONG    i;
    BOOLEAN  bRet = FALSE;
    BOOLEAN  bNewBlock = FALSE;

    PEXT2_INODE Ext2Inode = Fcb->ext2_inode;

    Index = Ext2Inode->i_blocks / (Vcb->ext2_block / 512);

    for (i = 0; i < 4; i++)
    {
        dwSizes[i] = dwSizes[i] << ((10 + pExt2Sb->s_log_block_size - 2) * i);
        dwTotal += dwSizes[i];
    }

	if (Index >= dwTotal)
	{
		Ext2DbgPrint(D_EXT2, "Ext2ExpandInode: beyond the maxinum size of an inode.\n");
		return FALSE;
	}

	for (i = 0; i < 4; i++)
	{
		if (Index < dwSizes[i])
		{
            dwBlk = Ext2Inode->i_block[i==0 ? (Index):(i + EXT2_NDIR_BLOCKS - 1)];
            if (dwBlk == 0)
            {
                if (!Ext2NewBlock(IrpContext,
                            Vcb,
                            Fcb->BlkHint ? 0 : ((Fcb->Ext2Mcb->Inode - 1) / Vcb->ext2_super_block->s_inodes_per_group),
                            Fcb->BlkHint,
                            &dwBlk ) )
                {
                    Ext2DbgPrint(D_EXT2, "Ext2ExpandInode: get new block error.\n");
                    break;
                }

                Ext2Inode->i_block[i==0 ? (Index):(i + EXT2_NDIR_BLOCKS - 1)] = dwBlk;

                bNewBlock = TRUE;
            }

			bRet = Ext2ExpandBlock(IrpContext, Vcb, Fcb, dwBlk, Index , i, bNewBlock, &dwNewBlk); 

            if (bRet)
            {
                Ext2Inode->i_blocks += (Vcb->ext2_block / SECTOR_SIZE);
                Fcb->CommonFCBHeader.AllocationSize.QuadPart += Vcb->ext2_block;

                bRet = Ext2SaveInode(IrpContext, Vcb, Fcb->Ext2Mcb->Inode, Ext2Inode);
            }

            break;
		}

		Index -= dwSizes[i];
	}

    if (bRet && dwNewBlk)
    {
        if (dwRet)
        {
            Fcb->BlkHint = dwNewBlk+1;
            *dwRet = dwNewBlk;

            Ext2DbgPrint(D_EXT2, "Ext2ExpandInode: %S (%xh) i=%2.2xh Index=%8.8xh New Block=%8.8xh\n", Fcb->Ext2Mcb->ShortName.Buffer, Fcb->Ext2Mcb->Inode, i, Index, dwNewBlk);
        }
        return TRUE;
    }
    else
        return FALSE;
}

BOOLEAN
Ext2NewInode(
            PEXT2_IRP_CONTEXT IrpContext,
            PEXT2_VCB Vcb,
            ULONG   GroupHint,
            ULONG   Type,
            PULONG  Inode )
{
    RTL_BITMAP      InodeBitmap;
    PVOID           BitmapCache;
    PBCB            BitmapBcb;

    ULONG           Group, i, j;
    ULONG           Average, Length;
    LARGE_INTEGER   Offset;
    
    ULONG           dwInode;
    
    *Inode = dwInode = 0XFFFFFFFF;

repeat:

	Group = i = 0;
	
	if (Type == EXT2_FT_DIR)
    {
		Average = Vcb->ext2_super_block->s_free_inodes_count / Vcb->ext2_groups;

		for (j = 0; j < Vcb->ext2_groups; j++)
        {
            i = (j + GroupHint) % (Vcb->ext2_groups);

			if ((Vcb->ext2_group_desc[i].bg_used_dirs_count << 8) < 
                 Vcb->ext2_group_desc[i].bg_free_inodes_count )
            {
                Group = i + 1;
				break;
			}
		}

		if (!Group)
        {
			for (j = 0; j < Vcb->ext2_groups; j++)
            {
				if (Vcb->ext2_group_desc[j].bg_free_inodes_count >= Average)
                {
					if (!Group || (Vcb->ext2_group_desc[j].bg_free_blocks_count > Vcb->ext2_group_desc[Group].bg_free_blocks_count ))
						Group = j + 1;
				}
			}
		}
	}
	else 
	{
		/*
		 * Try to place the inode in its parent directory (GroupHint)
		 */
		if (Vcb->ext2_group_desc[GroupHint].bg_free_inodes_count)
        {
			Group = GroupHint + 1;
        }
		else
		{
            i = GroupHint;

			/*
			 * Use a quadratic hash to find a group with a
			 * free inode
			 */
			for (j = 1; j < Vcb->ext2_groups; j <<= 1)
            {

				i += j;
                if (i > Vcb->ext2_groups) 
                    i -= Vcb->ext2_groups;

				if (Vcb->ext2_group_desc[i].bg_free_inodes_count)
                {
					Group = i + 1;
					break;
				}
			}
		}

		if (!Group) {
			/*
			 * That failed: try linear search for a free inode
			 */
			i = GroupHint + 1;
			for (j = 2; j < Vcb->ext2_groups; j++)
            {
				if (++i >= Vcb->ext2_groups) i = 0;

				if (Vcb->ext2_group_desc[i].bg_free_inodes_count)
                {
					Group = i + 1;
					break;
				}
			}
		}
	}

    // Could not find a proper group.
	if (!Group)
    {
        return FALSE;
    }
    else
    {
        Group--;

        Offset.QuadPart = (LONGLONG) Vcb->ext2_block;
        Offset.QuadPart = Offset.QuadPart * Vcb->ext2_group_desc[Group].bg_inode_bitmap;

        if (Vcb->ext2_groups == 1)
        {
            Length = Vcb->ext2_super_block->s_inodes_count;
        }
        else
        {
            if (Group == Vcb->ext2_groups - 1)
                Length = Vcb->ext2_super_block->s_inodes_count % Vcb->ext2_super_block->s_inodes_per_group;
            else
                Length = Vcb->ext2_super_block->s_inodes_per_group;
        }
        
        if (!CcPinRead( Vcb->StreamObj,
                        &Offset,
                        Vcb->ext2_block,
                        TRUE,
                        &BitmapBcb,
                        &BitmapCache ) )
        {
            Ext2DbgPrint(D_EXT2, "Ext2NewInode: PinReading error ...\n");
            return FALSE;
        }

        RtlInitializeBitMap( &InodeBitmap,
                             BitmapCache,
                             Length );

        dwInode = RtlFindClearBits(&InodeBitmap, 1, 0);

        if (dwInode == 0xFFFFFFFF)
        {
            CcUnpinData(BitmapBcb);
            BitmapBcb = NULL;
            BitmapCache = NULL;

            RtlZeroMemory(&InodeBitmap, sizeof(RTL_BITMAP));
        }
    }

	if (dwInode == 0xFFFFFFFF || dwInode >= Length)
    {
		if (Vcb->ext2_group_desc[Group].bg_free_inodes_count != 0)
        {
            Vcb->ext2_group_desc[Group].bg_free_inodes_count = 0;

            Ext2SaveGroup(IrpContext, Vcb);            
		}

		goto repeat;
	}
    else
    {
        RtlSetBits(&InodeBitmap, dwInode, 1);

	    CcSetDirtyPinnedData(BitmapBcb, NULL );

        Ext2RepinBcb(IrpContext, BitmapBcb);

        CcUnpinData(BitmapBcb);

        Ext2AddMcbEntry(Vcb, Offset.QuadPart, (LONGLONG)Vcb->ext2_block);

        *Inode = dwInode + 1 + Group * Vcb->ext2_super_block->s_inodes_per_group;

        //Updating Group Desc / Superblock
        Vcb->ext2_group_desc[Group].bg_free_inodes_count--;
       	if (Type == EXT2_FT_DIR)
            Vcb->ext2_group_desc[Group].bg_used_dirs_count++;

        Ext2SaveGroup(IrpContext, Vcb);

        Vcb->ext2_super_block->s_free_inodes_count--;
        Ext2SaveSuper(IrpContext, Vcb);
        
        return TRUE;        
    }

    return FALSE;
}

BOOLEAN
Ext2FreeInode(
            PEXT2_IRP_CONTEXT IrpContext,
            PEXT2_VCB Vcb,
            ULONG Inode,
            ULONG Type )
{
    RTL_BITMAP      InodeBitmap;
    PVOID           BitmapCache;
    PBCB            BitmapBcb;

    ULONG           Group;
    ULONG           Length;
    LARGE_INTEGER   Offset;

    ULONG           dwIno;
    BOOLEAN         bModified = FALSE;
    

	Group = (Inode - 1) / Vcb->ext2_super_block->s_inodes_per_group;
    dwIno = (Inode - 1) % (Vcb->ext2_super_block->s_inodes_per_group);

    Ext2DbgPrint(D_EXT2, "Ext2FreeInode: Inode: %xh (Group/Off = %xh/%xh)\n", Inode, Group, dwIno);
	
    {
        Offset.QuadPart = (LONGLONG) Vcb->ext2_block;
        Offset.QuadPart = Offset.QuadPart * Vcb->ext2_group_desc[Group].bg_inode_bitmap;
        if (Group == Vcb->ext2_groups - 1)
            Length = Vcb->ext2_super_block->s_inodes_count % Vcb->ext2_super_block->s_inodes_per_group;
        else
            Length = Vcb->ext2_super_block->s_inodes_per_group;

        if (!CcPinRead( Vcb->StreamObj,
                        &Offset,
                        Vcb->ext2_block,
                        TRUE,
                        &BitmapBcb,
                        &BitmapCache ) )
        {
            Ext2DbgPrint(D_EXT2, "Ext2FreeInode: PinReading error ...\n");
            return FALSE;
        }

        RtlInitializeBitMap( &InodeBitmap,
                             BitmapCache,
                             Length );

        if (RtlCheckBit(&InodeBitmap, dwIno) == 0)
        {
            
        }
        else
        {
            RtlClearBits(&InodeBitmap, dwIno, 1);
            bModified = TRUE;
        }

        if (!bModified)
        {
            CcUnpinData(BitmapBcb);
            BitmapBcb = NULL;
            BitmapCache = NULL;

            RtlZeroMemory(&InodeBitmap, sizeof(RTL_BITMAP));
        }
    }
        
    if (bModified)
    {
	    CcSetDirtyPinnedData(BitmapBcb, NULL );

        Ext2RepinBcb(IrpContext, BitmapBcb);

        CcUnpinData(BitmapBcb);

        Ext2AddMcbEntry(Vcb, Offset.QuadPart, (LONGLONG)Vcb->ext2_block);

        //Updating Group Desc / Superblock
       	if (Type == EXT2_FT_DIR)
            Vcb->ext2_group_desc[Group].bg_used_dirs_count--;

        Vcb->ext2_group_desc[Group].bg_free_inodes_count++;
        Ext2SaveGroup(IrpContext, Vcb);

        Vcb->ext2_super_block->s_free_inodes_count++;
        Ext2SaveSuper(IrpContext, Vcb);

        return TRUE;
    }

    return FALSE;
}


NTSTATUS
Ext2AddEntry (
         IN PEXT2_IRP_CONTEXT   IrpContext,
         IN PEXT2_VCB           Vcb,
         IN PEXT2_FCB           Dcb,
         IN ULONG               FileType,
         IN ULONG               Inode,
         IN PUNICODE_STRING     FileName )
{
    NTSTATUS                Status = STATUS_UNSUCCESSFUL;

    PEXT2_DIR_ENTRY2        pDir = NULL;
    PEXT2_DIR_ENTRY2        pNewDir = NULL;
    PEXT2_DIR_ENTRY2        pTarget = NULL;

    ULONG                   Length = 0;
    ULONG                   dwBytes = 0;
    BOOLEAN                 bFound = FALSE;
    BOOLEAN                 bAdding = FALSE;

    BOOLEAN                 MainResourceAcquired = FALSE;

    ULONG                   dwRet;

    if (!IsFlagOn(Dcb->Ext2Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY))
    {
        Status = STATUS_INVALID_PARAMETER;
        return Status;
    }

    MainResourceAcquired = ExAcquireResourceExclusiveLite(&Dcb->MainResource, TRUE);

    __try
    {
        ExAcquireResourceExclusiveLite(&Dcb->CountResource, TRUE);
        Dcb->ReferenceCount++;
        ExReleaseResourceForThreadLite(
                    &Dcb->CountResource,
                    ExGetCurrentResourceThread());

        pDir = (PEXT2_DIR_ENTRY2) ExAllocatePool(PagedPool,
                                    EXT2_DIR_REC_LEN(EXT2_NAME_LEN));
        if (!pDir)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        pTarget = (PEXT2_DIR_ENTRY2) ExAllocatePool(PagedPool,
                                     2 * EXT2_DIR_REC_LEN(EXT2_NAME_LEN));
        if (!pTarget)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        if (IsFlagOn(Vcb->ext2_super_block->s_feature_incompat, EXT2_FEATURE_INCOMPAT_FILETYPE))
        {
            pDir->file_type = (UCHAR) FileType;
        }
        else
        {
            pDir->file_type = 0;
        }

        pDir->inode  = Inode;
        pDir->name_len = (UCHAR) ( FileName->Length > (EXT2_NAME_LEN * 2) ?
                                   EXT2_NAME_LEN : (FileName->Length / 2) );

        Ext2WcharToChar(
            pDir->name,
            FileName->Buffer,
            pDir->name_len);

        pDir->rec_len = (USHORT) (EXT2_DIR_REC_LEN(pDir->name_len));

        dwBytes = 0;

Repeat:

        while ((LONGLONG)dwBytes < Dcb->CommonFCBHeader.AllocationSize.QuadPart)
        {
            RtlZeroMemory(pTarget, EXT2_DIR_REC_LEN(EXT2_NAME_LEN));

            // Reading the DCB contents
            Status = Ext2ReadInode(
                        NULL,
                        Vcb,
                        Dcb->ext2_inode,
                        dwBytes,
                        (PVOID)pTarget,
                        EXT2_DIR_REC_LEN(EXT2_NAME_LEN),
                        &dwRet);

            if (!NT_SUCCESS(Status))
            {
                Ext2DbgPrint(D_EXT2, "Ext2AddDirectory: Reading Directory Content error.\n");
                __leave;
            }

            if ((pTarget->inode == 0) || (pTarget->rec_len >= EXT2_DIR_REC_LEN(pTarget->name_len) + pDir->rec_len))
            {
                if (pTarget->inode)
                {
                    RtlZeroMemory(pTarget, 2 * EXT2_DIR_REC_LEN(EXT2_NAME_LEN));

                    // Reading the DCB contents
                     Status = Ext2ReadInode(
                                NULL,
                                Vcb,
                                Dcb->ext2_inode,
                                dwBytes,
                                (PVOID)pTarget,
                                2 * EXT2_DIR_REC_LEN(EXT2_NAME_LEN),
                                &dwRet);

                    if (!NT_SUCCESS(Status))
                    {
                        Ext2DbgPrint(D_EXT2, "Ext2AddDirectory: Reading Directory Content error.\n");
                        __leave;
                    }

                    Length = EXT2_DIR_REC_LEN(pTarget->name_len);
                    
                    pNewDir = (PEXT2_DIR_ENTRY2) ((PUCHAR)pTarget + EXT2_DIR_REC_LEN(pTarget->name_len));
                    pNewDir->rec_len = pTarget->rec_len - EXT2_DIR_REC_LEN(pTarget->name_len);

                    pTarget->rec_len = EXT2_DIR_REC_LEN(pTarget->name_len);
                }
                else
                {
                    pNewDir = pTarget;
                    pNewDir->rec_len = (USHORT)((ULONG)(Dcb->CommonFCBHeader.AllocationSize.QuadPart) - dwBytes);
                }

                pNewDir->file_type = pDir->file_type;
                pNewDir->inode = pDir->inode;
                pNewDir->name_len = pDir->name_len;
                memcpy(pNewDir->name, pDir->name, pDir->name_len);
                Length += EXT2_DIR_REC_LEN(pDir->name_len);

                bFound = TRUE;
                break;
            }
            
            dwBytes += pTarget->rec_len;
        }

        if (bFound) // Here we fininish the searching journel ...
        {
            ULONG   dwRet;

            if ( (!((pDir->name_len == 1) && (pDir->name[0] == '.'))) &&
                 (!((pDir->name_len == 2) && (pDir->name[0] == '.') && (pDir->name[1] == '.'))) )
                Dcb->ext2_inode->i_links_count++;

            if (Ext2WriteInode(IrpContext, Vcb, Dcb->ext2_inode, dwBytes, pTarget, Length, FALSE, &dwRet))
                Status = STATUS_SUCCESS;
        }
        else
        {
            // We should expand the size of the dir inode 
            if (!bAdding)
            {
                ULONG dwRet;

                bAdding = Ext2ExpandInode(IrpContext, Vcb, Dcb, &dwRet);

                if (bAdding) {

                    Dcb->ext2_inode->i_size = Dcb->ext2_inode->i_blocks * SECTOR_SIZE;

                    Ext2SaveInode(IrpContext, Vcb, Dcb->Ext2Mcb->Inode, Dcb->ext2_inode);

                    Dcb->CommonFCBHeader.FileSize = Dcb->CommonFCBHeader.AllocationSize;

                     goto Repeat;
                }

                __leave;

            }
            else  // Something must be error!
            {
                __leave;
            }
        }
    }

    __finally
    {

        ExAcquireResourceExclusiveLite(&Dcb->CountResource, TRUE);
        Dcb->ReferenceCount--;
        ExReleaseResourceForThreadLite(
                    &Dcb->CountResource,
                    ExGetCurrentResourceThread());

        if(MainResourceAcquired)
            ExReleaseResourceForThreadLite(
                    &Dcb->MainResource,
                    ExGetCurrentResourceThread());

        if (pTarget != NULL)
        {
            ExFreePool(pTarget);
        }

        if (pDir)
            ExFreePool(pDir);
    }
    
    return Status;
}


NTSTATUS
Ext2RemoveEntry (
         IN PEXT2_IRP_CONTEXT   IrpContext,
         IN PEXT2_VCB           Vcb,
         IN PEXT2_FCB           Dcb,
         IN ULONG               Inode )
{
    NTSTATUS                Status = STATUS_UNSUCCESSFUL;

    PEXT2_DIR_ENTRY2        pTarget = NULL;
    PEXT2_DIR_ENTRY2        pPrevDir = NULL;

    USHORT                  PrevRecLen;

    ULONG                   Length = 0;
    ULONG                   dwBytes = 0;

    BOOLEAN                 bRet = FALSE;
    BOOLEAN                 MainResourceAcquired = FALSE;

    ULONG                   dwRet;

    if (!IsFlagOn(Dcb->Ext2Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY))
        return FALSE;

    MainResourceAcquired = ExAcquireResourceExclusiveLite(&Dcb->MainResource, TRUE);


    __try
    {

        ExAcquireResourceExclusiveLite(&Dcb->CountResource, TRUE);
        Dcb->ReferenceCount++;
        ExReleaseResourceForThreadLite(
                    &Dcb->CountResource,
                    ExGetCurrentResourceThread());

        pTarget = (PEXT2_DIR_ENTRY2) ExAllocatePool(PagedPool,
                                     EXT2_DIR_REC_LEN(EXT2_NAME_LEN));
        if (!pTarget)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }
        
        pPrevDir = (PEXT2_DIR_ENTRY2) ExAllocatePool(PagedPool,
                                     EXT2_DIR_REC_LEN(EXT2_NAME_LEN));
        if (!pPrevDir)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        dwBytes = 0;


        while ((LONGLONG)dwBytes < Dcb->CommonFCBHeader.AllocationSize.QuadPart)
        {
            RtlZeroMemory(pTarget, EXT2_DIR_REC_LEN(EXT2_NAME_LEN));

            Status = Ext2ReadInode(
                        NULL,
                        Vcb,
                        Dcb->ext2_inode,
                        dwBytes,
                        (PVOID)pTarget,
                        EXT2_DIR_REC_LEN(EXT2_NAME_LEN),
                        &dwRet);

            if (!NT_SUCCESS(Status))
            {
                Ext2DbgPrint(D_EXT2, "Ext2RemoveEntry: Reading Directory Content error.\n");
                __leave;
            }

            if (pTarget->inode == Inode)
            {
                ULONG   dwRet;
                ULONG   RecLen;

                pPrevDir->rec_len += pTarget->rec_len;
                RecLen = EXT2_DIR_REC_LEN(pTarget->name_len);

                RtlZeroMemory(pTarget, RecLen);

                Ext2WriteInode(IrpContext, Vcb, Dcb->ext2_inode, dwBytes - PrevRecLen, pPrevDir, 8, FALSE, &dwRet);
                Ext2WriteInode(IrpContext, Vcb, Dcb->ext2_inode, dwBytes, pTarget, RecLen, FALSE, &dwRet);

                // . / .. could not be removed.
                Dcb->ext2_inode->i_links_count--;

                Ext2SaveInode(IrpContext, Vcb, Dcb->Ext2Mcb->Inode, Dcb->ext2_inode);

                bRet = TRUE;
                
                break;
            }
            else
            {
                RtlCopyMemory(pPrevDir, pTarget, EXT2_DIR_REC_LEN(EXT2_NAME_LEN));
                PrevRecLen = pTarget->rec_len;
            }

            dwBytes += pTarget->rec_len;
        }
    }

    __finally
    {

        ExAcquireResourceExclusiveLite(&Dcb->CountResource, TRUE);
        Dcb->ReferenceCount--;
        ExReleaseResourceForThreadLite(
                    &Dcb->CountResource,
                    ExGetCurrentResourceThread());


        if(MainResourceAcquired)
            ExReleaseResourceForThreadLite(
                    &Dcb->MainResource,
                    ExGetCurrentResourceThread());


        if (pTarget != NULL)
        {
            ExFreePool(pTarget);
        }

        if (pPrevDir)
            ExFreePool(pPrevDir);
    }
    
    return bRet;
}

BOOLEAN Ext2TruncateBlock(
                        PEXT2_IRP_CONTEXT IrpContext,
                        PEXT2_VCB Vcb,
                        ULONG   dwContent,
                        ULONG   Index,
                        ULONG   layer,
                        BOOLEAN *bFreed )
{
	ULONG		*pData = NULL;
	ULONG		i = 0, j = 0, temp = 1;
    BOOLEAN     bDirty = FALSE;
    BOOLEAN     bRet = FALSE;
    ULONG       dwBlk;

    LONGLONG    Offset;

    PBCB        Bcb;

    PEXT2_SUPER_BLOCK pExt2Sb = Vcb->ext2_super_block;

    *bFreed = FALSE;

	if (layer == 0)
	{
        if (dwContent > 0 && dwContent < (Vcb->ext2_super_block->s_blocks_per_group * Vcb->ext2_groups))
            bRet = Ext2FreeBlock(IrpContext, Vcb, dwContent);
        else
            bRet = TRUE;
        *bFreed = bRet;
	}
	else if (layer <= 3)
	{
        Offset = (LONGLONG) dwContent;
        Offset = Offset * Vcb->ext2_block;

        if( !CcPinRead( Vcb->StreamObj,
                        (PLARGE_INTEGER) (&Offset),
                        Vcb->ext2_block,
                        TRUE,
                        &Bcb,
                        &pData ))
        {
            Ext2DbgPrint(D_EXT2, "Ext2SaveBuffer: PinReading error ...\n");
            goto errorout;
        }

        temp = 1 << ((10 + pExt2Sb->s_log_block_size - 2) * (layer - 1));

		i = Index / temp;
		j = Index % temp;

        dwBlk = pData[i];

        if(!Ext2TruncateBlock(IrpContext, Vcb, dwBlk, j, layer - 1, &bDirty))
            goto errorout;

        if (bDirty)
            pData[i] = 0;

        if (i == 0 && j == 0)
        {
            CcUnpinData(Bcb);
            pData = NULL;

            *bFreed = TRUE;
            bRet = Ext2FreeBlock(IrpContext, Vcb, dwContent);
        }
        else
        {
            CcSetDirtyPinnedData(Bcb, NULL );
            Ext2RepinBcb(IrpContext, Bcb);

            Ext2AddMcbEntry(Vcb, Offset, (LONGLONG)Vcb->ext2_block);

            bRet = TRUE;
            *bFreed = FALSE;
        }
	}

errorout:

    if (pData)
    {
        CcUnpinData(Bcb);
    }

	return bRet;
}


BOOLEAN
Ext2TruncateInode(
                PEXT2_IRP_CONTEXT IrpContext,
                PEXT2_VCB   Vcb,
                PEXT2_FCB   Fcb )
{
	ULONG   dwSizes[4] = {EXT2_NDIR_BLOCKS, 1, 1, 1};
    ULONG   Index = 0;
    ULONG   dwTotal = 0;
    ULONG   dwBlk = 0;

	ULONG    i;
    BOOLEAN  bRet = FALSE;
    BOOLEAN  bFreed = FALSE;

    PEXT2_SUPER_BLOCK pExt2Sb = Vcb->ext2_super_block;
    PEXT2_INODE Ext2Inode = Fcb->ext2_inode;

    Index = Ext2Inode->i_blocks / (Vcb->ext2_block / 512);

    if (Index > 0) {  Index--; } else { return TRUE; }

    for (i = 0; i < 4; i++)
    {
        dwSizes[i] = dwSizes[i] << ((10 + pExt2Sb->s_log_block_size - 2) * i);
        dwTotal += dwSizes[i];
    }

	if (Index >= dwTotal)
	{
		Ext2DbgPrint(D_EXT2, "Ext2ExpandInode: beyond the maxinum size of an inode.\n");
		return TRUE;
	}

	for (i = 0; i < 4; i++)
	{
		if (Index < dwSizes[i])
		{
            dwBlk = Ext2Inode->i_block[i==0 ? (Index):(i + EXT2_NDIR_BLOCKS - 1)];

			bRet = Ext2TruncateBlock(IrpContext, Vcb, dwBlk, Index , i, &bFreed); 

            if (bRet)
            {
                Ext2Inode->i_blocks -= (Vcb->ext2_block / SECTOR_SIZE);
                Fcb->CommonFCBHeader.AllocationSize.QuadPart -= Vcb->ext2_block;
            
                if (bFreed)
                {
                    Ext2Inode->i_block[i==0 ? (Index):(i + EXT2_NDIR_BLOCKS - 1)] = 0;

                    // Inode struct saving is done externally.
                    bRet = Ext2SaveInode(IrpContext, Vcb, Fcb->Ext2Mcb->Inode, Ext2Inode);
                }
            }

            break;
		}

		Index -= dwSizes[i];
	}

    return bRet;
}

BOOLEAN
Ext2AddMcbEntry (
    IN PEXT2_VCB Vcb,
    IN LONGLONG  Lba,
    IN LONGLONG  Length)
{
    LONGLONG    Offset;
    LONGLONG    DirtyLba;
    LONGLONG    DirtyLen;


    Offset = Lba & (~(SECTOR_SIZE - 1));

    Length = (Length + Lba - Offset + SECTOR_SIZE - 1) & (~(SECTOR_SIZE - 1));

    ASSERT ((Offset & (SECTOR_SIZE - 1)) == 0);
    ASSERT ((Length & (SECTOR_SIZE - 1)) == 0);

    if (Ext2LookupMcbEntry(Vcb, Offset, &DirtyLba, &DirtyLen, NULL, NULL, NULL))
    {
        if (DirtyLba == Offset && DirtyLen >= Length)
            return TRUE;
    }

    Ext2DbgPrint(D_EXT2, "Ext2AddMcbEntry: Range Lba/Len = %I64xh/%I64xh to be added.\n", Offset, Length);

#if DBG
    Ext2DbgPrint(D_EXT2, "Ext2AddMcbEntry: Number Runs in Mcb: %xh\n", FsRtlNumberOfRunsInLargeMcb(&(Vcb->DirtyMcbs)) );
#endif


    return FsRtlAddLargeMcbEntry(&(Vcb->DirtyMcbs), Offset, Offset, Length);
}

VOID
Ext2RemoveMcbEntry (
    IN PEXT2_VCB Vcb,
    IN LONGLONG  Lba,
    IN LONGLONG  Length)
{

    FsRtlRemoveLargeMcbEntry(&(Vcb->DirtyMcbs), Lba, Length);
}

BOOLEAN
Ext2LookupMcbEntry (
    IN PEXT2_VCB    Vcb,
    IN LONGLONG     Offset,
    OUT PLONGLONG   Lba,
    OUT PLONGLONG   Length,
    OUT PLONGLONG   RunStart,
    OUT PLONGLONG   RunLength,
    OUT PULONG      Index)
{
    BOOLEAN     bReturn;

    bReturn = FsRtlLookupLargeMcbEntry(
                &(Vcb->DirtyMcbs), Offset, Lba, Length,
                RunStart, RunLength, Index );

    return bReturn;
}
