package main

import (
	"MyOperatePacket4Client"
	"bufio"
	"fmt"
	"net"
	"os"
	"sync"
	"time"
)

func ReadFileContents(fileName string) ([]byte, error) {
	file, err := os.Open(fileName)
	if err != nil {
		return nil, err
	}
	defer file.Close()

	stats, err := file.Stat()
	FileSize := stats.Size()

	bytes := make([]byte, FileSize)

	buffer := bufio.NewReader(file)

	_, err = buffer.Read(bytes)

	return bytes, err
}

//sign封包编号
const (
	HEARTBEAT uint32 = iota
	CLIENT_PACKET uint32 = iota
)

//封包功能编号
const (
	UPLOAD uint32 = iota
	DOWNLOAD uint32 = iota
)
var m sync.RWMutex

func HeartBeat(c net.Conn){
	for{
		m.Lock()//因为是两个协程都在发数据包所以要加锁
		e:=MyOperatePacket4Client.WriteSign(c,HEARTBEAT)
		if e!=nil{fmt.Println(e);return}
		m.Unlock()
		time.Sleep(10*time.Second)//心跳包每10秒发一次
	}

}

func main() {
	// 连接服务器
	conn,_ := net.Dial("tcp","127.0.0.1:9999")
	s:=MyOperatePacket4Client.InitSendPacket(conn)
	r:=MyOperatePacket4Client.InitRecvPacket(conn)
	//客户端只是一个例子,会用C重写,所以很多地方没有判断,只是来说明运作原理和服务器框架的使用,比如服务器如何发送命令和接收客户端发来的的执行结果反馈封包
	go HeartBeat(conn)
	for {
		e:=r.RecvPacket()//客户端的RecvPacket()和服务器的RecvPacket()实现是不一样的

		if e!=nil{fmt.Println(e);return}
		v,e:=r.ReadUint32()//读命令编号
		switch v {//开始判断是什么命令
		case UPLOAD:
			time.Sleep(5*time.Second)
			fmt.Println(r.ReadString())//读需要输出到的目标机器的文件路径
			fmt.Println(r.ReadBytes())//读发过来的文件内容
			//这里只是简单展示下读取封包内容,没写入文件
			s.WriteUint32(0)//发送执行结果反馈包

			m.Lock()//因为是心跳包发送协程可能此时在发送数据所以要加锁
			MyOperatePacket4Client.WriteSign(conn,CLIENT_PACKET)
			s.SendPacket()
			m.Unlock()
		case DOWNLOAD:
			path,_:=r.ReadString()
			fmt.Println(path)

			s.WriteUint32(0)//成功
			b,e:=ReadFileContents(path)
			fmt.Println(e,b)
			s.WriteBytes(b)

			m.Lock()//因为是心跳包发送协程可能此时在发送数据所以要加锁
			MyOperatePacket4Client.WriteSign(conn,CLIENT_PACKET)
			s.SendPacket()
			m.Unlock()
		}
	}

}