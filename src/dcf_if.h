/**
 * @file		dcf_if.h
 * @brief		User interface of FileSystem function with depend on DCF(header).
 * @note		None
 * @attention	None
 * 
 * <B><I>Copyright 2015 Socionext Inc.</I></B>
 */
#ifndef _DCF_IF_H_
#define _DCF_IF_H_

typedef signed short		FW_SHORT;
typedef unsigned short		FW_USHORT;
typedef signed long 		FW_LONG;
typedef unsigned long		FW_ULONG;
typedef signed long long	FW_LLONG;
typedef unsigned long long	FW_ULLONG;
typedef char				FW_CHAR;
typedef unsigned char		FW_UCHAR;
typedef int					FW_INT32;
typedef unsigned int 	 	FW_UINT32;
typedef long long			FW_INT64;
typedef unsigned long long	FW_UINT64;
typedef double				FW_DOUBLE;
typedef float				FW_FLOAT;

/*---------------------------------------------------------------*/
/* Definition													 */
/*---------------------------------------------------------------*/
// for MPF -------------------------------------------
#define CO_BF_DCF_IF_MPF_ENABLE
//#define CO_BF_DCF_IF_JPG_SAVE_PADD_ON
#define CO_BF_DCF_IF_MPF_DISPARITY_ENABLE

#define CO_BF_DCF_IF_ADD_EXIF_INFO
#define CO_BF_DCF_IF_GET_GPSTAG
#define CO_BF_DCF_IF_SET_GPSTAG
// for Jpeg-HDR --------------------------------------
#define CO_BF_DCF_IF_APP11_ENABLE

/* for name of fullpass and object, extenstion */
#define D_BF_DCF_IF_FULLPASS_FILENAME_MAX		(766)
#define D_BF_DCF_IF_OBJ_NAME_MAX				(19)
#define D_BF_DCF_IF_EXT_MAX						(4)

/* for DCF DB */
#define D_BF_DCF_IF_TOP_DIR_NAME_MAX			(D_BF_FS_IF_DRVNAME_MAX+7)	// "[DriveName=8]:\DCIM\"
#define D_BF_DCF_IF_DIR_NAME_MAX				(9)
#define D_BF_DCF_IF_OBJ_ONLY_NAME_MAX			(9)
#define D_BF_DCF_IF_DIR_NAME_ONLY				(6)	// 5 charactor + NULL
#define D_BF_DCF_IF_FILE_NAME_ONLY				(5)	// 4 charactor + NULL
#define D_BF_DCF_IF_FILE_PATH_WO_DRVNAME		(28)	// DCF File path size without drive name length.  :\DCIM\100AAAAA\BBBB0001.JPG

/* action mode for BF_Dcf_If_Set_Current_Dir() */
#define D_BF_DCF_IF_PRM_NORMAL_ACTION			(1)
//#define D_BF_DCF_IF_PRM_CURRENT_ONLY			(2)
//#define D_BF_DCF_IF_PRM_PREV_NEXT_ONLY		(4)

#define D_BF_DCF_IF_ROOT_DIR_NAME				"DCIM"

#define D_BF_DCF_IF_FILE_TYPE_JPG_MAIN			(0x00)		// This type means a main image in the JPG file.
#define D_BF_DCF_IF_FILE_TYPE_JPG_SCRNAIL		(0x01)		// This type means a screen nail image(640x480) in the JPG file.
#define D_BF_DCF_IF_FILE_TYPE_JPG_THM			(0x02)		// This type means a thumbnail image(160x120) in the JPG file.
#define D_BF_DCF_IF_FILE_TYPE_VIDEO_MAIN		(0x10)		// This type means a main video file.             "TECB0001.MP4"
#define D_BF_DCF_IF_FILE_TYPE_VIDEO_SUB			(0x11)		// This type means a sub video file.              "TECS0001.MP4"
#define D_BF_DCF_IF_FILE_TYPE_VIDEO_THM			(0x12)		// This type means a thumbnail file of video fil. "TECB0001.THM"
#define D_BF_DCF_IF_FILE_TYPE_VIDEO_TOP			(0x13)		// This type means top image of " TECB0001.MP4"

/******************************************************************************
 Global Function

 <sample>
  "entry_func" implement is as follows:
	FW_VOID Sample_Func(FW_INT32 result)
	{
		;
	}

******************************************************************************/

/*---------------------------------------------------------------*/
/* Enumeration													 */
/*---------------------------------------------------------------*/
/**
 * @breif exposure progrem for DCF
 * @note none
 * @attention none
 */
