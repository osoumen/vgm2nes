//
//  main.cpp
//  vgm2nes
//
//  Created by osoumen on 2014/11/09.
//  Copyright (c) 2014年 osoumen. All rights reserved.
//

#include <iostream>

static const unsigned char nesheader[] = {
    0x4e, 0x45, 0x53, 0x1a, 0x20, 0x00, 0x40, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const unsigned char nesvector[] = {
    0x35, 0xE0, 0x00, 0xE0, 0x00, 0x00
};

static const unsigned char nespg[] = {
    0xa9, 0x00, 0x8d, 0x00, 0x20, 0x8d, 0x01, 0x20, 0xa9, 0x88, 0x8d, 0x00, 0x20, 0xa9, 0x00, 0x85,
    0x10, 0xa9, 0x00, 0x85, 0x00, 0xa9, 0x80, 0x85, 0x01, 0xa9, 0x3c, 0x85, 0x03, 0xa9, 0x00, 0x85,
    0x02, 0xa9, 0x40, 0x85, 0x21, 0xa9, 0x06, 0x8d, 0x00, 0x80, 0xa5, 0x02, 0x8d, 0x01, 0x80, 0x4c,
    0x32, 0xe0, 0x4c, 0x32, 0xe0, 0xc6, 0x03, 0xa6, 0x03, 0xf0, 0x01, 0x40, 0xa0, 0x00, 0xb1, 0x00,
    0xc9, 0xb4, 0xd0, 0x14, 0x20, 0x73, 0xe0, 0xb1, 0x00, 0x85, 0x20, 0x20, 0x73, 0xe0, 0xb1, 0x00,
    0x91, 0x20, 0x20, 0x73, 0xe0, 0x4c, 0x3c, 0xe0, 0xc9, 0x62, 0xd0, 0x08, 0x20, 0x73, 0xe0, 0xa9,
    0x01, 0x85, 0x03, 0x40, 0xc9, 0x61, 0xd0, 0x0a, 0x20, 0x73, 0xe0, 0xb1, 0x00, 0x85, 0x03, 0x20,
    0x73, 0xe0, 0x40, 0x48, 0xe6, 0x00, 0xd0, 0x1c, 0xe6, 0x01, 0xa5, 0x01, 0x29, 0x20, 0xf0, 0x14,
    0xa9, 0x00, 0x85, 0x01, 0xe6, 0x02, 0xa9, 0x06, 0x8d, 0x00, 0x80, 0xa5, 0x02, 0x8d, 0x01, 0x80,
    0xa9, 0x80, 0x85, 0x01, 0x68, 0x60
};

long vgmSize = 0;
unsigned char *vgmData = NULL;
unsigned int    nesClock = 0;
int    waitSamples = 0;
unsigned int    vgmPtr = 0;
unsigned char *outData = NULL;
unsigned int    outPtr = sizeof(nesheader);

static const int FRAME_SAMPLES_NTSC = 735;
//static const int FRAME_SAMPLES_PAL = 882;

static const int PGPOS = 0x7e000 + sizeof(nesheader);

static const int OUT_MAX_SIZE = 1024 * 512 + sizeof(nesheader);


long GetFileSize(FILE* fp)
{
	long fpos_save, size;
    
	/* 現在のファイルポジションを保存 */
	fpos_save = ftell( fp );
    
	/* ファイルの末尾まで移動して、その位置を調べる */
	fseek( fp, 0, SEEK_END );
	size = ftell(fp);
    
	/* ファイルポジションを元に戻す */
	fseek( fp, fpos_save, SEEK_SET );
    
	return size;
}

int getVgmCmdLen(int opCode)
{
    switch (opCode) {
        case 0x4f:
        case 0x50:
            return 2;
        case 0x51:
        case 0x52:
        case 0x53:
        case 0x61:
            return 3;
        case 0x62:
        case 0x63:
            return 1;
        case 0x64:
            return 4;
        case 0x66:
            return 1;
        case 0xb4:
            return 3;
        default:
            return 1;
    }
}

bool WaitSamples()
{
    if (waitSamples < 0) {
        return false;
    }
    int frames = (waitSamples + FRAME_SAMPLES_NTSC/2) / FRAME_SAMPLES_NTSC;
    int div = frames / 256;
    int mod = frames % 256;
    
    if (div == 0 && mod == 1) {
        if (outPtr >= OUT_MAX_SIZE) {
            return true;
        }
        outData[outPtr] = 0x62;
        outPtr++;
    }
    else {
        for (int i=0; i<div; i++) {
            if (outPtr >= OUT_MAX_SIZE) {
                return true;
            }
            outData[outPtr] = 0x61;
            outPtr++;
            
            if (outPtr >= OUT_MAX_SIZE) {
                return true;
            }
            outData[outPtr] = 0xff;
            outPtr++;
        }
        if (mod > 0) {
            if (outPtr >= OUT_MAX_SIZE) {
                return true;
            }
            outData[outPtr] = 0x61;
            outPtr++;
            
            outData[outPtr] = mod;
            outPtr++;
            if (outPtr >= OUT_MAX_SIZE) {
                return true;
            }
        }
    }
    
    waitSamples -= div * (FRAME_SAMPLES_NTSC * 256) + (mod * FRAME_SAMPLES_NTSC);
    
    return false;
}

bool WriteAPUReg(int addr, int data)
{
    if (outPtr >= OUT_MAX_SIZE) {
        return true;
    }
    outData[outPtr] = 0xb4;
    outPtr++;
    
    if (outPtr >= OUT_MAX_SIZE) {
        return true;
    }
    outData[outPtr] = addr;
    outPtr++;
    
    if (outPtr >= OUT_MAX_SIZE) {
        return true;
    }
    outData[outPtr] = data;
    outPtr++;
    
    return false;
}

int main(int argc, const char * argv[])
{

    // 出力ファイル名を決める
    char        dstFileName[512];
    if (argc < 3) {
        strncpy(dstFileName, argv[1], 512);
        int extPos = 0;
        for (int i=0; dstFileName[i] != 0; i++) {
            if (dstFileName[i] == '.') {
                extPos = i+1;
            }
        }
        strcpy(&dstFileName[extPos],"nes");
    }
    else {
        strncpy(dstFileName, argv[2], 512);
    }
    
    // vgmファイルの読み込み
    FILE    *fp;
    if ((fp = fopen(argv[1], "rb")) == NULL) {
        fprintf(stderr, "%sのオープンに失敗しました.\n", argv[1]);
        exit(EXIT_FAILURE);
    }
    vgmSize = GetFileSize(fp);
    vgmData = new unsigned char[vgmSize];
    fread(vgmData, 1, vgmSize, fp);
    fclose(fp);
    
    // nes用のデータがあるか
    nesClock = *(unsigned int*)(&vgmData[0x84]);
    
    if (nesClock == 0) {
        return 0;
    }
    
    // データの開始位置を得る
    vgmPtr = *(unsigned int*)(&vgmData[0x34]) + 0x34;
    
    // 出力メモリの確保
    outData = new unsigned char[OUT_MAX_SIZE];
    memset(outData, 0, OUT_MAX_SIZE);

    // ヘッダプログラム部の書き込み
    memcpy(outData, nesheader, sizeof(nesheader));
    memcpy(&outData[PGPOS], nespg, sizeof(nespg));
    memcpy(&outData[OUT_MAX_SIZE-sizeof(nesvector)], nesvector, sizeof(nesvector));
    
    int opCode = 0;
    while (opCode != 0x66 && vgmPtr < vgmSize) {
        int samplesToWait[2] = {FRAME_SAMPLES_NTSC, 882};
        opCode = vgmData[vgmPtr];
        int opData[4];
        int cmdLen = getVgmCmdLen(opCode);
        for (int i=1; i<cmdLen; i++) {
            opData[i-1] = vgmData[vgmPtr+i];
        }
        switch (opCode) {
            case 0xb4:
                if (WaitSamples()) {
                    goto endVgm;
                }
                if (WriteAPUReg(opData[0], opData[1])) {
                    goto endVgm;
                }
                break;
            case 0x61:
                waitSamples += opData[0] + (opData[1] * 256);
                break;
            case 0x62:
                waitSamples += samplesToWait[0];
                break;
            case 0x63:
                waitSamples += samplesToWait[1];
                break;
            case 0x64:
                samplesToWait[opData[0]-0x62] = opData[1] + (opData[2] << 8);
                break;
            case 0x66:
                // data End
                WaitSamples();
                break;
            default:
                if ((opCode & 0xf0) == 0x70) {
                    waitSamples += (opCode & 0x0f) + 1;
                }
                else {
                    printf("unknown command(%02x):%02x\n", vgmPtr, opCode);
                }
                break;
        }
        
        vgmPtr += cmdLen;
    }
endVgm:
    
    // 出力ファイルの書き込み
    if ((fp = fopen(dstFileName, "wb")) == NULL) {
        fprintf(stderr, "%sのオープンに失敗しました.\n", argv[1]);
        exit(EXIT_FAILURE);
    }
    fwrite(outData, 1, OUT_MAX_SIZE, fp);
    fclose(fp);
    
    return 0;
}

