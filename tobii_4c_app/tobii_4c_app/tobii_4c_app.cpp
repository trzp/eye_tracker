#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4996)

#include <tobii/tobii.h>
#include <tobii/tobii_streams.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "stdafx.h"  
#include <WinSock2.h>
#include <windows.h>  

#pragma comment(lib, "ws2_32.lib")
#pragma comment( linker, "/subsystem:windows /entry:mainCRTStartup" )  //不显示控制台


//全局，用于在主函数和回调函数中共享数据
float gaze_x = 0;
float gaze_y = 0;

//标记是否有新数据到达
int flg1 = 0;
int flg2 = 0;

bool debug = true;


//眼动仪
void gaze_point_callback(tobii_gaze_point_t const* gaze_point, void* user_data)
{
	if (gaze_point->validity == TOBII_VALIDITY_VALID)
	{
		gaze_x = gaze_point->position_xy[0];
		gaze_y = gaze_point->position_xy[1];
		flg1++;
	}
}

//眼动仪
static void url_receiver(char const* url, void* user_data)
{
	char* buffer = (char*)user_data;
	if (*buffer != '\0') return; // only keep first value

	if (strlen(url) < 256)
		strcpy(buffer, url);
}


int main(int argc, char*  argv[])
{
	//命令行参数包含：
	/*
	target_port: 目标端口，默认5150
	target_ip: 目标ip,默认127.0.0.1
	my_port: 自己绑定的端口，默认5151
	my_ip: 自己的ip,默认127.0.0.1
	*/


	//socket交互相关
	//程序启动后，通过udp socket向指定地址(target_port,target_ip)发送眼动数据，x,y float数据，8字节
	//上位机可在某一次接收数据后，向tobii_4c_app地址（my_port,my_ip）发送四字节数据：1,3,125,127，来关闭app
	
	SOCKADDR_IN target_siLocal, my_siLocal, remote_siLocal;
	int target_port = 5150;
	target_siLocal.sin_family = AF_INET;
	target_siLocal.sin_port = htons(target_port);
	target_siLocal.sin_addr.s_addr = inet_addr("127.0.0.1");

	int my_port = 5151;
	my_siLocal.sin_family = AF_INET;
	my_siLocal.sin_port = htons(my_port);
	my_siLocal.sin_addr.s_addr = inet_addr("127.0.0.1");

	
	if (argc >= 4)
	{
		if (strcpy(argv[0], "1"))
		{
			debug = true;
		}
		else
		{
			debug = false;
		}

		assert(sscanf(argv[1], "%d", &target_port));
		target_siLocal.sin_family = AF_INET;
		target_siLocal.sin_port = htons(target_port);
		target_siLocal.sin_addr.s_addr = inet_addr(argv[2]);
		
		assert(sscanf(argv[3], "%d", &my_port));
		my_siLocal.sin_family = AF_INET;
		my_siLocal.sin_port = htons(my_port);
		my_siLocal.sin_addr.s_addr = inet_addr(argv[4]);
	}

	

	bool running = true;
	//=======================================================================================================================
	//socket
	WSAData wsd;           //初始化信息
	SOCKET udp_sock;         //发送SOCKET
	int nRet = 0;

	//启动Winsock

	if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0)
	{
		if (debug)
		{
			printf("WSAStartup Error: %i\n", WSAGetLastError());
		}
		return 0;
	}

	//创建socket
	
	udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (udp_sock == SOCKET_ERROR) 
	{
		if (debug)
		{
			printf("socket Error: %i\n", WSAGetLastError());
		}
		return 0;
	}

	char recv_buf[4] = {0};
	char sendbuffer[8] = { 0 };
	int dwSendSize = sizeof(remote_siLocal);

	if (bind(udp_sock, (SOCKADDR*)&my_siLocal, sizeof(my_siLocal)) == SOCKET_ERROR)
	{
		printf("bind Error: %i",WSAGetLastError());
		return 1;
	}
	
	unsigned long ul = 1;
	int ret = ioctlsocket(udp_sock, FIONBIO, (unsigned long *)&ul);//设置成非阻塞模式。
	assert(ret != SOCKET_ERROR);

	//================================================================================================================================
	//眼动仪相关
	tobii_api_t* api;
	tobii_error_t error = tobii_api_create(&api, NULL, NULL);
	assert(error == TOBII_ERROR_NO_ERROR);

	char url[256] = { 0 };
	error = tobii_enumerate_local_device_urls(api, url_receiver, url);
	assert(error == TOBII_ERROR_NO_ERROR && *url != '\0');

	tobii_device_t* device;
	error = tobii_device_create(api, url, &device);
	assert(error == TOBII_ERROR_NO_ERROR);

	error = tobii_gaze_point_subscribe(device, gaze_point_callback, 0);
	assert(error == TOBII_ERROR_NO_ERROR);

	while (running)
	{
		flg2 = flg1;
		error = tobii_wait_for_callbacks(NULL, 1, &device);
		assert(error == TOBII_ERROR_NO_ERROR || error == TOBII_ERROR_TIMED_OUT);
		error = tobii_device_process_callbacks(device);
		assert(error == TOBII_ERROR_NO_ERROR);
		if (flg2 != flg1)	//接收到新数据
		{
			if (debug)
			{
				printf("Gaze point: %f, %f\n",gaze_x,gaze_y);
			}

			memcpy(sendbuffer,&gaze_x,4);
			memcpy(sendbuffer + 4, &gaze_y, 4);

			nRet = sendto(udp_sock, sendbuffer, 8, 0, (SOCKADDR*)&target_siLocal, sizeof(SOCKADDR));
			assert(nRet != SOCKET_ERROR);
			//每次发送数据后都查看接收，只接收上位机的关闭指令
			nRet = recvfrom(udp_sock, recv_buf, 4, 0, (SOCKADDR*)&remote_siLocal, &dwSendSize);
			if (nRet > 0)
			{
				if ((recv_buf[0] == 1) & (recv_buf[1] == 3) & (recv_buf[2] == 125) & (recv_buf[3] == 127))	//上位机命令关闭
				{
					running = false;
				}
			}
		}
	}

	error = tobii_gaze_point_unsubscribe(device);
	assert(error == TOBII_ERROR_NO_ERROR);

	error = tobii_device_destroy(device);
	assert(error == TOBII_ERROR_NO_ERROR);

	error = tobii_api_destroy(api);
	assert(error == TOBII_ERROR_NO_ERROR);

	return 0;
}