typedef enum {
	E_BF_DCF_IF_EXPOSURE_NOT_DEFINED = 0,
	E_BF_DCF_IF_EXPOSURE_MANUAL = 1,
	E_BF_DCF_IF_EXPOSURE_NORMAL = 2,
	E_BF_DCF_IF_EXPOSURE_APERTURE = 3,
	E_BF_DCF_IF_EXPOSURE_SHUTTER = 4,
	E_BF_DCF_IF_EXPOSURE_CREATE = 5,
	E_BF_DCF_IF_EXPOSURE_ACTION = 6,
	E_BF_DCF_IF_EXPOSURE_PORTRAIT = 7,
	E_BF_DCF_IF_EXPOSURE_LANDSCAPE = 8
} E_BF_DCF_IF_EXPOSURE;

/**
 * @breif metering mode for DCF
 * @note none
 * @attention the following values are exif tag
 */
typedef enum {
	E_BF_DCF_IF_METERING_UNKNOWN = 0,			/**< unknown */
	E_BF_DCF_IF_METERING_AVERAGE = 1,			/**< average */
	E_BF_DCF_IF_METERING_CENTER_WEIGHTED = 2,	/**< center weighted average */
	E_BF_DCF_IF_METERING_SPOT = 3,			/**< spot */
	E_BF_DCF_IF_METERING_MULTI_SPOT = 4,		/**< multi spot */
	E_BF_DCF_IF_METERING_MULTI = 5,			/**< pattern */
	E_BF_DCF_IF_METERING_PATTERN = 5,			/**< pattern */
	E_BF_DCF_IF_METERING_PARTIAL = 6,			/**< partial */
	E_BF_DCF_IF_METERING_OTHER = 255			/**< other */
} E_BF_DCF_IF_METERING;

/**
 * white balance
 */
/**
 * @breif white balance mode
 * @note none
 * @attention none
 */
typedef enum {
	E_BF_DCF_IF_WB_MANUAL = 0,       /**< manual */
	E_BF_DCF_IF_WB_AUTO = 1,         /**< auto */
	E_BF_DCF_IF_WB_DAYLIGHT = 2,     /**< daylight */
	E_BF_DCF_IF_WB_CLOUDY = 3,       /**< cloudy */
	E_BF_DCF_IF_WB_FLOURESCENT = 4,  /**< flourescent */
	E_BF_DCF_IF_WB_TUNGSTEN = 5      /**< tungsten */
} E_BF_DCF_IF_WB;

/**
 * @breif scene capture type for DCF
 * @note none
 * @attention none
 */
typedef enum {
    E_BF_DCF_IF_SCENE_STANDARD = 0,
    E_BF_DCF_IF_SCENE_LANDSCAPE = 1,
    E_BF_DCF_IF_SCENE_PORTRAIT = 2,
    E_BF_DCF_IF_SCENE_NIGHT = 3
} E_BF_DCF_IF_SCENE;

/**
 * @breif file extension type for DCF
 * @note none
 * @attention none
 */
typedef enum {
	E_BF_DCF_IF_FILE_TYPE_NOTHING = 0x00,
	E_BF_DCF_IF_FILE_TYPE_JPG = 0x01,
//#if ( CO_MOVIE_FF_AUDIO_PATTERN == 0 )
	E_BF_DCF_IF_FILE_TYPE_MP4 = 0x02,
//#else
	E_BF_DCF_IF_FILE_TYPE_MOV = 0x04,
//#endif
	E_BF_DCF_IF_FILE_TYPE_RAW = 0x08,
	E_BF_DCF_IF_FILE_TYPE_WAV = 0x10,
	E_BF_DCF_IF_FILE_TYPE_THM = 0x20,
	E_BF_DCF_IF_FILE_TYPE_AVI = 0x40,
	E_BF_DCF_IF_FILE_TYPE_TIF = 0x80
} E_BF_DCF_IF_FILE_TYPE;

/**
 * @breif file index type for DCF
 * @note none
 * @attention none
 */
typedef enum {
	E_BF_DCF_IF_FILE_INDEX_TYPE_PHOTO = 0x01, // jpeg,tiff
	E_BF_DCF_IF_FILE_INDEX_TYPE_MOVIE = 0x02 // mov
} E_BF_DCF_IF_FILE_INDEX_TYPE;

/**
 * @breif file attrubute for DCF
 * @note none
 * @attention none
 */
typedef enum {
	E_BF_DCF_IF_ATTR_NORMAL = 0x00,
	E_BF_DCF_IF_ATTR_RDONLY = 0x01
} E_BF_DCF_IF_ATTR;

/**
 * @breif file duplication status for DCF
 * @note none
 * @attention none
 */
typedef enum {
	E_BF_DCF_IF_UNDUPLICATE = 0x00,
	E_BF_DCF_IF_DUPLICATE = 0x01
} E_BF_DCF_IF_DUPLICATION_STATUS;

/**
 * @breif voice memo exist flag for DCF
 * @note none
 * @attention none
 */
