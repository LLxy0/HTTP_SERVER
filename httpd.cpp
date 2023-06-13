#include<stdio.h>
#include <winsock2.h>
#include<string.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<ws2tcpip.h>
#pragma comment(lib, "WS2_32.lib")

#define PRINTF(str) printf("[%s - %d]"#str"=%s\n", __func__, __LINE__, str);

void error_die(const char* str)
{
	perror(str);
	exit(1);
}

//ʵ�������ʼ��
//����ֵ���׽���
//�˿�
//������port��ʾ�˿�
// ���*port��ֵ��0�� ��ô���Զ�����һ�����õĶ˿�
int startup(unsigned short *port)
{
	//1.����ͨ�ų�ʼ��(windowsר��)
	WSADATA data;
	int ret = WSAStartup(MAKEWORD(1, 1), &data);
	if (ret)
	{
		error_die("WSAStartup");
	}

	//2.�����׽���
	int server_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server_socket == -1)
	{
		//��ӡ������ʾ
		error_die("socket error");
	}

	//3.���ö˿ڸ���
	int opt = 1;
	ret = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
	if (ret == -1)
	{
		error_die("setsockopt error");
	}

	//4.���׽��ֺ������ַ
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(*port);
	server_addr.sin_addr.s_addr = inet_addr("10.16.60.26");

	ret = bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if (ret == -1)
	{
		error_die("bind error");
	}

	//��̬����˿�
	int nameLen = sizeof(server_addr);
	if (*port == 0)
	{
		if (getsockname(server_socket, (struct sockaddr*)&server_addr, &nameLen) < 0)
		{
			error_die("getsockname error");
		}

		*port = server_addr.sin_port;
	}

	//5.������������
	ret = listen(server_socket, 5);
	if (ret < 0)
	{
		error_die("listen error");
	}

	return server_socket;
}

//��ָ���Ŀͻ����׽��֣���ȡһ�����ݣ������ڲ���buff��
//����ʵ�ʶ�ȡ�����ֽ�
int get_line(int sock, char* buff, int size)
{
	char c = 0;
	int i = 0;

	while ( i < size -1 && c != '\n')
	{
		int n = recv(sock, &c, 1, 0);
		if (n > 0)
		{
			if (c == '\r')
			{
				n = recv(sock, &c, 1, MSG_PEEK);
				if (n > 0 && c == '\n')
				{
					recv(sock, &c, 1, 0);
				}
				else
				{
					c = '\n';
				}
			}
			buff[i++] = c;
		}
		else
		{
			c = '\n';
		}
	}

	buff[i] = 0;
	return i;
}

void unimplement(int client)
{

}

void not_found(int client)
{
	char buff[1024];

	strcpy(buff, "HTTP/1.0 404 NOT FOUND\r\n");
	send(client, buff, strlen(buff), 0);

	strcpy(buff, "Server: LXY/1.0\r\n");
	send(client, buff, strlen(buff), 0);

	strcpy(buff, "Content-type:text/html\n");
	send(client, buff, strlen(buff), 0);

	strcpy(buff, "\r\n");
	send(client, buff, strlen(buff), 0);

	//����404��ҳ����
	/*
	<HTML>					\
		<TITLE>Not Found</TITLE> \
		<BODY>\
			<H2>The resource is unavailable.</H2>\
			<img src=\"404.png\" />\
		</BODY>\
	</HTML>
	*/

	sprintf(buff,
		"	<HTML>					\
		<TITLE>Not Found</TITLE> \
		<BODY>\
			<H2>The resource is unavailable.</H2>\
			<img src=\"404.png\" />\
		</BODY>\
	</HTML>");

	send(client, buff, strlen(buff), 0);
}

void headers(int client, const char* type)
{
	char buff[1024];

	strcpy(buff, "HTTP/1.0 200 OK\r\n");
	send(client, buff, strlen(buff), 0);

	strcpy(buff, "Server: LXY/1.0\r\n");
	send(client, buff, strlen(buff), 0);

	char buf[1024];
	sprintf(buf, "Content-type:%s\r\n", type);
	send(client, buff, strlen(buff), 0);



	strcpy(buff, "\r\n");
	send(client, buff, strlen(buff), 0);

}

