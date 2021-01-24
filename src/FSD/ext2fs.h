
 /*                                                                            
  *		Copyright (c) 2001 - 2003 Satish Kumar J (vsat_in@yahoo.com)
  *
  *		Project:		Win2fs
  *                                                                            
  *		Module Name:	\FSD\Win2000\Ext2fs.h
  *                                                                            
  *		Abstract:		Main header for the driver.
  *
  *		Notes:			None  
  *
  *		Revision History:
  *
  *		Date		Version		Author				Change Log
  *		------------------------------------------------------------------------
  *
  *					0.0.1		Satish Kumar J		Initial Version
  */                          		

 #ifndef __EXT2FS_H
 #define __EXT2FS_H

 /////////////////////////////////////////////////////////////////////////////

 // Global declarations.

 // Indicate we are building the driver.
 #define DRIVER

 // Use this to force a debug build.
 // #undef  DBG
 #define DBG	1

 // Use this to force a read only build of the driver.
 //#define EXT2_READ_ONLY 1 

 /////////////////////////////////////////////////////////////////////////////

 // Includes.

 #include "ntifs.h"
 #include "ntddk.h"
 #include "ntdddisk.h"

 #include "debug.h"
 #include "ioctls.h"
 #include "linux/ext2_fs.h"

 /////////////////////////////////////////////////////////////////////////////

 #pragma pack(1)

 typedef PVOID PBCB;

 #define SECTOR_SIZE                     (512)
 #define READ_AHEAD_GRANULARITY          (0x10000)

 // Define IsEndofFile for read and write operations
 #define FILE_WRITE_TO_END_OF_FILE       0xffffffff
 #define FILE_USE_FILE_POINTER_POSITION  0xfffffffe
 
 #define IsEndOfFile(Pos) ((Pos.LowPart == FILE_WRITE_TO_END_OF_FILE) && \
                           (Pos.HighPart == FILE_USE_FILE_POINTER_POSITION ))

 /////////////////////////////////////////////////////////////////////////////

 #define CHECK_AND_SET(Status, Val)		if (NT_SUCCESS(Status))	{ Val = TRUE; }

 //
 // Inode flags
 //

 #define S_IFMT   0x0F000            /*017 0000 */

 #define S_IFSOCK 0x0C000            /*014 0000 */
 #define S_IFLNK  0x0A000            /*012 0000 */
 #define S_IFFIL  0x08000            /*010 0000 */
 #define S_IFBLK  0x06000            /*006 0000 */
 #define S_IFDIR  0x04000            /*004 0000 */
 #define S_IFCHR  0x02000            /*002 0000 */
 #define S_IFIFO  0x01000            /*001 0000 */

 #define S_ISSOCK(m)     (((m) & S_IFMT) == S_IFSOCK)
 #define S_ISLNK(m)      (((m) & S_IFMT) == S_IFLNK)
 #define S_ISFIL(m)      (((m) & S_IFMT) == S_IFFIL)
 #define S_ISBLK(m)      (((m) & S_IFMT) == S_IFBLK)
 #define S_ISDIR(m)      (((m) & S_IFMT) == S_IFDIR)
 #define S_ISCHR(m)      (((m) & S_IFMT) == S_IFCHR)
 #define S_ISFIFO(m)     (((m) & S_IFMT) == S_IFIFO)
 
 #define S_IPERMISSION_MASK 0x1FF 

 #define S_IRWXU 0x01C0     /*  00700 */
 #define S_IRUSR 0x0100     /*  00400 */
 #define S_IWUSR 0x0080     /*  00200 */
 #define S_IXUSR 0x0040     /*  00100 */

 #define S_IRWXG 0x0038     /*  00070 */
 #define S_IRGRP 0x0020     /*  00040 */
 #define S_IWGRP 0x0010     /*  00020 */
 #define S_IXGRP 0x0008     /*  00010 */
 
 #define S_IRWXO 0x0007     /*  00007 */
 #define S_IROTH 0x0004     /*  00004 */
 #define S_IWOTH 0x0002     /*  00002 */
 #define S_IXOTH 0x0001     /*  00001 */
 
 #define S_ISREADABLE(m)    (((m) & S_IPERMISSION_MASK) == (S_IRUSR | S_IRGRP | S_IROTH))
 #define S_ISWRITABLE(m)    (((m) & S_IPERMISSION_MASK) == (S_IWUSR | S_IWGRP | S_IWOTH))

 #define Ext2SetReadable(m) (m) = ((m) | (S_IRUSR | S_IRGRP | S_IROTH))
 #define Ext2SetWritable(m) (m) = ((m) | (S_IWUSR | S_IWGRP | S_IWOTH))

 #define Ext2SetReadOnly(m) (m) = ((m) & (~(S_IWUSR | S_IWGRP | S_IWOTH)))
 #define Ext2IsReadOnly(m)  (!((m) & (S_IWUSR | S_IWGRP | S_IWOTH)))
 
 #define EXT2_FIRST_DATA_BLOCK   (Vcb->ext2_super_block->s_first_data_block)

 typedef struct ext2_inode			EXT2_INODE,			*PEXT2_INODE;
 typedef struct ext2_dir_entry		EXT2_DIR_ENTRY,		*PEXT2_DIR_ENTRY;
 typedef struct ext2_dir_entry_2	EXT2_DIR_ENTRY2,	*PEXT2_DIR_ENTRY2;
 typedef struct ext2_group_desc		EXT2_GROUP_DESC,	*PEXT2_GROUP_DESC;
 typedef struct ext2_super_block	EXT2_SUPER_BLOCK,	*PEXT2_SUPER_BLOCK;

 /////////////////////////////////////////////////////////////////////////////

 //
 // Filesystem related stuff.
 //

 #define DRIVER_NAME		"Win2fs"
 #define DEVICE_NAME		L"\\FileSystem\\Ext2"

 //
 // Registry
 //

 #define PARAMETERS_KEY		L"\\Parameters"
 #define WRITING_SUPPORT	L"WritingSupport"

 #ifndef SetFlag
 #define SetFlag(x,f)    ((x) |= (f))
 #endif

 #ifndef ClearFlag
 #define ClearFlag(x,f)  ((x) &= ~(f))
 #endif

 #define IsFlagOn(a,b) ((BOOLEAN)(FlagOn(a,b) ? TRUE : FALSE))

 #define Ext2RaiseStatus(IRPCONTEXT,STATUS) {   \
    (IRPCONTEXT)->ExceptionStatus = (STATUS);	\
    ExRaiseStatus( (STATUS) );					\
 }

 #define Ext2NormalizeAndRaiseStatus(IRPCONTEXT,STATUS) {                       \
    /* (IRPCONTEXT)->ExceptionStatus = (STATUS);  */                            \
    if ((STATUS) == STATUS_VERIFY_REQUIRED) { ExRaiseStatus((STATUS)); }        \
    ExRaiseStatus(FsRtlNormalizeNtstatus((STATUS),STATUS_UNEXPECTED_IO_ERROR)); \
 }

 //
 // EXT2_IDENTIFIER_TYPE
 //
 // Identifiers used to mark the structures
 //

 typedef enum _EXT2_IDENTIFIER_TYPE 
 {
    EXT2FGD  = ':DGF',
    EXT2VCB  = ':BCV',
    EXT2FCB  = ':BCF',
    EXT2CCB  = ':BCC',
    EXT2ICX  = ':XCI',
    EXT2FSD  = ':DSF',
    EXT2MCB  = ':BCM'
 } EXT2_IDENTIFIER_TYPE;

 //
 // EXT2_IDENTIFIER
 //
 // Header used to mark the structures
 //

 typedef struct _EXT2_IDENTIFIER 
 {
    EXT2_IDENTIFIER_TYPE     Type;
    ULONG                    Size;
 } EXT2_IDENTIFIER, *PEXT2_IDENTIFIER;

 #define NodeType(Ptr) (*((EXT2_IDENTIFIER_TYPE *)(Ptr)))

 typedef struct _EXT2_MCB  EXT2_MCB, *PEXT2_MCB;

 //
 // EXT2_GLOBAL_DATA
 //
 // Data that is not specific to a mounted volume.
 //

 typedef struct _EXT2_GLOBAL 
 {
    // Identifier for this structure
    EXT2_IDENTIFIER             Identifier;
    
    // Syncronization primitive for this structure
    ERESOURCE                   Resource;
    
    // Table of pointers to the fast I/O entry points
    FAST_IO_DISPATCH            FastIoDispatch;
    
    // Table of pointers to the Cache Manager callbacks
    CACHE_MANAGER_CALLBACKS     CacheManagerCallbacks;
    CACHE_MANAGER_CALLBACKS     CacheManagerNoOpCallbacks;
    
    // Pointer to the driver object
    PDRIVER_OBJECT              DriverObject;
    
    // Pointer to the main device object
    PDEVICE_OBJECT              DeviceObject;
    
    // List of mounted volumes
    LIST_ENTRY                  VcbList;

    // Look Aside table of IRP_CONTEXT, FCB, MCB, CCB
    USHORT                      MaxDepth;
    NPAGED_LOOKASIDE_LIST       Ext2IrpContextLookasideList;
    NPAGED_LOOKASIDE_LIST       Ext2FcbLookasideList;
    NPAGED_LOOKASIDE_LIST       Ext2CcbLookasideList;
    PAGED_LOOKASIDE_LIST        Ext2McbLookasideList;

    // Mcb Count ...
    USHORT                      McbAllocated;

 #if DBG
    // Fcb Count
    USHORT                      FcbAllocated;

    // IRP_MJ_CLOSE : FCB
    USHORT                      IRPCloseCount;
 #endif
    
    // Global flags for the driver
    ULONG                       Flags;
    
    LARGE_INTEGER               TimeZone;
    
 } EXT2_GLOBAL, *PEXT2_GLOBAL;

 //
 // Flags for EXT2_GLOBAL_DATA
 //

 #define EXT2_SUPPORT_WRITING    0x00000002

 //
 // Driver Extension define
 //

 typedef struct 
 {
    EXT2_GLOBAL gExt2Global;
 } EXT2FS_EXT, *PEXT2FS_EXT;

 typedef struct _EXT2_FCBVCB 
 {
    // FCB header required by NT
    FSRTL_COMMON_FCB_HEADER         CommonFCBHeader;
    SECTION_OBJECT_POINTERS         SectionObject;
    ERESOURCE                       MainResource;
    ERESOURCE                       PagingIoResource;
    // end FCB header required by NT
    
    // Identifier for this structure
    EXT2_IDENTIFIER                  Identifier;
 } EXT2_FCBVCB, *PEXT2_FCBVCB;

 //
 // EXT2_VCB Volume Control Block.
 //
 // Data that represents a mounted logical volume.
 // It is allocated as the device extension of the volume device object.
 //

 typedef struct _EXT2_VCB 
 {
    // FCB header required by NT
    // The VCB is also used as an FCB for file objects
    // that represents the volume itself
    FSRTL_COMMON_FCB_HEADER     CommonFCBHeader;
    SECTION_OBJECT_POINTERS     SectionObject;
    ERESOURCE                   MainResource;
    ERESOURCE                   PagingIoResource;
    // end FCB header required by NT
    
    // Identifier for this structure
    EXT2_IDENTIFIER             Identifier;
    
    LIST_ENTRY                  Next;
    
    // Share Access for the file object
    SHARE_ACCESS                ShareAccess;

    // Count Lock
    ERESOURCE                   CountResource;

    // Incremented on IRP_MJ_CREATE, decremented on IRP_MJ_CLEANUP
    // for files on this volume.
    ULONG                       OpenFileHandleCount;
    
    // Incremented on IRP_MJ_CREATE, decremented on IRP_MJ_CLOSE
    // for both files on this volume and open instances of the
    // volume itself.
    ULONG                       ReferenceCount;
    ULONG                       OpenHandleCount;
    
    // Pointer to the VPB in the target device object
    PVPB                        Vpb;

    // List of FCBs for open files on this volume
    LIST_ENTRY                  FcbList;

    // List of IRPs pending on directory change notify requests
    LIST_ENTRY                  NotifyList;

    // Pointer to syncronization primitive for this list
    PNOTIFY_SYNC                NotifySync;
    
    // This volumes device object
    PDEVICE_OBJECT              DeviceObject;
    
    // The physical device object (the disk)
    PDEVICE_OBJECT              TargetDeviceObject;
    
    // Information about the physical device object
    DISK_GEOMETRY               DiskGeometry;
    PARTITION_INFORMATION       PartitionInformation;
    
    PEXT2_SUPER_BLOCK           ext2_super_block;
    PEXT2_GROUP_DESC            ext2_group_desc;
    
    // Number of Group Decsciptions
    ULONG                       ext2_groups;

    // Bitmap Block per group
    //PRTL_BITMAP                 BlockBitMaps;
    //PRTL_BITMAP                 InodeBitMaps;
    
	// Block and fragment size
    ULONG                       ext2_block;
    ULONG                       ext2_frag;
    
    // Flags for the volume
    ULONG                       Flags;

    // Streaming File Object
    PFILE_OBJECT                StreamObj;

    // Dirty Mcbs of modifications for volume stream
    LARGE_MCB                   DirtyMcbs;

    // Entry of Mcb Tree (Root Node)
    PEXT2_MCB                   Ext2McbTree;
 } EXT2_VCB, *PEXT2_VCB;

 //
 // Flags for EXT2_VCB
 //

 #define VCB_INITIALIZED         0x00000001
 #define VCB_VOLUME_LOCKED       0x00000002
 #define VCB_DISMOUNT_PENDING    0x00000004
 #define VCB_READ_ONLY           0x00000008
 #define VCB_DISMOUNTED          0x00000010

 //
 // EXT2_FCB File Control Block
 //
 // Data that represents an open file
 // There is a single instance of the FCB for every open file
 //

 typedef struct _EXT2_FCB 
 {
    // FCB header required by NT
    FSRTL_COMMON_FCB_HEADER         CommonFCBHeader;
    SECTION_OBJECT_POINTERS         SectionObject;
    ERESOURCE                       MainResource;
    ERESOURCE                       PagingIoResource;
    // end FCB header required by NT
    
    // Identifier for this structure
    EXT2_IDENTIFIER                 Identifier;
    
    // List of FCBs for this volume
    LIST_ENTRY                      Next;
    
    // Share Access for the file object
    SHARE_ACCESS                    ShareAccess;

    // Count Lock
    ERESOURCE                       CountResource;

    // List of byte-range locks for this file
    FILE_LOCK                       FileLockAnchor;

    // Incremented on IRP_MJ_CREATE, decremented on IRP_MJ_CLEANUP
    ULONG                           OpenHandleCount;
    
    // Incremented on IRP_MJ_CREATE, decremented on IRP_MJ_CLOSE
    ULONG                           ReferenceCount;

    // Incremented on IRP_MJ_CREATE, decremented on IRP_MJ_CLEANUP
    // But only for Files with FO_NO_INTERMEDIATE_BUFFERING flag
    ULONG                           NonCachedOpenCount;

    // Flags for the FCB
    ULONG                           Flags;
    
    // Pointer to the inode
    PEXT2_INODE                     ext2_inode;

    // Hint block for next allocation
    ULONG                           BlkHint;
    
    // Vcb

    PEXT2_VCB                       Vcb;

    // Mcb Node ...
    PEXT2_MCB                       Ext2Mcb;

 #if DBG
    // The filename
    ANSI_STRING                     AnsiFileName;   
 #endif
 } EXT2_FCB, *PEXT2_FCB;

 //
 // Flags for EXT2_FCB
 //

 #define FCB_FROM_POOL               0x00000001
 #define FCB_PAGE_FILE               0x00000002
 #define FCB_DELETE_ON_CLOSE         0x00000004
 #define FCB_DELETE_PENDING          0x00000008
 #define FCB_FILE_DELETED            0x00000010
 #define FCB_FILE_MODIFIED           0x00000020

 //
 // Mcb Node
 //

 struct _EXT2_MCB 
 {
    // Identifier for this structure
    EXT2_IDENTIFIER                 Identifier;

    // Flags
    ULONG                           Flags;

    // Link List Info

    PEXT2_MCB                       Parent; // Parent
    PEXT2_MCB                       Child;  // Children
    PEXT2_MCB                       Next;   // Brothers

    // Mcb Node Info

    // Ticker Count when created
    LONGLONG                        TickerCount;

    // -> Fcb
    PEXT2_FCB                       Ext2Fcb;

    // Short name
    UNICODE_STRING                  ShortName;

    // Inode number
    ULONG                           Inode;

    // Dir entry offset in parent
    ULONG                           DeOffset;

    // File attribute
    ULONG                           FileAttr;
 };

 //
 // Flags for MCB
 //

 #define MCB_FROM_POOL               0x00000001
 #define MCB_IN_TREE                 0x00000002

 //
 // EXT2_CCB Context Control Block
 //
 // Data that represents one instance of an open file
 // There is one instance of the CCB for every instance of an open file
 //

 typedef struct _EXT2_CCB 
 {
    // Identifier for this structure
    EXT2_IDENTIFIER  Identifier;

    // Flags
    ULONG               Flags;
    
    // State that may need to be maintained
    ULONG           CurrentByteOffset;
    UNICODE_STRING  DirectorySearchPattern;
 } EXT2_CCB, *PEXT2_CCB;

 //
 // Flags for CCB
 //

 #define CCB_FROM_POOL               0x00000001

 //
 // REPINNED_BCBS List
 //

 #define EXT2_REPINNED_BCBS_ARRAY_SIZE         (8)

 typedef struct _EXT2_REPINNED_BCBS 
 {
    //
    //  A pointer to the next structure contains additional repinned bcbs
    //

    struct _EXT2_REPINNED_BCBS *Next;

    //
    //  A fixed size array of pinned bcbs.  Whenever a new bcb is added to
    //  the repinned bcb structure it is added to this array.  If the
    //  array is already full then another repinned bcb structure is allocated
    //  and pointed to with Next.
    //

    PBCB Bcb[EXT2_REPINNED_BCBS_ARRAY_SIZE];

 } EXT2_REPINNED_BCBS, *PEXT2_REPINNED_BCBS;

 //
 // EXT2_IRP_CONTEXT
 //
 // Used to pass information about a request between the drivers functions
 //

 typedef struct _EXT2_IRP_CONTEXT 
 {
    // Identifier for this structure
    EXT2_IDENTIFIER     Identifier;
    
    // Pointer to the IRP this request describes
    PIRP                Irp;

    // Flags
    ULONG               Flags;
    
    // The major and minor function code for the request
    UCHAR               MajorFunction;
    UCHAR               MinorFunction;
    
    // The device object
    PDEVICE_OBJECT      DeviceObject;
    
    // The file object
    PFILE_OBJECT        FileObject;
    
    // If the request is synchronous (we are allowed to block)
    BOOLEAN             IsSynchronous;
    
    // If the request is top level
    BOOLEAN             IsTopLevel;
    
    // Used if the request needs to be queued for later processing
    WORK_QUEUE_ITEM     WorkQueueItem;
    
    // If an exception is currently in progress
    BOOLEAN             ExceptionInProgress;
    
    // The exception code when an exception is in progress
    NTSTATUS            ExceptionCode;

    // Repinned BCBs List
    EXT2_REPINNED_BCBS  Repinned;
 } EXT2_IRP_CONTEXT, *PEXT2_IRP_CONTEXT;

 #define IRP_CONTEXT_FLAG_FROM_POOL       (0x00000001)
 #define IRP_CONTEXT_FLAG_WAIT            (0x00000002)
 #define IRP_CONTEXT_FLAG_WRITE_THROUGH   (0x00000004)
 #define IRP_CONTEXT_FLAG_FLOPPY          (0x00000008)
 #define IRP_CONTEXT_FLAG_RECURSIVE_CALL  (0x00000010)
 #define IRP_CONTEXT_FLAG_DISABLE_POPUPS  (0x00000020)
 #define IRP_CONTEXT_FLAG_DEFERRED_WRITE  (0x00000040)
 #define IRP_CONTEXT_FLAG_VERIFY_READ     (0x00000080)
 #define IRP_CONTEXT_STACK_IO_CONTEXT     (0x00000100)
 #define IRP_CONTEXT_FLAG_IN_FSP          (0x00000200)
 #define IRP_CONTEXT_FLAG_USER_IO         (0x00000400)
 #define IRP_CONTEXT_FLAG_QUEUED_REQ      (0x00000800)

 //
 // EXT2_ALLOC_HEADER
 //
 // In the checked version of the driver this header is put in the beginning of
 // every memory allocation
 //

 typedef struct _EXT2_ALLOC_HEADER 
 {
    EXT2_IDENTIFIER Identifier;
 } EXT2_ALLOC_HEADER, *PEXT2_ALLOC_HEADER;

 typedef struct _FCB_LIST_ENTRY 
 {
    PEXT2_FCB    Fcb;
    LIST_ENTRY   Next;
 } FCB_LIST_ENTRY, *PFCB_LIST_ENTRY;

 //
 // Block Description List
 //

 typedef struct _EXT2_BDL 
 {
    LONGLONG    Lba;
    ULONG       Offset;
    ULONG       Length;
    PIRP        Irp;
 } EXT2_BDL, *PEXT2_BDL;

 #pragma pack()

 /////////////////////////////////////////////////////////////////////////////

 // Function protos.

 //
 //  The following macro is used to determine if an FSD thread can block
 //  for I/O or wait for a resource.  It returns TRUE if the thread can
 //  block and FALSE otherwise.  This attribute can then be used to call
 //  the FSD & FSP common work routine with the proper wait value.
 //

 #define Ext2CanWait(IRP) IoIsOperationSynchronous(Irp)

 // util.c

 LARGE_INTEGER 
 Ext2SysTime (
	IN ULONG i_time );

 ULONG 
 Ext2InodeTime (
	IN LARGE_INTEGER	SysTime );

 VOID 
 Ext2SyncUninitializeCacheMap (
	IN PFILE_OBJECT		FileObject );

 BOOLEAN 
 Ext2CopyRead(
    IN PFILE_OBJECT		FileObject,
    IN PLARGE_INTEGER	FileOffset,
    IN ULONG			Length,
    IN BOOLEAN			Wait,
    OUT PVOID			Buffer,
    OUT PIO_STATUS_BLOCK  IoStatus );

 // block.c

 NTSTATUS
 Ext2ReadWriteBlocks(
    IN PEXT2_IRP_CONTEXT IrpContext,
    IN PEXT2_VCB        Vcb,
    IN PEXT2_BDL        Ext2BDL,
    IN ULONG            Length,
    IN ULONG            Count,
    IN BOOLEAN          bVerify );

 NTSTATUS
 Ext2ReadSync(
    IN PDEVICE_OBJECT   DeviceObject,
    IN LONGLONG         Offset,
    IN ULONG            Length,
    OUT PVOID           Buffer,
    BOOLEAN             bVerify );

 NTSTATUS
 Ext2ReadDisk(
	IN PDEVICE_OBJECT	pDeviceObject,
    IN ULONG			lba,
    IN ULONG			offset,
    IN ULONG			Size,
    IN PVOID			Buffer );

 NTSTATUS 
 Ext2DiskIoControl (
	IN PDEVICE_OBJECT   pDeviceObject,
    IN ULONG            IoctlCode,
    IN PVOID            InputBuffer,
    IN ULONG            InputBufferSize,
    IN OUT PVOID        OutputBuffer,
    IN OUT PULONG       OutputBufferSize );

 NTSTATUS
 Ext2DiskShutDown(
	PEXT2_VCB Vcb );

 NTSTATUS
 Ext2ReadDiskOverrideVerify (
	IN PDEVICE_OBJECT	pDeviceObject,
    IN ULONG			DiskSector, 
    IN ULONG			SectorCount,
    IN OUT PUCHAR		Buffer );

 // ext2.c

 PEXT2_SUPER_BLOCK
 Ext2LoadSuper(
	IN PDEVICE_OBJECT pDeviceObject );

 BOOLEAN
 Ext2SaveSuper(  
	IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb );

 PEXT2_GROUP_DESC
 Ext2LoadGroup(
	IN PEXT2_VCB vcb );

 BOOLEAN
 Ext2SaveGroup(  
	IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb       );
 
 BOOLEAN
 Ext2GetInodeLba (
	IN PEXT2_VCB vcb,
    IN ULONG inode,
    OUT PLONGLONG offset );

 BOOLEAN
 Ext2LoadInode (
	IN PEXT2_VCB vcb,
    IN ULONG inode,
    IN PEXT2_INODE ext2_inode );

 BOOLEAN
 Ext2SaveInode (
	IN PEXT2_IRP_CONTEXT IrpContext,
    IN PEXT2_VCB vcb,
    IN ULONG inode,
    IN PEXT2_INODE ext2_inode);

 BOOLEAN
 Ext2LoadBlock (
    IN PEXT2_VCB Vcb,
    IN ULONG     dwBlk,
    IN PVOID     Buffer);

 BOOLEAN
 Ext2SaveBlock ( 
	IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN ULONG                dwBlk,
    IN PVOID                Buf );

 BOOLEAN
 Ext2SaveBuffer( 
	IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN LONGLONG             Offset,
    IN ULONG                Size,
    IN PVOID                Buf );

 ULONG  
 Ext2GetBlock(
	IN PEXT2_VCB	vcb,
    ULONG			dwContent,
    ULONG			Index,
    int				layer );

 ULONG 
 Ext2BlockMap(
	IN PEXT2_VCB vcb,
    IN PEXT2_INODE ext2_inode,
    IN ULONG Index );

 ULONG 
 Ext2BuildBDL(
	IN PEXT2_IRP_CONTEXT    IrpContext,
	IN PEXT2_VCB			Vcb,
	IN PEXT2_INODE			ext2_inode,
	IN ULONG				offset, 
	IN ULONG				size, 
	OUT PEXT2_BDL*			ext2_bdl );

 BOOLEAN 
 Ext2NewBlock(   
	PEXT2_IRP_CONTEXT	IrpContext,
    PEXT2_VCB			Vcb,
    ULONG				GroupHint,
    ULONG				BlockHint,  
    PULONG				dwRet );

 BOOLEAN 
 Ext2FreeBlock(  
	PEXT2_IRP_CONTEXT	IrpContext,
    PEXT2_VCB			Vcb,
    ULONG				Block );

 BOOLEAN 
 Ext2ExpandBlock(
    PEXT2_IRP_CONTEXT	IrpContext,
    PEXT2_VCB			Vcb,
    PEXT2_FCB			Fcb,
    ULONG				dwContent,
    ULONG				Index,
    ULONG				layer,
    BOOLEAN				bNew,
    ULONG				*dwRet );


 BOOLEAN 
 Ext2ExpandInode(
	PEXT2_IRP_CONTEXT	IrpContext,
    PEXT2_VCB			Vcb,
    PEXT2_FCB			Fcb,
    ULONG				*dwRet );

 BOOLEAN
 Ext2NewInode(
    PEXT2_IRP_CONTEXT	IrpContext,
    PEXT2_VCB			Vcb,
    ULONG				GroupHint,
    ULONG				mode,
    PULONG				Inode );

 BOOLEAN
 Ext2FreeInode(
    PEXT2_IRP_CONTEXT	IrpContext,
    PEXT2_VCB			Vcb,
    ULONG				Inode,
    ULONG				Type );

 NTSTATUS
 Ext2AddEntry (
	IN PEXT2_IRP_CONTEXT   IrpContext,
	IN PEXT2_VCB           Vcb,
	IN PEXT2_FCB           Dcb,
	IN ULONG               FileType,
	IN ULONG               Inode,
	IN PUNICODE_STRING     FileName );

 NTSTATUS
 Ext2RemoveEntry (
	IN PEXT2_IRP_CONTEXT   IrpContext,
	IN PEXT2_VCB           Vcb,
	IN PEXT2_FCB           Dcb,
	IN ULONG               Inode );

 BOOLEAN 
 Ext2TruncateBlock(
    PEXT2_IRP_CONTEXT		IrpContext,
    PEXT2_VCB				Vcb,
    ULONG					dwContent,
    ULONG					Index,
    ULONG					layer,
    BOOLEAN					*bFreed );

 BOOLEAN
 Ext2TruncateInode(
    PEXT2_IRP_CONTEXT	IrpContext,
    PEXT2_VCB			Vcb,
    PEXT2_FCB			Fcb );

 BOOLEAN
 Ext2AddMcbEntry (
    IN PEXT2_VCB Vcb,
    IN LONGLONG  Lba,
    IN LONGLONG  Length );

 VOID
 Ext2RemoveMcbEntry (
    IN PEXT2_VCB Vcb,
    IN LONGLONG  Lba,
    IN LONGLONG  Length );

 BOOLEAN
 Ext2LookupMcbEntry (
    IN PEXT2_VCB    Vcb,
    IN LONGLONG     Offset,
    OUT PLONGLONG   Lba OPTIONAL,
    OUT PLONGLONG   Length OPTIONAL,
    OUT PLONGLONG   RunStart OPTIONAL,
    OUT PLONGLONG   RunLength OPTIONAL,
    OUT PULONG      Index OPTIONAL );

 // read.c

 NTSTATUS
 Ext2ReadInode (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB			vcb,
    IN PEXT2_INODE			ext2_inode,
    IN ULONG				offset,
    IN PVOID				Buffer,
    IN ULONG				size,
    OUT PULONG				dwReturn);

 // write.c

 NTSTATUS
 Ext2WriteInode (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN PEXT2_INODE          ext2_inode,
    IN ULONG                offset,
    IN PVOID                Buffer,
    IN ULONG                size,
    IN BOOLEAN              bWriteToDisk,
    OUT PULONG              dwReturn);

 // memory.c

 PEXT2_IRP_CONTEXT
 Ext2AllocateIrpContext (
	IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp );

 VOID
 Ext2FreeIrpContext (
	IN PEXT2_IRP_CONTEXT IrpContext );

 VOID
 Ext2RepinBcb (
    IN PEXT2_IRP_CONTEXT IrpContext,
    IN PBCB Bcb );

 VOID
 Ext2UnpinRepinnedBcbs (
    IN PEXT2_IRP_CONTEXT IrpContext );

 PEXT2_FCB
 Ext2AllocateFcb (
	IN PEXT2_VCB		Vcb,
	IN PEXT2_MCB       Ext2Mcb,
	IN PEXT2_INODE     ext2_inode );

 VOID
 Ext2FreeFcb (
	IN PEXT2_FCB Fcb );

 PEXT2_CCB
 Ext2AllocateCcb (VOID);

 VOID
 Ext2FreeMcb (
	IN PEXT2_MCB Mcb );

 PEXT2_FCB
 Ext2CreateFcbFromMcb(
	PEXT2_VCB Vcb, 
	PEXT2_MCB Mcb );

 VOID
 Ext2FreeCcb (
	IN PEXT2_CCB Ccb );

 PEXT2_MCB
 Ext2AllocateMcb (
	PEXT2_VCB		Vcb, 
	PUNICODE_STRING FileName, 
	ULONG			FileAttr );

 PEXT2_MCB
 Ext2SearchMcbTree(
	PEXT2_MCB	Ext2Mcb, 
	ULONG		Inode);

 PEXT2_MCB
 Ext2SearchMcb(
	PEXT2_MCB		Parent, 
	PUNICODE_STRING FileName);

 BOOLEAN
 Ext2GetFullFileName(
	PEXT2_MCB		Mcb, 
	PUNICODE_STRING FileName);

 VOID
 Ext2AddMcbNode(
	PEXT2_MCB Parent, 
	PEXT2_MCB Child );

 BOOLEAN
 Ext2DeleteMcbNode(
	PEXT2_MCB Ext2McbTree, 
	PEXT2_MCB Ext2Mcb);

 int Ext2GetMcbDepth(
	PEXT2_MCB Ext2Mcb );

 BOOLEAN
 Ext2CompareMcb(
	PEXT2_MCB Ext2Mcb1, 
	PEXT2_MCB Ext2Mcb2);

 VOID
 Ext2FindUnusedMcb (
	PEXT2_MCB	Ext2McbTree, 
	PEXT2_MCB	*Ext2Mcb);

 VOID
 Ext2FreeMcbTree(
	PEXT2_MCB McbTree );

 BOOLEAN
 Ext2CheckSetBlock(
	PEXT2_IRP_CONTEXT	IrpContext, 
	PEXT2_VCB			Vcb, 
	ULONG Block );

 BOOLEAN
 Ext2CheckBitmapConsistency(
	PEXT2_IRP_CONTEXT IrpContext, 
	PEXT2_VCB Vcb );

 NTSTATUS
 Ext2InitializeVcb(
	IN PEXT2_IRP_CONTEXT	IrpContext, 
	PEXT2_VCB				Vcb, 
    PEXT2_SUPER_BLOCK		Ext2Sb, 
	PDEVICE_OBJECT			TargetDevice,
    PDEVICE_OBJECT			VolumeDevice, 
	PVPB					Vpb);

 VOID
 Ext2FreeVcb (
	IN PEXT2_VCB Vcb );

 // fastio.c

 BOOLEAN
 Ext2FastIoQueryBasicInfo (
	IN PFILE_OBJECT             FileObject,
	IN BOOLEAN                  Wait,
	OUT PFILE_BASIC_INFORMATION Buffer,
	OUT PIO_STATUS_BLOCK        IoStatus,
	IN PDEVICE_OBJECT           DeviceObject );

 BOOLEAN
 Ext2FastIoQueryStandardInfo (
	IN PFILE_OBJECT                 FileObject,
	IN BOOLEAN                      Wait,
	OUT PFILE_STANDARD_INFORMATION  Buffer,
	OUT PIO_STATUS_BLOCK            IoStatus,
	IN PDEVICE_OBJECT               DeviceObject );

 BOOLEAN
 Ext2FastIoLock (
	IN PFILE_OBJECT         FileObject,
	IN PLARGE_INTEGER       FileOffset,
	IN PLARGE_INTEGER       Length,
	IN PEPROCESS            Process,
	IN ULONG                Key,
	IN BOOLEAN              FailImmediately,
	IN BOOLEAN              ExclusiveLock,
	OUT PIO_STATUS_BLOCK    IoStatus,
	IN PDEVICE_OBJECT       DeviceObject   );

 BOOLEAN
 Ext2FastIoUnlockSingle (
	IN PFILE_OBJECT         FileObject,
	IN PLARGE_INTEGER       FileOffset,
	IN PLARGE_INTEGER       Length,
	IN PEPROCESS            Process,
	IN ULONG                Key,
	OUT PIO_STATUS_BLOCK    IoStatus,
	IN PDEVICE_OBJECT       DeviceObject  );

 BOOLEAN
 Ext2FastIoUnlockAll (
    IN PFILE_OBJECT         FileObject,
    IN PEPROCESS            Process,
    OUT PIO_STATUS_BLOCK    IoStatus,
    IN PDEVICE_OBJECT       DeviceObject  );

 BOOLEAN
 Ext2FastIoUnlockAllByKey (
	IN PFILE_OBJECT         FileObject,
	IN PEPROCESS            Process,
	IN ULONG                Key,
	OUT PIO_STATUS_BLOCK    IoStatus,
	IN PDEVICE_OBJECT       DeviceObject  );

 BOOLEAN
 Ext2FastIoQueryNetworkOpenInfo (
	IN PFILE_OBJECT                     FileObject,
	IN BOOLEAN                          Wait,
	OUT PFILE_NETWORK_OPEN_INFORMATION  Buffer,
	OUT PIO_STATUS_BLOCK                IoStatus,
	IN PDEVICE_OBJECT                   DeviceObject );

 BOOLEAN
 Ext2FastIoQueryNetworkOpenInfo (
	IN PFILE_OBJECT                     FileObject,
    IN BOOLEAN                          Wait,
    OUT PFILE_NETWORK_OPEN_INFORMATION  Buffer,
    OUT PIO_STATUS_BLOCK                IoStatus,
    IN PDEVICE_OBJECT                   DeviceObject );

 // lock.c

 NTSTATUS
 Ext2LockControl (
	IN PEXT2_IRP_CONTEXT IrpContext );

 // cmcb.c

 BOOLEAN
 Ext2AcquireForLazyWrite (
	IN PVOID    Context,
    IN BOOLEAN  Wait );

 VOID
 Ext2ReleaseFromLazyWrite (
	IN PVOID Context );

 BOOLEAN
 Ext2AcquireForReadAhead (
	IN PVOID    Context,
    IN BOOLEAN  Wait );

 BOOLEAN
 Ext2NoOpAcquire (
	IN PVOID Fcb,
    IN BOOLEAN Wait );

 VOID
 Ext2NoOpRelease (
	IN PVOID Fcb    );

 VOID
 Ext2ReleaseFromReadAhead (
	IN PVOID Context );

 NTSTATUS
 Ext2Close (
	IN PEXT2_IRP_CONTEXT IrpContext );

 VOID
 Ext2QueueCloseRequest (
	IN PEXT2_IRP_CONTEXT IrpContext );

 VOID
 Ext2DeQueueCloseRequest (
	IN PVOID Context );

 PEXT2_FCB
 Ext2SearchFcbList(	
	IN PEXT2_VCB    Vcb,
	IN ULONG        inode);

 NTSTATUS
 Ext2ScanDir (
	IN PEXT2_VCB		Vcb,
    IN PEXT2_MCB        ParentMcb,
    IN PUNICODE_STRING  FileName,
	IN OUT PULONG       Index,
	IN PEXT2_INODE      ext2_inode,
	IN PEXT2_DIR_ENTRY2 dir_entry);

 NTSTATUS
 Ext2LookupFileName (
	IN PEXT2_VCB    Vcb,
	IN PUNICODE_STRING		FullFileName,
    IN PEXT2_FCB            ParentFcb,
    OUT PEXT2_MCB *         Ext2Mcb,
	IN OUT PEXT2_INODE		ext2_inode);

 NTSTATUS
 Ext2CreateFile(
	IN PEXT2_IRP_CONTEXT IrpContext, 
	PEXT2_VCB			 Vcb );

 NTSTATUS
 Ext2CreateVolume(
	IN PEXT2_IRP_CONTEXT IrpContext, 
	PEXT2_VCB Vcb );

 NTSTATUS
 Ext2Create (
	IN PEXT2_IRP_CONTEXT IrpContext );

 NTSTATUS
 Ext2CreateInode(
    PEXT2_IRP_CONTEXT   IrpContext,
    PEXT2_VCB           Vcb,
    PEXT2_FCB           pParentFcb,
    ULONG               Type,
    ULONG               FileAttr,
    PUNICODE_STRING     FileName);

 NTSTATUS
 Ext2Read (
	IN PEXT2_IRP_CONTEXT IrpContext );

 // Fileinfo.c

 NTSTATUS
 Ext2QueryInformation (
	IN PEXT2_IRP_CONTEXT IrpContext );

 NTSTATUS
 Ext2SetInformation (
	IN PEXT2_IRP_CONTEXT IrpContext);

 BOOLEAN
 Ext2ExpandFileAllocation (
    PEXT2_IRP_CONTEXT IrpContext,
    PEXT2_VCB Vcb,
    PEXT2_FCB Fcb,
    PLARGE_INTEGER AllocationSize );

 BOOLEAN
 Ext2TruncateFileAllocation (
    PEXT2_IRP_CONTEXT IrpContext,
    PEXT2_VCB Vcb,
    PEXT2_FCB Fcb,
    PLARGE_INTEGER AllocationSize );

 NTSTATUS
 Ext2SetDispositionInfo(
    PEXT2_IRP_CONTEXT IrpContext,
    PEXT2_VCB Vcb,
    PEXT2_FCB Fcb,
    BOOLEAN bDelete );

 NTSTATUS
 Ext2SetRenameInfo(
    PEXT2_IRP_CONTEXT IrpContext,
    PEXT2_VCB Vcb,
    PEXT2_FCB Fcb );

 // Volinfo.c
 
 NTSTATUS
 Ext2QueryVolumeInformation (
	IN PEXT2_IRP_CONTEXT IrpContext );

 NTSTATUS
 Ext2SetVolumeInformation (
	IN PEXT2_IRP_CONTEXT IrpContext );

 NTSTATUS
 Ext2CharToWchar (
	IN OUT PWCHAR   Destination,
	IN PCHAR        Source,
	IN ULONG        Length );

 NTSTATUS
 Ext2WcharToChar (
	IN OUT PCHAR    Destination,
	IN PWCHAR       Source,
	IN ULONG        Length);

 NTSTATUS
 Ext2LockUserBuffer (
	IN PIRP             Irp,
    IN ULONG            Length,
    IN LOCK_OPERATION   Operation);

 PVOID
 Ext2GetUserBuffer (
	IN PIRP Irp );

 ULONG
 Ext2GetInfoLength(
	IN FILE_INFORMATION_CLASS  FileInformationClass );

 ULONG
 Ext2ProcessDirEntry(
	IN PEXT2_VCB         Vcb,
    IN FILE_INFORMATION_CLASS  FileInformationClass,
    IN ULONG         in,
    IN PVOID         Buffer,
    IN ULONG         UsedLength,
    IN ULONG         Length,
    IN ULONG         FileIndex,
    IN UNICODE_STRING*   pName,
    IN BOOLEAN       Single );

 NTSTATUS
 Ext2QueryDirectory (
	IN PEXT2_IRP_CONTEXT IrpContext );

 NTSTATUS
 Ext2NotifyChangeDirectory (
    IN PEXT2_IRP_CONTEXT IrpContext    );

 NTSTATUS
 Ext2DirectoryControl (
	IN PEXT2_IRP_CONTEXT IrpContext);

 NTSTATUS
 Ext2CompleteIrpContext (
    IN PEXT2_IRP_CONTEXT IrpContext,
    IN NTSTATUS Status );

 NTSTATUS
 Ext2QueueRequest (
	IN PEXT2_IRP_CONTEXT IrpContext );

 VOID
 Ext2DeQueueRequest (
	IN PVOID Context );

 NTSTATUS
 Ext2Cleanup (
	IN PEXT2_IRP_CONTEXT IrpContext );
 
 NTSTATUS
 Ext2DeviceControlNormal (
	IN PEXT2_IRP_CONTEXT IrpContext );

 NTSTATUS
 Ext2PrepareToUnload (
	IN PEXT2_IRP_CONTEXT IrpContext );

 NTSTATUS
 Ext2DeviceControl (
	IN PEXT2_IRP_CONTEXT IrpContext );

 NTSTATUS
 Ext2VerifyVolume (
	IN PEXT2_IRP_CONTEXT IrpContext );
 
 NTSTATUS
 Ext2IsVolumeMounted (
	IN PEXT2_IRP_CONTEXT IrpContext );
 
 NTSTATUS
 Ext2DismountVolume (
	IN PEXT2_IRP_CONTEXT IrpContext );
 
 NTSTATUS
 Ext2PurgeVolume (
	IN PEXT2_VCB Vcb,
    IN BOOLEAN  FlushBeforePurge);

 NTSTATUS
 Ext2PurgeFile (
	IN PEXT2_FCB Fcb,
    IN BOOLEAN   FlushBeforePurge);

 NTSTATUS
 Ext2LockVolume (
	IN PEXT2_IRP_CONTEXT IrpContext );
 
 NTSTATUS
 Ext2UnlockVolume (
	IN PEXT2_IRP_CONTEXT IrpContext );

 NTSTATUS
 Ext2UserFsRequest (
	IN PEXT2_IRP_CONTEXT IrpContext );

 NTSTATUS
 Ext2MountVolume (
	IN PEXT2_IRP_CONTEXT IrpContext );

 NTSTATUS
 Ext2FileSystemControl (
	IN PEXT2_IRP_CONTEXT IrpContext );

 NTSTATUS
 Ext2DispatchRequest (
	IN PEXT2_IRP_CONTEXT IrpContext );

 NTSTATUS
 Ext2ExceptionFilter (
	IN PEXT2_IRP_CONTEXT    IrpContext,
	IN NTSTATUS             ExceptionCode );

 NTSTATUS
 Ext2ExceptionHandler (
	IN PEXT2_IRP_CONTEXT IrpContext );

 VOID
 Ext2SetVpbFlag (
	IN PVPB     Vpb,
    IN USHORT   Flag );

 VOID
 Ext2ClearVpbFlag (
	IN PVPB     Vpb,
    IN USHORT   Flag );

 NTSTATUS
 Ext2BuildRequest (
	IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp);

 // init.c

 extern PEXT2_GLOBAL	gExt2Global;

 BOOLEAN
 Ext2QueryRegistry( 
 	IN PUNICODE_STRING  RegistryPath, 
 	PULONG WritingSupport );

 NTSTATUS DriverEntry (
	IN PDRIVER_OBJECT   DriverObject,
	IN PUNICODE_STRING  RegistryPath );

 BOOLEAN
 Ext2FastIoCheckIfPossible (
	IN PFILE_OBJECT         FileObject,
	IN PLARGE_INTEGER       FileOffset,
	IN ULONG                Length,
	IN BOOLEAN              Wait,
	IN ULONG                LockKey,
	IN BOOLEAN              CheckForReadOperation,
	OUT PIO_STATUS_BLOCK    IoStatus,
	IN PDEVICE_OBJECT       DeviceObject );

 PSZ
 Ext2DbgNtStatusToString (
	IN NTSTATUS Status );

 #if DBG
  
	 BOOLEAN
	 Ext2FastIoRead (
		IN PFILE_OBJECT         FileObject,
		IN PLARGE_INTEGER       FileOffset,
		IN ULONG                Length,
		IN BOOLEAN              Wait,
		IN ULONG                LockKey,
		OUT PVOID               Buffer,
		OUT PIO_STATUS_BLOCK    IoStatus,
		IN PDEVICE_OBJECT       DeviceObject);
 
	 BOOLEAN
	 Ext2FastIoWrite (
		IN PFILE_OBJECT         FileObject,
		IN PLARGE_INTEGER       FileOffset,
		IN ULONG                Length,
		IN BOOLEAN              Wait,
		IN ULONG                LockKey,
		OUT PVOID               Buffer,
		OUT PIO_STATUS_BLOCK    IoStatus,
		IN PDEVICE_OBJECT       DeviceObject );

	#define Ext2CompleteRequest(Irp, PriorityBoost)		\
			Ext2DbgPrintComplete(Irp);					\
			IoCompleteRequest(Irp, PriorityBoost)

 #else

	 #define Ext2CompleteRequest(Irp, PriorityBoost)	\
		    IoCompleteRequest(Irp, PriorityBoost)

 #endif // DBG

 // Flush.c
 
 NTSTATUS
 Ext2FlushVolume (
	IN PEXT2_VCB	Vcb, 
	BOOLEAN			bShutDown );

 NTSTATUS
 Ext2FlushFile (
	IN PEXT2_FCB Fcb );

 NTSTATUS
 Ext2Flush (
	IN PEXT2_IRP_CONTEXT IrpContext );

 // Shutdown.c

 NTSTATUS
 Ext2ShutDown (
	IN PEXT2_IRP_CONTEXT IrpContext );

 // Write.c

 BOOLEAN
 Ext2ZeroHoles (
    IN PEXT2_IRP_CONTEXT	IrpContext,
    IN PEXT2_VCB			Vcb,
    IN PFILE_OBJECT			FileObject,
    IN LONGLONG				Offset,
    IN LONGLONG				Count );

 NTSTATUS
 Ext2Write (
	IN PEXT2_IRP_CONTEXT IrpContext );

 BOOLEAN
 Ext2SupersedeOrOverWriteFile(
    PEXT2_IRP_CONTEXT	IrpContext,
    PEXT2_VCB			Vcb,
    PEXT2_FCB			Fcb,
    ULONG				Disposition );

 BOOLEAN Ext2IsDirectoryEmpty (
    PEXT2_VCB Vcb,
    PEXT2_FCB Fcb );

 BOOLEAN
 Ext2DeleteFile(
    PEXT2_IRP_CONTEXT IrpContext,
    PEXT2_VCB Vcb,
    PEXT2_FCB Fcb );

 //
 // These decls. are missed out at times.
 //

 #if (_WIN32_WINNT == 0x0500)

 USHORT
 FASTCALL
 RtlUshortByteSwap(
    IN USHORT Source    );

 ULONG
 FASTCALL
 RtlUlongByteSwap(
    IN ULONG Source    );

 ULONGLONG
 FASTCALL
 RtlUlonglongByteSwap(
    IN ULONGLONG Source    );

 #endif // (_WIN32_WINNT >= 0x0500)

 /////////////////////////////////////////////////////////////////////////////
 
 #endif		// __EXT2FS_H