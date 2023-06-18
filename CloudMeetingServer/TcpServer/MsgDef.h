#pragma once

#define BASE_BUFFER_SIZE 16
#define MSG_TYPE_SIZE 1
#define USER_SIZE 6
#define DATA_TYPE_SIZE 1
#define DATA_SIZE 8

enum MSG_TYPE {
	IMG_SEND = 0,
    IMG_RECV,
    AUDIO_SEND,
    AUDIO_RECV,
    TEXT_SEND,
    TEXT_RECV,
    CREATE_MEETING,
    EXIT_MEETING,
    JOIN_MEETING,
	CLOSE_CAMERA,

    LOGIN=11,
	REGISTER,
	LOGIN_RESPONSE,
	REGISTER_RESPONSE,

    CREATE_MEETING_RESPONSE = 20,
    PARTNER_EXIT = 21,
    PARTNER_JOIN = 22,
    JOIN_MEETING_RESPONSE = 23,
    PARTNER_JOIN2 = 24
};

enum IMAGE_FORMAT{
	Format_Invalid,
    Format_Mono,
    Format_MonoLSB,
    Format_Indexed8,
    Format_RGB32,
    Format_ARGB32,
    Format_ARGB32_Premultiplied,
    Format_RGB16,
    Format_ARGB8565_Premultiplied,
    Format_RGB666,
    Format_ARGB6666_Premultiplied,
    Format_RGB555,
    Format_ARGB8555_Premultiplied,
    Format_RGB888,
    Format_RGB444,
    Format_ARGB4444_Premultiplied,
    Format_RGBX8888,
    Format_RGBA8888,
    Format_RGBA8888_Premultiplied,
    Format_BGR30,
    Format_A2BGR30_Premultiplied,
    Format_RGB30,
    Format_A2RGB30_Premultiplied,
    Format_Alpha8,
    Format_Grayscale8,
    Format_RGBX64,
    Format_RGBA64,
    Format_RGBA64_Premultiplied,
    Format_Grayscale16,
    Format_BGR888,
    NImageFormats
};

struct MSG
{
    char *ptr;
    unsigned int len;
    MSG_TYPE msgType;
    unsigned int ip;
    IMAGE_FORMAT format;

    MSG()
    {

    }
    MSG(MSG_TYPE msg_type, char *msg, unsigned int length)
    {
        msgType = msg_type;
        ptr = msg;
        len = length;
    }
};
