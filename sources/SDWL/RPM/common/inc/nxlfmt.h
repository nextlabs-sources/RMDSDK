#ifndef NXL_FILE_EXTENTION
#define NXL_FILE_EXTENTION	".nxl"
#define NXL_FILE_EXTENTIONW	L".nxl"
#endif


#ifndef __NXL_FORMAT_H__
#define __NXL_FORMAT_H__


/*

NXL Format Specification


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//          Version 2.0
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    Descriptions:
        The second version NXL format.

    Changes:
        - Change magic code to indicate NXL format version
        - Change crypt header because of encryption protocol change

    Details
        
        >>> NXL Header Format

        =========================================================================================================================================
        |   +Offset         +Size           +Name                   +Description                                                                |
        |_______________________________________________________________________________________________________________________________________|
        |   ### Signature Header  ###                                                                                                           |
        |   0x0             0x8             Magic Code              A magic code indicates NXL header   "NXLFMT@\0"                             |
        |   0x8             0x4             Version                 NXL format version                                                          |
        |                                                               High-word: Major version                                                |
        |                                                               Low-word:  Minor version                                                |
        |   0xC             0xF4            Message                 An UTF-8 message can be viewed by user (Up to 244 bytes, include last '\0') |
        |---------------------------------------------------------------------------------------------------------------------------------------|
        |   ### File Header ###                                                                                                                 |
        |   0x100           0x10            UDID                    The unique ID of this document                                              |
        |   0x110           0x4             Flags                   Document flags                                                              |
        |   0x114           0x4             Alignment               Data alignment of this document                                             |
        |   0x118           0x4             Algorithm               The algorithm to encrypt content                                            |
        |   0x11C           0x4             CBC Size                The CBC size of the algorithm                                               |
        |   0x120           0x4             Content Offset          The offset of document content                                              |
        |   0x124           0x100           Owner Id                Owner's member id (UTF-8 string, up to 255 bytes)                           |
        |   0x224           0x4             Extended Data Offset    The offset of extend file header data                                       |
        |---------------------------------------------------------------------------------------------------------------------------------------|
        |   ### Key Header  ###                                                                                                                 |
        |   0x228           0x4             Flags                   The highest byte - Secure Mode                                              |
        |                                                               0x01 - Client Token Mode                                                |
        |                                                               0x02 - Server Token Mode                                                |
        |                                                               0x03 - Split Token Mode                                                 |
        |                                                           The lower-order 3 bytes - Key Header Flags                                  |
        |                                                               0x000001 - KF_RECOVERY_KEY_ENABLED                                      |
        |   0x22C           0x10            IV Seed                 Random Ivec Seed (used to generate IV for AES algorithm)                    |
        |   0x23C           0x40            Token Protected CEK     0 - 31 Bytes: CEK encrypted by token                                        |
        |                                                           32 - 63 Bytes: CEK Verification Code HMAC_SHA256(CEK, Token)                |
        |   0x27C           0x40            Recovery password       0 - 31 Bytes: CEK encrypted by recovery key                                 |
        |                                   protected CEK           32 - 63 Bytes: CEK Verification Code HMAC_SHA256(CEK, RecoveryKey)          |
        |   0x2BC           0x100           Public Key #1           The public key between member and root                                      |
        |   0x3BC           0x100           Public Key #2           The public key between member and iCA                                       |
        |   0x4BC           0x4             Token Level             Token's maintenance level                                                   |
        |   0x4C0           0x4             Extended Data Offset    The offset of extend key header data                                        |
        |---------------------------------------------------------------------------------------------------------------------------------------|
        |   ### Section Header  ###                                                                                                             |
        |   -> Section table                                                                                                                    |
        |   0x4C4           0x4             Section map             A bits map indicating which sections are valid                              |
        |   0x4C8           0x40            Section Record 0        Section 0, File Internal Attributes (".FileInfo", UTF-8, JSON)              |
        |   0x508           0x40            Section Record 1        Section 1, Ad-hoc policy (".AdHoc", UTF-8, JSON)                            |
        |   0x548           0x40            Section Record 2        Section 2, File tags (".FileTag", UTF-8, JSON)                              |
        |   0x588           0x40            Section Record 3        Section 3, user defined                                                     |
        |   0x5C8           0x40            Section Record 4        Section 4, user defined                                                     |
        |   0x608           0x40            Section Record 5        Section 5, user defined                                                     |
        |   0x648           0x40            Section Record 6        Section 6, user defined                                                     |
        |   0x688           0x40            Section Record 7        Section 7, user defined                                                     |
        |     .              .                  .                   ...                                                                         |
        |   0xC88           0x40            Section Record 31       Section 31, user defined                                                    |
        |---------------------------------------------------------------------------------------------------------------------------------------|
        |   ### Extended Data  ###                                                                                                              |
        |   0xCC8           0x310           Reserved                Zero                                                                        |
        |---------------------------------------------------------------------------------------------------------------------------------------|
        |   ### Dynamic Header  ###                                                                                                             |
        |   0xFD8           0x20            Header Hash             Hash of fixed header (File/Key/Section)                                     |
        |                                                           HMAC_SHA256(Length, Header), encrypted by Token                             |
        |   0xFF8           0x8             Content Length          The length of document content (original document content)                  |
        |---------------------------------------------------------------------------------------------------------------------------------------|
        |   ### Section Data  ###                                                                                                               |
        |   -> Section ".FileInfo"  (Built-in)                                                                                                  |
        |   0x1000          0x1000          Data                    Section data (4096 bytes)                                                   |
        |   -> Section ".Ad-hoc"    (Built-in)                                                                                                  |
        |   0x2000          0x1000          Data                    Section data (4096 bytes)                                                   |
        |   -> Section ".FileTag"   (Built-in)                                                                                                  |
        |   0x3000          0x1000          Data                    Section data (4096 bytes)                                                   |
        |---------------------------------------------------------------------------------------------------------------------------------------|
        |   ### Content  ###                                                                                                                    |
        |   0x4000          $(ContentLen)   Cipher Data             Encrypted document content                                                  |
        |_______________________________________________________________________________________________________________________________________|

        NOTE:
            (1)   Three built-in sections' size are default value, it can be other value defined by user (but must be aligned with 4096).

        >>> NXL Section Format
        =========================================================================================================================================
        |   +Offset         +Size           +Name                   +Description                                                                |
        |_______________________________________________________________________________________________________________________________________|
        |   0x0             0x10            Section Name            UTF-8 section name, up to 15 characters                                     |
        |   0x10            0x4             Section flags           Flags                                                                       |
        |                                                               0x00000001 - encrypted (by CEK)                                         |
        |                                                               0x00000002 - zipped                                                     |
        |   0x14            0x4             Start offset            Section start offset                                                        |
        |   0x18            0x4             Section size            Section size                                                                |
        |   0x1C            0x2             Data size               Original data size                                                          |
        |   0x1E            0x2             Compressed size         Compressed data size                                                        |
        |   0x20            0x20            Checksum                HMAC_SHA256(Length, Data) data checksum                                     |
        |_______________________________________________________________________________________________________________________________________|




/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//          Version 1.0
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    Descriptions:
        The first version NXL format.

    Details

        =====================================================================================================================================
        |   +Offset         +Size           +Name               +Description                                                                |
        |___________________________________________________________________________________________________________________________________|
        |   ### Signature Header  ###                                                                                                       |
        |   0x0             0x8             Magic Code          A magic code indicates NXL header   "NXLFMT!\0"                             |
        |   0x8             0x88            Message             A Unicode message can be viewed by user                                     |
        |-----------------------------------------------------------------------------------------------------------------------------------|
        |   ### Basic Info Header ###                                                                                                       |
        |   0x90            0x10            Thumb print         A unique ID of this document                                                |
        |   0xA0            0x4             Version             Version of NXL format used by this document                                 |
        |   0xA4            0x4             Flags               Version of NXL format used by this document                                 |
        |   0xA8            0x4             Alignment           Data alignment in this document                                             |
        |   0xAC            0x4             Point of Content    The offset of encrypted document data                                       |
        |-----------------------------------------------------------------------------------------------------------------------------------|
        |   ### Crypto Header  ###                                                                                                          |
        |   0xB0            0x4             Algorithm           The algorithm to encrypt content                                            |
        |   0xB4            0x4             CBC Size            The CBC size of the algorithm                                               |
        |   -> Primary Key Blob  (3 items)                                                                                                  |
        |   0xB8            0x2             KEK Algorithm       The algorithm to encrypt CEK                                                |
        |   0xBA            0x2             KEK Id Size         The size of primary KEK ID                                                  |
        |   0xBC            0x3C            KEK Id Data         The content of primary KEK ID                                               |
        |   0xF8            0x100           Encrypted CEK       CEK encrypted by primary KEK                                                |
        |   -> Recovery Key Blob (3 items)                                                                                                  |
        |   0x1F8           0x2             KEK Algorithm       The algorithm to encrypt CEK                                                |
        |   0x1FA           0x2             KEK Id Size         The size of recovery KEK ID                                                 |
        |   0x1FC           0x3C            KEK Id Data         The content of recovery KEK ID                                              |
        |   0x238           0x100           Encrypted CEK       CEK encrypted by recovery KEK                                               |
        |   0x338           0x8             Content Length      The real length of document content                                         |
        |   0x340           0x8             Allocation Length   The allocated size of document content (aligned with document alignment)    |
        |   0x348           0x20            Content Padding     Unused                                                                      |
        |-----------------------------------------------------------------------------------------------------------------------------------|
        |   ### Section Header  ###                                                                                                         |
        |   -> Section table                                                                                                                |
        |   0x368           0x10            Checksum            Section table checksum (encrypted by first 16 bits of CEK)                  |
        |   0x378           0x4             Section count       Valid sections in the table                                                 |
        |   0x37C           0x4             Reserved            Reserved data, must be zero                                                 |
        |       + Section ".Attrs"                                                                                                          |
        |   0x380           0x8             Name                Section Name ".Attrs"                                                       |
        |   0x388           0x4             Size                Section size, 2048 bytes                                                    |
        |   0x38C           0x4             Checksum            CRC32 checksum of section data                                              |
        |       + Section ".Rights"                                                                                                         |
        |   0x390           0x8             Name                Section Name ".Rights"                                                      |
        |   0x398           0x4             Size                Section size, 4096 bytes                                                    |
        |   0x39C           0x4             Checksum            CRC32 checksum of section data                                              |
        |       + Section ".Tags"                                                                                                           |
        |   0x3A0           0x8             Name                Section Name ".Tags"                                                        |
        |   0x3A8           0x4             Size                Section size, 4096 bytes                                                    |
        |   0x3AC           0x4             Checksum            CRC32 checksum of section data                                              |
        |       + User defined sections count = 72                                                                                          |
        |   0x3B0           0x450           Sections (72)       User defined sections (up to 72)                                            |
        |-----------------------------------------------------------------------------------------------------------------------------------|
        |   ### Section Data  ###                                                                                                           |
        |   -> Section ".Attrs"                                                                                                             |
        |   0x800           0x800           Data                Section data (2048 bytes)                                                   |
        |   -> Section ".Rights"                                                                                                            |
        |   0x1000          0x1000          Data                Section data (4096 bytes)                                                   |
        |   -> Section ".tags"                                                                                                              |
        |   0x2000          0x1000          Data                Section data (4096 bytes)                                                   |
        |-----------------------------------------------------------------------------------------------------------------------------------|
        |   ### Section Data  ###                                                                                                           |
        |   0x3000          $(ContentLeng)  Cipher Data         Encrypted document content                                                  |
        |___________________________________________________________________________________________________________________________________|

*/


