#include <pch.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>
#pragma comment(lib, "winmm.lib") 


#define FRAME_SIZE(SAMPLEPSEC, BITS_PER_SAMPLE, CHANNELS, n)		(10 * n * (SAMPLEPSEC * CHANNELS * (BITS_PER_SAMPLE/8) /1000))//10ms

#define BUF_CNT				10

DWORD samplepsec = 16000;
DWORD bits_per_sample = 16;
DWORD channels = 1;

DWORD buf_size = 0;

HWAVEIN			hWaveIn;
HWAVEOUT		hWaveOut;
LPWAVEHDR		WaveInHead[BUF_CNT] = { 0 };


FILE* pfWaveIn;//音频文件的句柄
int SaveWaveIn = 0;

DWORD_PTR CALLBACK WaveInCallBack(HWAVEIN hWave, UINT uMsg, DWORD_PTR  dwInstance, DWORD_PTR  dwParam1, DWORD_PTR  dwParam2)
{
	MMRESULT mmResult = 0;

	//PWAVEHDR whd = (PWAVEHDR)dwParam1;
	LPWAVEHDR lpHdr = (LPWAVEHDR)dwParam1;



	switch (uMsg)
	{
	case WIM_OPEN:
		break;
	case WIM_DATA:

		if (lpHdr->dwBytesRecorded == 0 || lpHdr == NULL)
			//return ERROR_SUCCESS;
			goto error;


		waveInUnprepareHeader(hWave, lpHdr, sizeof(WAVEHDR));

		if (pfWaveIn)
		{
			fwrite(lpHdr->lpData, 1, lpHdr->dwBytesRecorded, pfWaveIn);
		}

		//将要输出的数据写入buffer
		mmResult = ::waveOutPrepareHeader(hWaveOut, lpHdr, sizeof(WAVEHDR));
		if (mmResult)
		{
			printf("waveOutPrepareHeader error\n");
			//return ERROR_SUCCESS;
			goto error;
		}

		//将输出数据发送给输出设备
		mmResult = ::waveOutWrite(hWaveOut, lpHdr, sizeof(WAVEHDR));
		if (mmResult)
		{
			printf("waveOutPrepareHeader error\n");
			//return ERROR_SUCCESS;
			goto error;
		}

		//waveInPrepareHeader(hWave, lpHdr, sizeof(WAVEHDR));
		//waveInAddBuffer(hWave, lpHdr, sizeof(WAVEHDR));

		break;
	case WIM_CLOSE:
		//waveInStop(hWaveIn);
		//waveInReset(hWaveIn);
		//waveInClose(hWaveIn);
		break;
	default:
		break;
	}



	return 0;

error:


	return ERROR_SUCCESS;

}


DWORD_PTR CALLBACK WaveOutCallBack(HWAVEOUT hWave, UINT uMsg, DWORD_PTR  dwInstance, DWORD_PTR  dwParam1, DWORD_PTR  dwParam2)
{
	MMRESULT mmResult = 0;

	LPWAVEHDR lpHdr = (LPWAVEHDR)dwParam1;

	switch (uMsg)
	{
	case WIM_OPEN:
		break;
	case WIM_DATA:
		break;
	case WOM_DONE:
		if (lpHdr)
		{
			waveOutUnprepareHeader(hWave, lpHdr, sizeof(WAVEHDR));
			waveInPrepareHeader(hWaveIn, lpHdr, sizeof(WAVEHDR));
			waveInAddBuffer(hWaveIn, lpHdr, sizeof(WAVEHDR));
		}
		break;
	case WIM_CLOSE:
		break;

	default:
		break;
	}
	return 0;
}

LPWAVEHDR CreateWaveHeader(void)
{
	LPWAVEHDR lpHdr = new WAVEHDR;

	if (lpHdr == NULL)
	{
		printf("Unable to allocate the memory\n");
		return NULL;
	}

	ZeroMemory(lpHdr, sizeof(WAVEHDR));
	char* lpByte = new char[buf_size];

	if (lpByte == NULL)
	{
		printf("Unable to allocate the memory\n");
		return NULL;
	}
	lpHdr->lpData = lpByte;
	lpHdr->dwBufferLength = buf_size;
	return lpHdr;
}

void ReleaseWaveHeader(LPWAVEHDR lpHdr)
{
	if (lpHdr == NULL)
	{
		return;
	}

	if (lpHdr->lpData)
	{
		delete lpHdr->lpData;
	}

	free(lpHdr);
}

