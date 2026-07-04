#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>

#include "librtmp/rtmp.h"
#include "librtmp/rtmp_sys.h"
#include "librtmp/amf.h"

#ifdef WIN32
#include <windows.h>
#endif

#define FILEBUFSIZE (1024 * 1024 * 30)

typedef struct _NaluUnit {
    int type;
    int size;
    unsigned char *data;
} NaluUnit;

typedef struct _RTMPMetadata {
    unsigned int nWidth;
    unsigned int nHeight;
    unsigned int nFrameRate;
    unsigned int nVideoDataRate;
    unsigned int nSpsLen;
    unsigned char Sps[1024];
    unsigned int nPpsLen;
    unsigned char Pps[1024];
    int bHasAudio;
    unsigned int nAudioSampleRate;
    unsigned int nAudioSampleSize;
    unsigned int nAudioChannels;
    char pAudioSpecCfg;
    unsigned int nAudioSpecCfgLen;
} RTMPMetadata;

typedef struct _RTMPStream {
    RTMP* pRtmp;
    unsigned char* pFileBuf;
    unsigned int nFileBufSize;
    unsigned int nCurPos;
} RTMPStream;

enum {
    FLV_CODECID_H264 = 7,
};

static int InitSockets() {
#ifdef WIN32
    WORD version;
    WSADATA wsaData;
    version = MAKEWORD(1, 1);
    return (WSAStartup(version, &wsaData) == 0);
#else
    return 1;
#endif
}

static void CleanupSockets() {
#ifdef WIN32
    WSACleanup();
#endif
}

static char* put_byte(char* output, uint8_t nVal) {
    output[0] = nVal;
    return output + 1;
}

static char* put_be16(char* output, uint16_t nVal) {
    output[1] = nVal & 0xff;
    output[0] = nVal >> 8;
    return output + 2;
}

static char* put_be24(char* output, uint32_t nVal) {
    output[2] = nVal & 0xff;
    output[1] = nVal >> 8;
    output[0] = nVal >> 16;
    return output + 3;
}

static char* put_be32(char* output, uint32_t nVal) {
    output[3] = nVal & 0xff;
    output[2] = nVal >> 8;
    output[1] = nVal >> 16;
    output[0] = nVal >> 24;
    return output + 4;
}

static char* put_amf_string(char* c, const char* str) {
    uint16_t len = strlen(str);
    c = put_be16(c, len);
    memcpy(c, str, len);
    return c + len;
}

static char* put_amf_double(char* c, double d) {
    *c++ = AMF_NUMBER;
    unsigned char* ci = (unsigned char*)&d;
    unsigned char* co = (unsigned char*)c;
    co[0] = ci[7];
    co[1] = ci[6];
    co[2] = ci[5];
    co[3] = ci[4];
    co[4] = ci[3];
    co[5] = ci[2];
    co[6] = ci[1];
    co[7] = ci[0];
    return c + 8;
}

RTMPStream* RTMPStream_Create(void) {
    RTMPStream* stream = (RTMPStream*)malloc(sizeof(RTMPStream));
    if (!stream) {
        printf("[RTMP] ERROR: Failed to allocate RTMPStream\n");
        return NULL;
    }
    
    memset(stream, 0, sizeof(RTMPStream));
    
    printf("[RTMP] Creating RTMPStream instance\n");
    stream->pFileBuf = (unsigned char*)malloc(FILEBUFSIZE);
    if (!stream->pFileBuf) {
        printf("[RTMP] ERROR: Failed to allocate file buffer\n");
        free(stream);
        return NULL;
    }
    memset(stream->pFileBuf, 0, FILEBUFSIZE);
    printf("[RTMP] Allocated file buffer: %d bytes\n", FILEBUFSIZE);
    
    if (InitSockets()) {
        printf("[RTMP] Socket initialized successfully\n");
    } else {
        printf("[RTMP] Socket initialization failed\n");
    }
    
    stream->pRtmp = RTMP_Alloc();
    if (stream->pRtmp) {
        RTMP_Init(stream->pRtmp);
        printf("[RTMP] RTMP context allocated and initialized\n");
    } else {
        printf("[RTMP] Failed to allocate RTMP context\n");
        free(stream->pFileBuf);
        free(stream);
        return NULL;
    }
    
    return stream;
}