#ifdef __cplusplus
extern "C" {
#endif


//
//
//
#define NXLFMT_VERSION_1            0x0001
#define NXLFMT_VERSION_2            0x0002


#define NXL_MAGIC_NAME_1            "NXLFMT!"
#define NXL_MAGIC_CODE_1            0x0021544D464C584EULL

#define NXL_MAGIC_NAME_2            "NXLFMT@"
#define NXL_MAGIC_CODE_2            0x0040544D464C584EULL


// Key Header Flags
#define KF_RECOVERY_KEY_ENABLED     0x00000001


// Secure Mode
#define KF_CLIENT_SECURE_MODE       0x01
#define KF_SERVER_SECURE_MODE       0x02
#define KF_SPLIT_SECURE_MODE        0x03

//
//  Struct Alignment: 4
//
#pragma pack(push)
#pragma pack(4)

// Signature Header
typedef union _NXL_MAGIC {
    unsigned __int64    code;
    unsigned char       name[8];
} NXL_MAGIC;

typedef struct _NXL_SIGNATURE_1 {
    NXL_MAGIC   magic;
    wchar_t     message[68];
} NXL_SIGNATURE_1;

typedef struct _NXL_SIGNATURE_LITE {
    NXL_MAGIC       magic;
    unsigned long   version;
} NXL_SIGNATURE_LITE;

typedef struct _NXL_SIGNATURE_2 {
    NXL_MAGIC       magic;
    unsigned long   version;
    char            message[244];
} NXL_SIGNATURE_2;

// File Info Header

typedef struct _NXL_FILEINFO_1 {
    unsigned char   thumb_print[16];
    unsigned long   version;
    unsigned long   flags;
    unsigned long   alignment;
    unsigned long   content_offset;
} NXL_FILEINFO_1;

typedef struct _NXL_FILEINFO_2 {
    unsigned char   duid[16];
    unsigned long   flags;
    unsigned long   alignment;
    unsigned long   algorithm;
    unsigned long   block_size;
    unsigned long   content_offset;
    char            owner_id[256];
    unsigned long   extended_data_offset;
} NXL_FILEINFO_2;


// Crypto/Key Header

typedef struct _NXL_KEK_BLOB {
    unsigned short  key_algorithm;
    unsigned short  key_id_size;
    unsigned char   key_id[60];
    unsigned char   cek[256];
} NXL_KEK_BLOB;

typedef struct _NXL_CRYPTO {
    unsigned long   algorithm;
    unsigned long   cbc_size;
    NXL_KEK_BLOB    primary_key;
    NXL_KEK_BLOB    recovery_key;
    __int64         content_length;
    __int64         allocation_length;
    unsigned char   reserved[32];
} NXL_CRYPTO;

typedef struct _NXL_KEYS {
    unsigned long   mode_and_flags;
    unsigned char   iv_seed[16];
    unsigned char   token_cek[32];
    unsigned char   token_cek_checksum[32];
    unsigned char   recovery_cek[32];
    unsigned char   recovery_cek_checksum[32];
    unsigned char   public_key1[256];
    unsigned char   public_key2[256];
	unsigned long	token_level;
    unsigned long   extended_data_offset;
} NXL_KEYS;


// Section Header

typedef struct _NXL_SECTION_1 {
    char            name[8];
    unsigned long   size;
    unsigned long   checksum;
} NXL_SECTION_1;

typedef struct _NXL_SECTION_2 {
    char            name[16];
    unsigned long   flags;
    unsigned long   offset;
    unsigned long   size;
    unsigned short  data_size;
    unsigned short  compressed_size;
    unsigned char   checksum[32];   // HMAC_SHA256, encrypted by token
} NXL_SECTION_2;

typedef struct _NXL_SECTION_HEADER_1 {
    unsigned char   checksum[16];
    unsigned long   count;
    unsigned long   reserved;
    NXL_SECTION_1   record[75];
} NXL_SECTION_HEADER_1;

typedef struct _NXL_SECTION_HEADER_2 {
    unsigned long   section_map;
    NXL_SECTION_2   record[32];
} NXL_SECTION_HEADER_2;

// Fixed & Dynamic Header

typedef struct _NXL_FIXED_HEADER {
    NXL_SIGNATURE_2         signature;
    NXL_FILEINFO_2          file_info;
    NXL_KEYS                keys;
    NXL_SECTION_HEADER_2    sections;
    unsigned char           extend_data[784];
} NXL_FIXED_HEADER;

typedef struct _NXL_DYNAMIC_HEADER {
    unsigned char   fixed_header_hash[32];
    __int64         content_length;
} NXL_DYNAMIC_HEADER;

// Header
typedef struct _NXL_HEADER_1 {
    NXL_SIGNATURE_1         signature;
    NXL_FILEINFO_1          file_info;
    NXL_CRYPTO              crypto;
    NXL_SECTION_HEADER_1    sections;
} NXL_HEADER_1;

typedef struct _NXL_HEADER_2 {
    NXL_FIXED_HEADER    fixed;
    NXL_DYNAMIC_HEADER  dynamic;
} NXL_HEADER_2;
typedef const NXL_HEADER_2* PCNXL_HEADER_2;


typedef struct _NXL_TOKEN {
	unsigned char	Token[32];
	unsigned long	TokenSecureMode;
	unsigned long	TokenLevel;
} NXL_TOKEN;

typedef struct _NXL_CEK {
	unsigned char	Cek[32];
} NXL_CEK;

typedef struct _NXL_UDID {
	unsigned char   Udid[16];
}NXL_UDID;

#pragma pack(pop)   // End of Struct Alignment: 4



//
//  Define default Version & Revision (always up-to-date)
//
#ifndef NXL_VERSION
#define NXL_VERSION     2
#endif

#ifndef NXL_REVISION
#define NXL_REVISION    0
#endif


//
//  
//
#if (NXL_VERSION == 1)

#define NXL_MAGIC_NAME  NXL_MAGIC_NAME_1
#define NXL_MAGIC_CODE  NXL_MAGIC_CODE_1
#define NXL_HEADER      NXL_HEADER_1
#define PNXL_HEADER     NXL_HEADER_1*
#define PCNXL_HEADER    const NXL_HEADER_1*

#else   // #if (NXL_VERSION == 1)

#define NXL_MAGIC_NAME  NXL_MAGIC_NAME_2
#define NXL_MAGIC_CODE  NXL_MAGIC_CODE_2
#define NXL_HEADER      NXL_HEADER_2
#define PNXL_HEADER     NXL_HEADER_2*
#define PCNXL_HEADER    const NXL_HEADER_2*

#endif   // #if (NXL_VERSION == 1)

#define NXL_PAGE_SIZE           0x1000                  /**< NXL Format Page Size */
#define NXL_BLOCK_SIZE          0x200                   /**< NXL Format CBC Block Size */
#define NXL_MIN_SIZE			0x4000                  /**< NXL Format Minimum File Size */

#define NXL_MAX_SECTION_DATA_LENGTH 0xFFFF              // The original data length must not exceed 64K bytes

#define NXL_VERSION_10          0x00010000              /**< NXL Format Version 1.0 */
#define NXL_VERSION_30          0x00030000              /**< NXL Format Version 3.0 */

enum _NXLALGORITHM {
	NXL_ALGORITHM_NONE = 0,			/**< No algorithm (No encrypted) */
	NXL_ALGORITHM_AES128 = 1,		/**< AES 128 bits */
	NXL_ALGORITHM_AES256 = 2,		/**< AES 256 bits (Default content encryption algorithm) */
	NXL_ALGORITHM_RSA1024 = 3,		/**< RSA 1024 bits */
	NXL_ALGORITHM_RSA2048 = 4,		/**< RSA 2048 bits */
	NXL_ALGORITHM_SHA1 = 5,			/**< SHA1 */
	NXL_ALGORITHM_SHA256 = 6,		/**< SHA256 (Default hash algorithm) */
	NXL_ALGORITHM_MD5 = 7			/**< MD5 */
};

#define NXL_DEFAULT_MSG         "Protected by SkyDRM.com"   /**< NXL Format default message */

#define NXL_SECTION_FLAG_ENCRYPTED				(0x00000001)
#define NXL_SECTION_FLAG_COMPRESSED				(0x00000002)

#define NXL_SECTION_NAME_FILEINFO               ".FileInfo"
#define NXL_SECTION_NAME_FILEPOLICY             ".FilePolicy"
#define NXL_SECTION_NAME_FILETAG                ".FileTag"

#ifdef __cplusplus
}
#endif

#endif