int main(int argc, char* argv[])
{
	MMRESULT		mmReturn;
	int	i;
	WAVEFORMATEX	WaveInFormatEx;
	WAVEFORMATEX	WaveOutFormatEx;


	if (argc > 3)
	{
		samplepsec = atoi(argv[1]);
		bits_per_sample = atoi(argv[2]);
		channels = atoi(argv[3]);
	}

	if (argc > 4)
	{
		SaveWaveIn = atoi(argv[4]);
	}



	printf("SamplesPerSec:%d, BitsPerSample:%d, Channels:%d, SaveWaveIn:%d\n", samplepsec, bits_per_sample, channels, SaveWaveIn);


	//for (i = 0; i < BUF_CNT; i++)
	//	memset(WaveInHead[i], 0, sizeof(WaveInHead[i]));


	//初始化音频格式结构体
	memset(&WaveInFormatEx, 0, sizeof(WaveInFormatEx));
	memset(&WaveOutFormatEx, 0, sizeof(WaveOutFormatEx));

	WaveInFormatEx.wFormatTag = WAVE_FORMAT_PCM;//声音格式为PCM  
	WaveInFormatEx.nSamplesPerSec = samplepsec;//采样率，16000次/秒  
	WaveInFormatEx.wBitsPerSample = bits_per_sample;//采样比特，16bits/次  
	WaveInFormatEx.nChannels = channels;//采样声道数，2声道  
	WaveInFormatEx.cbSize = 0;//一般为0  
	WaveInFormatEx.nBlockAlign = (WaveInFormatEx.wBitsPerSample * WaveInFormatEx.nChannels) >> 3;//一个块的大小，采样bit的字节数乘以声道数  
	WaveInFormatEx.nAvgBytesPerSec = WaveInFormatEx.nBlockAlign * WaveInFormatEx.nSamplesPerSec;//每秒的数据率，就是每秒能采集多少字节的数据  

	WaveOutFormatEx.wFormatTag = WAVE_FORMAT_PCM;//声音格式为PCM  
	WaveOutFormatEx.nSamplesPerSec = samplepsec;//采样率，16000次/秒  
	WaveOutFormatEx.wBitsPerSample = bits_per_sample;//采样比特，16bits/次  
	WaveOutFormatEx.nChannels = channels;//采样声道数，2声道  
	WaveOutFormatEx.cbSize = 0;//一般为0 
	WaveOutFormatEx.nBlockAlign = (WaveOutFormatEx.wBitsPerSample * WaveOutFormatEx.nChannels) >> 3;//一个块的大小，采样bit的字节数乘以声道数
	WaveOutFormatEx.nAvgBytesPerSec = WaveOutFormatEx.nBlockAlign * WaveOutFormatEx.nSamplesPerSec;//每秒的数据率，就是每秒能采集多少字节的数据  


	if(SaveWaveIn)
		fopen_s(&pfWaveIn, "WaveIn.pcm", "wb");

	buf_size = FRAME_SIZE(samplepsec, bits_per_sample, channels, 1);

	//开启音频采集
	mmReturn = waveInOpen(&hWaveIn, WAVE_MAPPER, &WaveInFormatEx, (DWORD_PTR)WaveInCallBack, 0, CALLBACK_FUNCTION);
	//mmReturn = ::waveInOpen(&hWaveIn, WAVE_MAPPER, &WaveInFormatEx, ::GetCurrentThreadId(), 0, CALLBACK_THREAD);
	if (mmReturn != MMSYSERR_NOERROR) //打开采集失败
	{
		printf("waveInOpen fail\n");
		return -1;
	}

	//打开音频输出设备
	//mmReturn = waveOutOpen(&hWaveOut, WAVE_MAPPER, &WaveOutFormatEx, ::GetCurrentThreadId(), 0, CALLBACK_THREAD);
	mmReturn = waveOutOpen(&hWaveOut, WAVE_MAPPER, &WaveOutFormatEx, (DWORD_PTR)WaveOutCallBack, 0, CALLBACK_FUNCTION);
	if (mmReturn != MMSYSERR_NOERROR) //打开采集失败
	{
		printf("waveOutOpen fail\n");
		goto waveClose;
	}
	else
	{
		//DWORD volume = 0xffffffff;
		//waveOutSetVolume(hAudioPlay, volume);//设置输出设备的输出量
	}

	for (i = 0; i < BUF_CNT; i++)
	{
		WaveInHead[i] = CreateWaveHeader();
		if (WaveInHead[i])
		{
			mmReturn = waveInPrepareHeader(hWaveIn, WaveInHead[i], sizeof(WAVEHDR));
			if (mmReturn != MMSYSERR_NOERROR)
			{
				printf("waveInPrepareHeader fail\n");
				goto waveClose;
			}

			mmReturn = ::waveInAddBuffer(hWaveIn, WaveInHead[i], sizeof(WAVEHDR));
			if (mmReturn != MMSYSERR_NOERROR)
			{
				printf("waveInAddBuffer fail\n");
				goto waveClose;
			}
		}
	}


	//开启指定的输入采集设备
	mmReturn = waveInStart(hWaveIn);
	if (mmReturn != MMSYSERR_NOERROR)
	{
		printf("waveInStart fail\n");
		goto waveClose;
	}
	while (1)
	{
		Sleep(1000);
	}


waveClose:

	if (hWaveIn)
	{
		waveInStop(hWaveIn);
		waveInClose(hWaveIn);
	}

	if (hWaveOut)
	{
		waveOutClose(hWaveOut);
	}

	for (i = 0; i < BUF_CNT; i++)
	{
		ReleaseWaveHeader(WaveInHead[i]);
	}

	if (pfWaveIn)
		fclose(pfWaveIn);


	return 0;
}