void RTMPStream_Destroy(RTMPStream* stream) {
    if (!stream) return;
    
    if (stream->pRtmp) {
        printf("[RTMP] Closing RTMP connection\n");
        RTMP_Close(stream->pRtmp);
        RTMP_Free(stream->pRtmp);
        printf("[RTMP] RTMP connection closed\n");
    }
    
    CleanupSockets();
    
    if (stream->pFileBuf) {
        free(stream->pFileBuf);
    }
    
    free(stream);
}

int RTMPStream_Connect(RTMPStream* stream, const char* url) {
    if (!stream || !url || !stream->pRtmp) {
        printf("[RTMP] ERROR: Invalid stream, URL or RTMP context\n");
        return -1;
    }
    
    printf("[RTMP] Setting up URL: %s\n", url);
    if (RTMP_SetupURL(stream->pRtmp, (char*)url) < 0) {
        printf("[RTMP] ERROR: Failed to setup URL\n");
        return -1;
    }
    
    RTMP_EnableWrite(stream->pRtmp);
    printf("[RTMP] Write mode enabled\n");
    
    printf("[RTMP] Connecting to server...\n");
    if (RTMP_Connect(stream->pRtmp, NULL) == 0) {
        printf("[RTMP] ERROR: Failed to connect to server\n");
        return -1;
    }
    printf("[RTMP] Connected to server successfully\n");
    
    printf("[RTMP] Connecting to stream...\n");
    if (RTMP_ConnectStream(stream->pRtmp, 0) == 0) {
        printf("[RTMP] ERROR: Failed to connect to stream\n");
        return -1;
    }
    printf("[RTMP] Connected to stream successfully\n");
    
    return 0;
}

int RTMPStream_SendPacket(RTMPStream* stream, unsigned int nPacketType, unsigned char* data, unsigned int size, unsigned int nTimestamp) {
    if (!stream || !stream->pRtmp) {
        printf("[RTMP] ERROR: SendPacket failed - RTMP context is NULL\n");
        return -1;
    }

    const char* type_name = "unknown";
    if (nPacketType == RTMP_PACKET_TYPE_AUDIO) type_name = "audio";
    else if (nPacketType == RTMP_PACKET_TYPE_VIDEO) type_name = "video";
    else if (nPacketType == RTMP_PACKET_TYPE_INFO) type_name = "info";

    RTMPPacket packet;
    RTMPPacket_Reset(&packet);
    RTMPPacket_Alloc(&packet, size);
    packet.m_packetType = nPacketType;
    packet.m_nChannel = (nPacketType == RTMP_PACKET_TYPE_AUDIO) ? 0x05 : 0x04;
    packet.m_headerType = RTMP_PACKET_SIZE_LARGE;
    packet.m_nTimeStamp = nTimestamp;
    packet.m_nInfoField2 = stream->pRtmp->m_stream_id;
    packet.m_nBodySize = size;
    memcpy(packet.m_body, data, size);

    int nRet = RTMP_SendPacket(stream->pRtmp, &packet, 0);
    RTMPPacket_Free(&packet);
    
    if (nRet != 0) {
        printf("[RTMP] Sent %s packet: size=%d, timestamp=%dms, channel=0x%02X\n", 
               type_name, size, nTimestamp, (nPacketType == RTMP_PACKET_TYPE_AUDIO) ? 0x05 : 0x04);
        return 0;
    } else {
        printf("[RTMP] ERROR: Failed to send %s packet\n", type_name);
        return -1;
    }
}

