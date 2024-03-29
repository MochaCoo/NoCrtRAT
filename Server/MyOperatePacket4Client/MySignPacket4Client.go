package MyOperatePacket4Client

import (
	"encoding/binary"
	"io"
)
/*
type MyRecvSignPacket struct {
	r io.Reader
	sign uint32//根据标志确定封包类型: 心跳包/客户端数据包
}
type MySendSignPacket struct {
	w io.Writer
	sign uint32//根据标志确定封包类型: 心跳包/客户端数据包
}

func  InitRecvSignPacket(r io.Reader) MyRecvSignPacket{
	return MyRecvSignPacket{r:r}
}

func  InitSendSignPacket(w io.Writer) MySendSignPacket{
	return MySendSignPacket{w: w}
}
*/

func WriteSign(w io.Writer,v uint32) error{
	e:=binary.Write(w, binary.LittleEndian, &v)
	return e
}