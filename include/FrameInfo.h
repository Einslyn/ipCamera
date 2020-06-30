<<<<<<< HEAD
#pragma once


typedef struct encode_frame_info
{
	unsigned int	keyFrame;
	int			nWidth;
	int			nHeight;
	unsigned int	time;

	int			frameIndex;

	unsigned int	dwLen;
	unsigned char	*pData;
}ENCODE_FRAME_INFO;

typedef struct decode_frameInfo
{
	int			nWidth;
	int			nHeight;
	unsigned int	time;

	unsigned int	dwLen;
	unsigned char	*pData;
}DECODE_FRAME_INFO;


=======
#pragma once


typedef struct encode_frame_info
{
	unsigned int	keyFrame;
	int			nWidth;
	int			nHeight;
	unsigned int	time;

	int			frameIndex;

	unsigned int	dwLen;
	unsigned char	*pData;
}ENCODE_FRAME_INFO;

typedef struct decode_frameInfo
{
	int			nWidth;
	int			nHeight;
	unsigned int	time;

	unsigned int	dwLen;
	unsigned char	*pData;
}DECODE_FRAME_INFO;


>>>>>>> a6d2214bda1c223f2b5a1caf2eccb7f94d6903ae