int RTMPStream_SendMetadata(RTMPStream* stream, RTMPMetadata* lpMetaData) {
    if (!stream || !lpMetaData) {
        printf("[RTMP] ERROR: SendMetadata failed - metadata is NULL\n");
        return -1;
    }

    printf("[RTMP] Sending metadata: width=%d, height=%d, framerate=%d\n", 
           lpMetaData->nWidth, lpMetaData->nHeight, lpMetaData->nFrameRate);
    printf("[RTMP] SPS length=%d, PPS length=%d\n", lpMetaData->nSpsLen, lpMetaData->nPpsLen);

    char body[1024] = { 0 };
    char* p = (char*)body;

    p = put_byte(p, AMF_STRING);
    p = put_amf_string(p, "@setDataFrame");
    p = put_byte(p, AMF_STRING);
    p = put_amf_string(p, "onMetaData");
    p = put_byte(p, AMF_OBJECT);

    p = put_amf_string(p, "copyright");
    p = put_byte(p, AMF_STRING);
    p = put_amf_string(p, "firehood");
    p = put_amf_string(p, "width");
    p = put_amf_double(p, lpMetaData->nWidth);
    p = put_amf_string(p, "height");
    p = put_amf_double(p, lpMetaData->nHeight);
    p = put_amf_string(p, "framerate");
    p = put_amf_double(p, lpMetaData->nFrameRate);
    p = put_amf_string(p, "videocodecid");
    p = put_amf_double(p, FLV_CODECID_H264);
    p = put_amf_string(p, "");
    p = put_byte(p, AMF_OBJECT_END);

    printf("[RTMP] Sending AMF metadata packet: size=%d bytes\n", (int)(p - body));
    RTMPStream_SendPacket(stream, RTMP_PACKET_TYPE_INFO, (unsigned char*)body, p - body, 0);

    int i = 0;
    body[i++] = 0x17;
    body[i++] = 0x00;
    body[i++] = 0x00;
    body[i++] = 0x00;
    body[i++] = 0x00;

    body[i++] = 0x01;
    body[i++] = lpMetaData->Sps[1];
    body[i++] = lpMetaData->Sps[2];
    body[i++] = lpMetaData->Sps[3];
    body[i++] = 0xff;

    body[i++] = 0xE1;
    body[i++] = lpMetaData->nSpsLen >> 8;
    body[i++] = lpMetaData->nSpsLen & 0xff;
    memcpy(&body[i], lpMetaData->Sps, lpMetaData->nSpsLen);
    i = i + lpMetaData->nSpsLen;

    body[i++] = 0x01;
    body[i++] = lpMetaData->nPpsLen >> 8;
    body[i++] = lpMetaData->nPpsLen & 0xff;
    memcpy(&body[i], lpMetaData->Pps, lpMetaData->nPpsLen);
    i = i + lpMetaData->nPpsLen;

    printf("[RTMP] Sending AVC sequence header: size=%d bytes\n", i);
    return RTMPStream_SendPacket(stream, RTMP_PACKET_TYPE_VIDEO, (unsigned char*)body, i, 0);
}

int RTMPStream_SendH264Packet(RTMPStream* stream, unsigned char* data, unsigned int size, int bIsKeyFrame, unsigned int nTimeStamp) {
    if (!stream || !data || size < 1) {
        printf("[H264] ERROR: Invalid data or size (%d)\n", size);
        return -1;
    }

    unsigned char* body = (unsigned char*)malloc(size + 9);
    if (!body) {
        printf("[H264] ERROR: Failed to allocate memory for packet body\n");
        return -1;
    }

    int i = 0;

    if (bIsKeyFrame) {
        body[i++] = 0x17;
    } else {
        body[i++] = 0x27;
    }

    body[i++] = 0x01;
    body[i++] = 0x00;
    body[i++] = 0x00;
    body[i++] = 0x00;

    body[i++] = size >> 24;
    body[i++] = size >> 16;
    body[i++] = size >> 8;
    body[i++] = size & 0xff;

    memcpy(&body[i], data, size);

    printf("[H264] Sending %s packet: size=%d, timestamp=%dms\n", 
           bIsKeyFrame ? "keyframe" : "iframe", size, nTimeStamp);
    int ret = RTMPStream_SendPacket(stream, RTMP_PACKET_TYPE_VIDEO, body, i + size, nTimeStamp);
    free(body);
    return ret;
}