typedef enum {
	E_BF_DCF_IF_OPTION_FILE_NOTHING = 0x00,
	E_BF_DCF_IF_OPTION_FILE_EXIST = 0x01
} E_BF_DCF_IF_OPTION_FILE;

/**
 * @breif image orientation for DCF
 * @note none
 * @attention none
 */
typedef enum {
	E_BF_DCF_IF_ROTATE_0			 = 0x01,
	E_BF_DCF_IF_ROTATE_MIRROR_0		 = 0x02,
	E_BF_DCF_IF_ROTATE_180			 = 0x03,
	E_BF_DCF_IF_ROTATE_MIRROR_180	 = 0x04,
	E_BF_DCF_IF_ROTATE_MIRROR_90	 = 0x05,
	E_BF_DCF_IF_ROTATE_90			 = 0x06,
	E_BF_DCF_IF_ROTATE_MIRROR_270	 = 0x07,
	E_BF_DCF_IF_ROTATE_270			 = 0x08
} E_BF_DCF_IF_ROTATE;

/**
 * @breif DPOF setting information
 * @note none
 * @attention none
 */
typedef enum {
	E_BF_DCF_IF_DPOF_SET_OFF = 0x00,
	E_BF_DCF_IF_DPOF_SET_ON = 0x01
} E_BF_DCF_IF_DPOF_SET;

/**
 * @breif DPOF index setting information
 * @note none
 * @attention none
 */
typedef enum {
	E_BF_DCF_IF_DPOF_INDEX_NO = 0x00,
	E_BF_DCF_IF_DPOF_INDEX_YES = 0x01
} E_BF_DCF_IF_DPOF_INDEX;

/**
 * @breif DPOF size setting information
 * @note none
 * @attention none
 */
typedef enum {
	E_BF_DCF_IF_DPOF_SIZE_NO = 0x00,
	E_BF_DCF_IF_DPOF_SIZE_3X5 = 0x03,
	E_BF_DCF_IF_DPOF_SIZE_4X6 = 0x04,
	E_BF_DCF_IF_DPOF_SIZE_5X7 = 0x05,
	E_BF_DCF_IF_DPOF_SIZE_8X10 = 0x08
} E_BF_DCF_IF_DPOF_SIZE;

/**
 * @breif jpeg file format type
 * @note none
 * @attention none
 */
typedef enum {
	E_BF_DCF_IF_JPEG_FORMAT_EXIF = 0,
	E_BF_DCF_IF_JPEG_FORMAT_NOT_EXIF
} E_BF_DCF_IF_JPEG_FORMAT;

/**
 * @breif flag of execute remove, when file close
 * @note none
 * @attention none
 */
typedef enum {
	E_BF_DCF_IF_REMOVE_FLG_OFF = 0x00,
	E_BF_DCF_IF_REMOVE_FLG_ON = 0x01
} E_BF_DCF_IF_REMOVE_FLG;

/**
 * @breif File name setting
 * @note none
 * @attention none
 */
typedef enum {
	E_BF_DCF_IF_FILE_NAME_RESET = 0x00,
	E_BF_DCF_IF_FILE_NAME_SERIES = 0x01
} E_BF_DCF_IF_FILE_NAME;

/**
 * @breif qt type
 * @note none
 * @attention none
 */
typedef enum {
	E_BF_DCF_IF_QT_TYPE_MAIN = 0x00,
	E_BF_DCF_IF_QT_TYPE_THUMB = 0x01,
	E_BF_DCF_IF_QT_TYPE_VGA = 0x02
} E_BF_DCF_IF_QT_TYPE;

/**
 * @brief latest type.
 * @note none
 * @attention none.
 */
typedef enum {
	E_BF_DCF_IF_LATEST_TYPE_ALL = 0x00,
	E_BF_DCF_IF_LATEST_TYPE_MEDIA = 0x01,
	E_BF_DCF_IF_LATEST_TYPE_SPECIFIED = 0x02,
	E_BF_DCF_IF_LATEST_TYPE_SELECT = 0x03
} E_BF_DCF_IF_LATEST_TYPE;

/**
 * @brief DCF file create option.
 * @note none
 * @attention none.
 */
typedef enum {
	E_BF_DCF_IF_CREATE_UPDATE = 0x00,
	E_BF_DCF_IF_CREATE_NEW_FILE = 0x01
} E_BF_DCF_IF_CREATE;

/**
 * @brief Target that adds file
 * @note none
 * @attention none.
 */
typedef enum {
	E_BF_DCF_IF_UPDATE_DB = 0,			// update file in DCF DB and Date DB
	E_BF_DCF_IF_UPDATE_DATE_DB_COPY,	// update file in Date DB (for file operation copy)
	E_BF_DCF_IF_UPDATE_DATE_VOICE_MEMO	// update file in Date DB (for voice memo)
} E_BF_DCF_IF_UPDATE_KIND;