const char* getHeadType(const char* fileName)
{
	const char* ret = "text/html";
	const char* p = strrchr(fileName, '.');
	if (!p) return ret;

	p++;
	if (!strcmp(p, "css")) ret = "text/css";
	else if (!strcmp(p, "jpg")) ret = "image/jpeg";
	else if (!strcmp(p, "png")) ret = "image/png";
	else if (!strcmp(p, "js")) ret = "application/x-javascript";

	return ret;
}
void cat(int client, FILE* resource)
{
	char buff[4096];
	int count = 0;
	while (1)
	{
		int ret = fread(buff, sizeof(char), sizeof(buff), resource);
		if (ret <= 0)
		{
			break;
		}
		send(client, buff, ret, 0);
		count += ret;
	}

	printf("һ��������[%d]���ֽڸ������\n", count);

}

void server_file(int client, const char* fileName)
{
	int numchars = 1;
	char buff[1024];

	//���������ݰ���ʣ�������У�����
	while (numchars > 0 && strcmp(buff, "\n"))
	{
		numchars = get_line(client, buff, sizeof(buff));
		PRINTF(buff);
	}

	FILE* resource = NULL;
	if (strcmp(fileName, "htdocs/index.html") == 0)
	{
		resource = fopen(fileName, "r");
	}
	else
	{
		resource = fopen(fileName, "rb");
	}

	if (resource == NULL)
	{
		not_found(client);
	}
	else
	{
		//��ʽ������Դ�������
		headers(client, getHeadType(fileName));
		cat(client, resource);

		printf("��Դ�������!\n");
	}

	fclose(resource);
}
//�����û�������̺߳���
DWORD WINAPI accept_request(LPVOID arg)
{
	char buff[1024];

	int client = (SOCKET)arg;

	//��ȡһ������
	int numchars = get_line(client, buff, sizeof(buff));
	PRINTF(buff);

	char method[255];
	int j = 0, i = 0;
	while (!isspace(buff[j]) && i < sizeof(method) - 1)
	{
		method[i++] = buff[j++];
	}
	method[i] = 0;
	PRINTF(method);

	//�������ķ��������������Ƿ�֧��
	/*if (stricmp(method, "GET") || stricmp(method, "POST"))
	{
		unimplement(client);
		return 0;
	}*/
	//������Դ·�� www.rock.com/abc/test.thml
	char url[255]; //����������Դ������·��
	i = 0;
	//������Դ·��ǰ��Ŀո�
	while (isspace(buff[j]) && j < sizeof(buff))
	{
		j++;
	}

	while (!isspace(buff[j]) && i < sizeof(url) - 1 && j < sizeof(buff))
	{
		url[i++] = buff[j++];
	}

	url[i] = 0;
	PRINTF(url);

	//www.rock.com
	
	char path[512] = "";
	sprintf(path, "htdocs%s", url);
	if (path[strlen(path) - 1] == '/')
	{
		strcat(path, "index.html");
	}
	PRINTF(path);

	struct stat status;
	if (stat(path, &status) == -1)
	{
		//�������ʣ�����ݶ�ȡ���
		while (numchars > 0 && strcmp(buff, "\n"))
		{
			numchars = get_line(client, buff, sizeof(buff));
		}
		not_found(client);
	}
	else
	{
		if ((status.st_mode & S_IFMT) == S_IFDIR)
		{
			strcat(path, "/index.html");
		}

		server_file(client, path);
	}

	closesocket(client);
	return 0;
}

int main(void)
{
	unsigned short port = 80;
	int server_socket = startup(&port);
	printf("Http Runing��Listening [%d] Port ...", port);

	struct sockaddr_in client_addr;
	int client_addr_len = sizeof(client_addr);

	while (1)
	{	
		int client_sock = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
		if (client_sock == -1)
		{
			error_die("accept error");
		}

		//����һ���µ��߳�
		DWORD threadId = 0;
		CreateThread(0, 0, accept_request, (void*)client_sock, 0, &threadId);
	}

	closesocket(server_socket);
	return 0;

}