static int FindStartCode(const unsigned char* buf, int start_pos, int buf_size, int* code_len) {
    int i;
    for (i = start_pos; i < buf_size - 3; i++) {
        if (buf[i] == 0x00 && buf[i+1] == 0x00) {
            if (buf[i+2] == 0x00 && buf[i+3] == 0x01) {
                *code_len = 4;
                return i;
            } else if (buf[i+2] == 0x01) {
                *code_len = 3;
                return i;
            }
        }
    }
    *code_len = 0;
    return -1;
}

static int u(int BitCount, unsigned char *buf, int *StartBit) {
    int ret = 0;
    int i;
    for (i = 0; i < BitCount; i++) {
        ret <<= 1;
        if (buf[*StartBit / 8] & (0x80 >> (*StartBit % 8))) {
            ret += 1;
        }
        *StartBit += 1;
    }
    return ret;
}

static unsigned int Ue(unsigned char *pBuff, int nLen, int *nStartBit) {
    int nZeroNum = 0;
    while (*nStartBit < nLen * 8) {
        if (pBuff[*nStartBit / 8] & (0x80 >> (*nStartBit % 8))) {
            break;
        }
        nZeroNum++;
        *nStartBit += 1;
    }
    *nStartBit += 1;
    return ((1 << nZeroNum) - 1 + u(nZeroNum, pBuff, nStartBit));
}

static int Se(unsigned char *pBuff, int nLen, int *nStartBit) {
    int UeVal = Ue(pBuff, nLen, nStartBit);
    int nValue = (UeVal + 1) / 2;
    if (UeVal % 2 == 0) {
        nValue = -nValue;
    }
    return nValue;
}

