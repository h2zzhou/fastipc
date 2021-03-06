#include "fastipc_4csharp.h"
#include "common/util/StrUtil.h"
#include "common/fastipc/Server.h"
#include "common/fastipc/Client.h"
#ifdef JWebTopLog
#include "common/tests/TestUtil.h"
#endif
using namespace std;

// 把wstring转为char*返回给csharp端
char* wstring2char(wstring ws){
	string s = jw::w2s(ws);
	char* c;
	const int len = s.length();
	c = new char[len + 1];
	//strcpy(c, s.c_str());
	memcpy(c, s.c_str(), len);
	c[len] = '\0';
	return c;
}

class CSharpIPCReadListener :public fastipc::ReadListener{// 这里不采用RebuildedBlockListener，数据的组装留给csharp侦听器去做
private:
	CSharpCallBack g_CSharpCallBack;  //回复信息的回调函数
public:
	void setListener(CSharpCallBack listener){
		g_CSharpCallBack = listener;
	}
	// 当有服务端读取到数据后，会调用此方法通知等待者进行处理
	// memBlock在分发后会由服务器销毁，外部调用者无需清理操作
	void onRead(fastipc::MemBlock* memBlock) override{
		if (!g_CSharpCallBack)return;
		// FIXME:packId、userShortValue是否也可以不进行新建，直接传出去用？
		//LPSTR packId = NULL;
		char * packId = NULL;
		if (memBlock->msgType != MSG_TYPE_NORMAL){
			packId = (char *)malloc(PACK_ID_LEN + 1);
			memcpy(packId, memBlock->packId, PACK_ID_LEN);
			packId[PACK_ID_LEN] = '\0';
			//string pid = memBlock->getPackId();
			//packId = LPSTR(pid.c_str());
		}
		//LPSTR userShortValue = NULL;
		//userShortValue = (char *)malloc(PACK_ID_LEN + 1);
		//memcpy(userShortValue, memBlock->userShortStr, PACK_ID_LEN);
		//userShortValue[PACK_ID_LEN] = '\0';
		// FIXME:data是否也可以不进行新建，把len作为传出参数
		int len = memBlock->dataLen;
		char * data = NULL;
		data = (char *)malloc(len);
		memcpy(data, memBlock->data, len);

		g_CSharpCallBack(memBlock->msgType, packId, data, len);
		delete data;
		if (packId)delete packId;
	}
};


/*
* 该方法用于创建一个fastipc服务端
* serverName	服务端名称
* blockSize		服务端与客户端通信时每次的数据量
* result		（传出参数）是否创建成功，0=成功
* return		创建的Server的指针
*/
EXPORT long WINAPI nCreateServer(LPTSTR serverName, int blockSize, int& result){
	fastipc::Server * server = new fastipc::Server();
	result = server->create(serverName, blockSize);
	return (int)server;
}
EXPORT void WINAPI nAbc(){

}
/*
* 该方法用于开启fastipc服务端的读取侦听，成功后会阻塞在此处
* nativeServer	指向服务端实例的指针
* listener		响应读取操作的指针
*/
EXPORT void WINAPI nStartRead(int nativeServer, CSharpCallBack callBack){
	fastipc::Server * server = (fastipc::Server *) nativeServer;
	CSharpIPCReadListener * ipcListener = new CSharpIPCReadListener();
	ipcListener->setListener(callBack);
	server->setListener(ipcListener);
	server->startRead();
}
/*
* 该方法用于关闭fastipc服务端，执行后nStartRead的阻塞会停止
* nativeServer	指向服务端实例的指针
*/
EXPORT void WINAPI nCloseServer(int nativeServer){
	fastipc::Server * server = (fastipc::Server *) nativeServer;
	server->close();
}

/*
* 该方法用于创建一个fastipc客户端
* serverName	客户端对应的服务端的名称
* blockSize		服务端与客户端通信时每次的数据量
* result		（传出参数）是否创建成功，0=成功
* return		创建的Client的指针
*/
EXPORT int WINAPI nCreateClient(LPTSTR serverName, int blockSize, int& result){
	fastipc::Client * client = new fastipc::Client();
	result = client->create(serverName, blockSize);
	return (int)client;
}

/*
* 该方法用于早fastipc客户端写入数据，数据写完成后，会触发服务端的侦听
* nativeClient	指向客户端实例的指针
* data			要写入的数据
* return		成功写入的数据长度
*/
EXPORT int WINAPI nWriteClient(int nativeClient, LPTSTR data){
	fastipc::Client * client = (fastipc::Client *) nativeClient;
	char * shortStr = NULL;
	char * str = NULL;
	int len = 0;
	// data
	if (data != NULL){
		char*  tmp = wstring2char(data);
		len = lstrlenA(tmp);
		str = new char[len + 1];
		memcpy(str, tmp, len);
		delete tmp;
		str[len] = '\0';
	}	
	// 写到客户端，并清理当前申请的内存
#ifdef JWebTopLog
	std::wstringstream wss;
	wss << L"Writed "
		<< L" userMsgType=" << userMsgType
		<< L" userValue=" << userValue;
	if (shortStr != NULL)wss << L" userShortStr=" << shortStr;
	if (str != NULL)wss << L" data=" << str;
	wss << L"||\r\n";
	writeLog(wss.str());
#endif
	client->write(str, len);
	delete[] str;
	delete[] shortStr;
	return 0;
}

/*
* 该方法用于关闭fastipc客户端
* nativeClient	指向客户端实例的指针
*/
EXPORT void WINAPI nCloseClient(int nativeClient){
	fastipc::Client * client = (fastipc::Client *) nativeClient;
	client->close();
}