#ifdef CO_BF_DCF_IF_MPF_DISPARITY_ENABLE
/**
 * @brief MPF Extended MP Index
 * @note none
 * @attention none.
 */
typedef enum {
	E_BF_DCF_IF_MPF_EXT_MP_SELECT_1= 0,	
	E_BF_DCF_IF_MPF_EXT_MP_SELECT_2,
	E_BF_DCF_IF_MPF_EXT_MP_SELECT_MAX
} E_BF_DCF_IF_MPF_EXT_MP_SELECT;
#endif     // CO_BF_DCF_IF_MPF_DISPARITY_ENABLE

/**
 * @brief APP11 Marker Existence
 * @note none
 * @attention none.
 */
typedef enum {
	E_BF_DCF_IF_APP11_FLG_OFF = 0x00,
	E_BF_DCF_IF_APP11_FLG_ON  = 0x01
} E_BF_DCF_IF_APP11_FLG;

/**
 * @brief Data Type
 * @note none
 * @attention none.
 */
typedef enum {
	E_BF_DCF_IF_DATA_TYPE_RAW = 0x00,
	E_BF_DCF_IF_DATA_TYPE_DNG = 0x01
} E_BF_DCF_IF_DATA_TYPE;

/*---------------------------------------------------------------*/
/* Structure													 */
/*---------------------------------------------------------------*/
/**
 * @breif file index structure for DCF
 * @note none
 * @attention none
 */
/* file index */
typedef union {
	FW_LONG index;
	struct {
#ifdef FJ_CUSTOM_CPU_BIG_ENDIAN
		FW_SHORT dir_index;
		FW_SHORT obj_index;
#else
		FW_SHORT obj_index;
		FW_SHORT dir_index;
#endif
	} pack;
} T_BF_DCF_IF_FILE_INDEX;

/**
 * @breif object structure for DCF
 * @note none
 * @attention none
 */
/* object information */
typedef struct {
  FW_UCHAR extType;     // extension type. bit field.
  FW_UCHAR attr;         // file attribute
} T_BF_DCF_IF_DCFDB_OBJ_INFO;


/**
 * @breif date structure for DCF
 * @note none
 * @attention none
 */
typedef struct {
	FW_USHORT ad_year;			/* date A.D. year */
	FW_UCHAR month;				/* date month */
	FW_UCHAR day;				/* date day */
	FW_UCHAR hour;				/* date hour */
	FW_UCHAR min;				/* date minute */
	FW_UCHAR sec;				/* date second */
} T_BF_DCF_IF_DATE;

/**
 * @breif fraction structure for DCF
 * @note none
 * @attention none
 */
typedef struct {
	FW_ULONG nume;				/**< numerator */
	FW_ULONG denomi;			/**< denominator */
} T_BF_DCF_IF_FRACTION;

/**
 * @breif fraction structure for DCF 
 * @note none
 * @attention none
 */
typedef struct {
	FW_LONG nume;				/**< numerator */
	FW_ULONG denomi;			/**< denominator */
} T_BF_DCF_IF_SFRACTION;

/**
 * @breif date structure for GPS
 * @note none
 * @attention none
 */
typedef struct {
	FW_UCHAR latitude_ref;
	T_BF_DCF_IF_FRACTION latitude[3];
	FW_UCHAR longitude_ref;
	T_BF_DCF_IF_FRACTION longitude[3];
	FW_UCHAR img_direction_ref;
	T_BF_DCF_IF_FRACTION img_direction;
} T_BF_DCF_IF_GPS;

/**
 * @breif saved exif info structure for DCF
 * @note none
 * @attention none
 */
typedef struct {
	FW_USHORT width;		// image width
	FW_USHORT lines;		// image length
	FW_DOUBLE ev;
	T_BF_DCF_IF_DATE  date;
	T_BF_DCF_IF_FRACTION exposure_time;
	T_BF_DCF_IF_FRACTION f_value;
	E_BF_DCF_IF_EXPOSURE exposure_prog;
	T_BF_DCF_IF_FRACTION shutter_speed;
	T_BF_DCF_IF_FRACTION aperture;
	T_BF_DCF_IF_FRACTION max_aperture;
	E_BF_DCF_IF_METERING metering;
	E_BF_DCF_IF_WB light_source;
	FW_USHORT flash;
	T_BF_DCF_IF_FRACTION dzoom_ratio;
	FW_USHORT iso_value;
	T_BF_DCF_IF_FRACTION focal_length;
	FW_SHORT sharpness;
	FW_USHORT maker_note_length;
	FW_USHORT qvga_enable;
	T_BF_DCF_IF_GPS  gps;
} T_BF_DCF_IF_EXIF_INFO;

#endif