static int h264_decode_sps(unsigned char *buf, unsigned int nLen, int *width, int *height) {
    int StartBit = 0;
    int forbidden_zero_bit = u(1, buf, &StartBit);
    int nal_ref_idc = u(2, buf, &StartBit);
    int nal_unit_type = u(5, buf, &StartBit);
    
    if (nal_unit_type != 7) {
        return 0;
    }
    
    int profile_idc = u(8, buf, &StartBit);
    int constraint_set0_flag = u(1, buf, &StartBit);
    int constraint_set1_flag = u(1, buf, &StartBit);
    int constraint_set2_flag = u(1, buf, &StartBit);
    int constraint_set3_flag = u(1, buf, &StartBit);
    int reserved_zero_4bits = u(4, buf, &StartBit);
    int level_idc = u(8, buf, &StartBit);
    int seq_parameter_set_id = Ue(buf, nLen, &StartBit);
    
    if (profile_idc == 100 || profile_idc == 110 || profile_idc == 122 || profile_idc == 244 || 
        profile_idc == 44 || profile_idc == 83 || profile_idc == 86 || profile_idc == 118 || 
        profile_idc == 128 || profile_idc == 138 || profile_idc == 139 || profile_idc == 134) {
        
        int chroma_format_idc = Ue(buf, nLen, &StartBit);
        
        if (chroma_format_idc == 3) {
            int residual_colour_transform_flag = u(1, buf, &StartBit);
        }
        
        int bit_depth_luma_minus8 = Ue(buf, nLen, &StartBit);
        int bit_depth_chroma_minus8 = Ue(buf, nLen, &StartBit);
        int qpprime_y_zero_transform_bypass_flag = u(1, buf, &StartBit);
        int seq_scaling_matrix_present_flag = u(1, buf, &StartBit);
        
        if (seq_scaling_matrix_present_flag) {
            int i;
            for (i = 0; i < ((chroma_format_idc != 3) ? 8 : 12); i++) {
                int seq_scaling_list_present_flag = u(1, buf, &StartBit);
                if (seq_scaling_list_present_flag) {
                    int j;
                    int sizeOfScalingList = (i < 6) ? 16 : 64;
                    int lastScale = 8;
                    int nextScale = 8;
                    for (j = 0; j < sizeOfScalingList; j++) {
                        if (nextScale != 0) {
                            int delta_scale = Se(buf, nLen, &StartBit);
                            nextScale = (lastScale + delta_scale + 256) % 256;
                        }
                        lastScale = (nextScale == 0) ? lastScale : nextScale;
                    }
                }
            }
        }
    }
    
    int log2_max_frame_num_minus4 = Ue(buf, nLen, &StartBit);
    int pic_order_cnt_type = Ue(buf, nLen, &StartBit);
    
    if (pic_order_cnt_type == 0) {
        int log2_max_pic_order_cnt_lsb_minus4 = Ue(buf, nLen, &StartBit);
    } else if (pic_order_cnt_type == 1) {
        int delta_pic_order_always_zero_flag = u(1, buf, &StartBit);
        int offset_for_non_ref_pic = Se(buf, nLen, &StartBit);
        int offset_for_top_to_bottom_field = Se(buf, nLen, &StartBit);
        int num_ref_frames_in_pic_order_cnt_cycle = Ue(buf, nLen, &StartBit);
        
        int i;
        for (i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++) {
            Se(buf, nLen, &StartBit);
        }
    }
    
    int num_ref_frames = Ue(buf, nLen, &StartBit);
    int gaps_in_frame_num_value_allowed_flag = u(1, buf, &StartBit);
    int pic_width_in_mbs_minus1 = Ue(buf, nLen, &StartBit);
    int pic_height_in_map_units_minus1 = Ue(buf, nLen, &StartBit);
    
    *width = (pic_width_in_mbs_minus1 + 1) * 16;
    *height = (pic_height_in_map_units_minus1 + 1) * 16;
    
    return 1;
}

int RTMPStream_SendH264File(RTMPStream* stream, const char* pFileName) {
    if (!stream || !pFileName) {
        printf("[H264] ERROR: Invalid stream or file name\n");
        return -1;
    }

    printf("[H264] Opening file: %s\n", pFileName);
    FILE* fp = fopen(pFileName, "rb");
    if (!fp) {
        printf("[H264] ERROR: Failed to open file\n");
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    printf("[H264] File size: %ld bytes\n", file_size);

    unsigned char* pFileBuf = (unsigned char*)malloc(file_size);
    if (!pFileBuf) {
        printf("[H264] ERROR: Failed to allocate buffer for file\n");
        fclose(fp);
        return -1;
    }

    long bytes_read = fread(pFileBuf, sizeof(unsigned char), file_size, fp);
    fclose(fp);
    printf("[H264] Read %ld bytes into buffer\n", bytes_read);

    RTMPMetadata metaData;
    memset(&metaData, 0, sizeof(RTMPMetadata));

    NaluUnit naluUnit;
    int cur_pos = 0;

    printf("[H264] Reading SPS NALU...\n");
    int start_code_len;
    int start_pos = FindStartCode(pFileBuf, cur_pos, bytes_read, &start_code_len);
    if (start_pos == -1) {
        printf("[H264] ERROR: Failed to find SPS NALU\n");
        free(pFileBuf);
        return -1;
    }
    int nalu_start = start_pos + start_code_len;
    
    int next_start_code_len;
    int next_start_pos = FindStartCode(pFileBuf, nalu_start, bytes_read, &next_start_code_len);
    
    int nalu_size;
    if (next_start_pos == -1) {
        nalu_size = bytes_read - nalu_start;
    } else {
        nalu_size = next_start_pos - nalu_start;
    }
    
    naluUnit.type = pFileBuf[nalu_start] & 0x1f;
    naluUnit.size = nalu_size;
    naluUnit.data = &pFileBuf[nalu_start];
    
    metaData.nSpsLen = naluUnit.size;
    memcpy(metaData.Sps, naluUnit.data, naluUnit.size);
    printf("[H264] SPS NALU found: type=%d, size=%d\n", naluUnit.type, naluUnit.size);
    cur_pos = next_start_pos == -1 ? bytes_read : next_start_pos;

    printf("[H264] Reading PPS NALU...\n");
    start_pos = FindStartCode(pFileBuf, cur_pos, bytes_read, &start_code_len);
    if (start_pos == -1) {
        printf("[H264] ERROR: Failed to find PPS NALU\n");
        free(pFileBuf);
        return -1;
    }
    nalu_start = start_pos + start_code_len;
    
    next_start_pos = FindStartCode(pFileBuf, nalu_start, bytes_read, &next_start_code_len);
    
    if (next_start_pos == -1) {
        nalu_size = bytes_read - nalu_start;
    } else {
        nalu_size = next_start_pos - nalu_start;
    }
    
    naluUnit.type = pFileBuf[nalu_start] & 0x1f;
    naluUnit.size = nalu_size;
    naluUnit.data = &pFileBuf[nalu_start];
    
    metaData.nPpsLen = naluUnit.size;
    memcpy(metaData.Pps, naluUnit.data, naluUnit.size);
    printf("[H264] PPS NALU found: type=%d, size=%d\n", naluUnit.type, naluUnit.size);
    cur_pos = next_start_pos == -1 ? bytes_read : next_start_pos;

    int width = 0, height = 0;
    if (h264_decode_sps(metaData.Sps, metaData.nSpsLen, &width, &height)) {
        printf("[H264] SPS decoded: width=%d, height=%d\n", width, height);
    } else {
        printf("[H264] WARNING: Failed to decode SPS, using default 1920x1080\n");
        width = 1920;
        height = 1080;
    }
    metaData.nWidth = width;
    metaData.nHeight = height;
    metaData.nFrameRate = 25;

    printf("[H264] Sending metadata and sequence headers...\n");
    RTMPStream_SendMetadata(stream, &metaData);

    unsigned int tick = 0;
    unsigned int nalu_count = 0;
    unsigned int keyframe_count = 0;
    unsigned int dropped_count = 0;
    printf("[H264] Starting to push H264 frames...\n");
    
    while (cur_pos < bytes_read) {
        start_pos = FindStartCode(pFileBuf, cur_pos, bytes_read, &start_code_len);
        if (start_pos == -1) {
            break;
        }
        nalu_start = start_pos + start_code_len;
        
        next_start_pos = FindStartCode(pFileBuf, nalu_start, bytes_read, &next_start_code_len);
        
        if (next_start_pos == -1) {
            nalu_size = bytes_read - nalu_start;
            cur_pos = bytes_read;
        } else {
            nalu_size = next_start_pos - nalu_start;
            cur_pos = next_start_pos;
        }
        
        if (nalu_size < 1) {
            continue;
        }
        
        nalu_count++;
        
        int nalu_type = pFileBuf[nalu_start] & 0x1f;
        if (nalu_type >= 1 && nalu_type <= 5) {
            int bKeyframe = (nalu_type == 0x05) ? 1 : 0;
            if (bKeyframe) keyframe_count++;
            
            printf("[H264] NALU #%d: type=%d (%s), size=%d, timestamp=%dms\n", 
                   nalu_count, nalu_type, bKeyframe ? "keyframe" : "iframe", nalu_size, tick);
            
            int ret = RTMPStream_SendH264Packet(stream, &pFileBuf[nalu_start], nalu_size, bKeyframe, tick);
            if (ret != 0) {
                printf("[H264] ERROR: SendH264Packet failed, stopping\n");
                free(pFileBuf);
                return -1;
            }
            
#ifdef WIN32
            Sleep(40);
#else
            usleep(40000);
#endif
            tick += 40;
        } else {
            dropped_count++;
            printf("[H264] NALU #%d: type=%d (skipped), size=%d\n", nalu_count, nalu_type, nalu_size);
        }
    }

    printf("[H264] Push completed. Total NALUs: %d (keyframes: %d, skipped: %d)\n", 
           nalu_count, keyframe_count, dropped_count);
    printf("[H264] Total duration: %dms\n", tick);

    free(pFileBuf);
    return 0;
}

int RTMPStream_SendFLVFile(RTMPStream* stream, const char* pFileName) {
    if (!stream || !pFileName) {
        printf("[FLV] ERROR: Invalid stream or file name\n");
        return -1;
    }

    printf("[FLV] Opening file: %s\n", pFileName);
    FILE* fp = fopen(pFileName, "rb");
    if (!fp) {
        printf("[FLV] ERROR: Failed to open file\n");
        return -1;
    }

    unsigned char header[9];
    if (fread(header, 1, 9, fp) != 9) {
        printf("[FLV] ERROR: Failed to read FLV header\n");
        fclose(fp);
        return -1;
    }

    printf("[FLV] Header: %.3s v%d, video:%d, audio:%d\n", 
           header, header[3], (header[4] & 0x01), ((header[4] >> 2) & 0x01));

    unsigned int prev_tag_size;
    fread(&prev_tag_size, 4, 1, fp);
    printf("[FLV] prev_tag_size0: %d\n", prev_tag_size);

    unsigned char tag_header[11];
    unsigned int last_timestamp = 0;
    unsigned int total_tags = 0;
    unsigned int video_tags = 0;
    unsigned int audio_tags = 0;
    unsigned int script_tags = 0;
    int send_result = 0;

    printf("[FLV] Starting to push FLV file...\n");

    while (fread(tag_header, 1, 11, fp) == 11) {
        unsigned char tag_type = tag_header[0];
        unsigned int data_size = (tag_header[1] << 16) | (tag_header[2] << 8) | tag_header[3];
        unsigned int timestamp = (tag_header[4] << 16) | (tag_header[5] << 8) | tag_header[6];
        unsigned int timestamp_extended = tag_header[7];
        timestamp |= (timestamp_extended << 24);

        total_tags++;

        const char* type_name = "unknown";
        if (tag_type == 0x08) { type_name = "audio"; audio_tags++; }
        else if (tag_type == 0x09) { type_name = "video"; video_tags++; }
        else if (tag_type == 0x12) { type_name = "script"; script_tags++; }

        unsigned char* data = (unsigned char*)malloc(data_size);
        if (!data) {
            printf("[FLV] ERROR: Failed to allocate memory for tag data\n");
            break;
        }

        if (fread(data, 1, data_size, fp) != data_size) {
            printf("[FLV] ERROR: Failed to read tag data\n");
            free(data);
            break;
        }

        fread(&prev_tag_size, 4, 1, fp);

        if (timestamp < last_timestamp) {
            printf("[FLV] WARNING: Timestamp went backwards (%d -> %d), using last_timestamp\n", 
                   timestamp, last_timestamp);
            timestamp = last_timestamp;
        }

        if (timestamp > last_timestamp) {
            unsigned int delay = timestamp - last_timestamp;
            printf("[FLV] Sleeping %dms before sending next tag\n", delay);
#ifdef WIN32
            Sleep(delay);
#else
            usleep(delay * 1000);
#endif
        }

        if (data_size >= 2) {
            printf("[FLV] Tag #%d: type=%s(0x%02X), size=%d, timestamp=%dms, data[0]=0x%02X, data[1]=0x%02X\n", 
                   total_tags, type_name, tag_type, data_size, timestamp, data[0], data[1]);
        } else {
            printf("[FLV] Tag #%d: type=%s(0x%02X), size=%d, timestamp=%dms\n", 
                   total_tags, type_name, tag_type, data_size, timestamp);
        }

        if (tag_type == 0x08) {
            send_result = RTMPStream_SendPacket(stream, RTMP_PACKET_TYPE_AUDIO, data, data_size, timestamp);
        } else if (tag_type == 0x09) {
            send_result = RTMPStream_SendPacket(stream, RTMP_PACKET_TYPE_VIDEO, data, data_size, timestamp);
        } else if (tag_type == 0x12) {
            send_result = RTMPStream_SendPacket(stream, RTMP_PACKET_TYPE_INFO, data, data_size, timestamp);
        }

        if (send_result != 0) {
            printf("[FLV] ERROR: SendPacket failed, stopping push\n");
            free(data);
            break;
        }

        last_timestamp = timestamp;
        free(data);
    }

    printf("[FLV] Push completed. Total tags: %d (video: %d, audio: %d, script: %d)\n", 
           total_tags, video_tags, audio_tags, script_tags);

    fclose(fp);
    return send_result;
}

static int strcasecmp(const char* s1, const char* s2) {
    while (*s1 && *s2) {
        int c1 = tolower((unsigned char)*s1);
        int c2 = tolower((unsigned char)*s2);
        if (c1 != c2) return c1 - c2;
        s1++;
        s2++;
    }
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

static void print_usage(const char* prog) {
    printf("Usage: %s -i <input_file> -r <rtmp_url>\n", prog);
    printf("  -i, --input     Input file (FLV or H264 file)\n");
    printf("  -r, --rtmp      RTMP server address\n");
    printf("  -h, --help      Show this help message\n");
}

int main(int argc, char* argv[]) {
    const char* input_file = NULL;
    const char* rtmp_url = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "i:r:h")) != -1) {
        switch (opt) {
        case 'i':
            input_file = optarg;
            break;
        case 'r':
            rtmp_url = optarg;
            break;
        case 'h':
            print_usage(argv[0]);
            return 0;
        default:
            print_usage(argv[0]);
            return 1;
        }
    }

    if (!input_file || !rtmp_url) {
        printf("ERROR: Missing required arguments\n");
        print_usage(argv[0]);
        return 1;
    }

    setvbuf(stdout, NULL, _IONBF, 0);
    printf("[MAIN] Input file: %s\n", input_file);
    printf("[MAIN] RTMP URL: %s\n", rtmp_url);

    RTMPStream* rtmp_stream = RTMPStream_Create();
    if (!rtmp_stream) {
        printf("ERROR: Failed to create RTMPStream\n");
        return 1;
    }

    printf("[MAIN] Connecting to RTMP server...\n");
    if (RTMPStream_Connect(rtmp_stream, rtmp_url) < 0) {
        printf("ERROR: Failed to connect to RTMP server\n");
        RTMPStream_Destroy(rtmp_stream);
        return 1;
    }
    printf("[MAIN] Connected to RTMP server successfully\n");

    const char* ext = strrchr(input_file, '.');
    if (ext != NULL) {
        if (strcasecmp(ext, ".flv") == 0) {
            printf("[MAIN] Sending FLV file...\n");
            RTMPStream_SendFLVFile(rtmp_stream, input_file);
        } else if (strcasecmp(ext, ".h264") == 0 || strcasecmp(ext, ".264") == 0) {
            printf("[MAIN] Sending H264 file...\n");
            RTMPStream_SendH264File(rtmp_stream, input_file);
        } else {
            printf("[MAIN] Unknown file format, trying FLV...\n");
            RTMPStream_SendFLVFile(rtmp_stream, input_file);
        }
    } else {
        printf("[MAIN] No file extension, trying FLV...\n");
        RTMPStream_SendFLVFile(rtmp_stream, input_file);
    }

    printf("[MAIN] Push completed\n");
    RTMPStream_Destroy(rtmp_stream);
    printf("[MAIN] Cleanup done\n");

    return 0;
}