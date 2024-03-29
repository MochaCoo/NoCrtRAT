package MyOperatePacket4Client

import (
	"MyOperatePacket4Client/MyBuf"
	"encoding/binary"
	"errors"
	"golang.org/x/text/encoding/unicode"
	"io"
	"reflect"
	"strconv"
	"unsafe"
)
const maxsize uint32=50*1024*1024//50MB
const magicNum uint32=0xcafefafa
//数据包格式: 魔数+长度+数据(支持3种数据类型int32,string,[]byte)
//数据格式: 字符串(string)和字节数组([]byte)格式均为: 长度+内容(string含终止符)

type MyRecvPatket struct {
	reader io.Reader
	magic uint32
	length uint32
	data MyBuf.Buffer
}
type MySendPatket struct {
	writer io.Writer
	magic uint32
	data MyBuf.Buffer
}

func bytes2string(b []byte) string{
	sliceHeader := (*reflect.SliceHeader)(unsafe.Pointer(&b))
	sh := reflect.StringHeader{
		Data: sliceHeader.Data,
		Len:  sliceHeader.Len,
	}
	return *(*string)(unsafe.Pointer(&sh))
}

func string2bytes(s string) []byte {
	stringHeader := (*reflect.StringHeader)(unsafe.Pointer(&s))
	bh := reflect.SliceHeader{
		Data: stringHeader.Data,
		Len:  stringHeader.Len,
		Cap:  stringHeader.Len,
	}
	return *(*[]byte)(unsafe.Pointer(&bh))
}

func  InitSendPacket(w io.Writer) *MySendPatket{
	return &MySendPatket{writer: w,magic: magicNum}
}

func (p *MySendPatket) SendPacket() error{//发送完毕后清空数据包内容
	e:=binary.Write(p.writer,binary.LittleEndian,p.magic)//写魔数
	if e!=nil{return e}
	e=binary.Write(p.writer,binary.LittleEndian,uint32(p.data.Len()))//写长度
	if e!=nil{return e}
	_,e=p.writer.Write(p.data.Bytes())//写内容
	if e!=nil{return e}

	if p.data.Cap()>int(maxsize) {//之前传输了很大的文件时,让GC回收这部分内存
		p.data=*MyBuf.NewBuffer(make([]byte,1024))
	}else{
		p.data.Reset()
	}
	return e
}

func (p *MySendPatket) WriteUint32(v uint32) error{
	e:=binary.Write(&p.data,binary.LittleEndian,v)
	return e
}

func (p *MySendPatket) WriteBytes(b []byte) (uint32,error){
	e:=binary.Write(&p.data,binary.LittleEndian,uint32(len(b)))

	if e!=nil{return 0,e}

	n,e:=p.data.Write(b)
	return uint32(n),e
}

func (p *MySendPatket) WriteString(s string) (uint32,error){
	encoder := unicode.UTF16(unicode.LittleEndian, unicode.IgnoreBOM).NewEncoder()//UTF8转UTF16
	utf16,e:=encoder.Bytes(string2bytes(s))
	utf16=append(utf16, 0,0)//终止符

	e=binary.Write(&p.data,binary.LittleEndian,uint32(len(utf16)))

	if e!=nil{return 0,e}

	n,e:=p.data.Write(utf16)
	return uint32(n),e
}

func  InitRecvPacket(r io.Reader) *MyRecvPatket{
	return &MyRecvPatket{reader:r,magic: magicNum,length: 0}
}

func (p *MyRecvPatket) RecvPacket() error{//接收之前清空数据包内容
	if p.data.Cap()>int(maxsize) {//之前传输了很大的文件时,让GC回收这部分内存
		p.data=*MyBuf.NewBuffer(make([]byte,1024))
	}else{
		p.data.Reset()
	}

	var v uint32
	e:=binary.Read(p.reader, binary.LittleEndian, &v)//读魔数

	if e!=nil{return e}
	if v!=magicNum{return errors.New("wrong packet format: magicNum: "+strconv.FormatUint(uint64(v),16))}

	e=binary.Read(p.reader, binary.LittleEndian, &v)//读长度
	if e!=nil && e!=io.EOF{return e}

	//简易长度验证,恶意封包可能会导致panic
	if int(v)<0{return errors.New("wrong packet format: packet length")}

	_,e=p.data.ReadFull(p.reader,int(v))//读内容,保证这个数据包的内容一定是全部都存储在内存中了

	if e!=nil && e!=io.EOF {return e}

	return nil
}

//ReadUint32 一定要有错误处理,应对数据包格式错误的情况
func (p *MyRecvPatket) ReadUint32() (uint32,error){
	var v uint32
	e:=binary.Read(&p.data, binary.LittleEndian, &v)
	if e!=nil {return 0,e}
	return v,e
}

//ReadBytes 一定要有错误处理,应对数据包格式错误的情况
func (p *MyRecvPatket) ReadBytes() ([]byte,error) {
	var v uint32
	e:=binary.Read(&p.data, binary.LittleEndian, &v)

	if e!=nil {return nil,e}
	if v > uint32(p.data.Len()){return nil,errors.New("wrong packet format: bytes length")}

	b:=p.data.Next(int(v))

	return b,e
}

// ReadString 一定要有错误处理,应对数据包格式错误的情况
func (p *MyRecvPatket) ReadString() (string,error){
	var v uint32
	e:=binary.Read(&p.data, binary.LittleEndian, &v)

	if e!=nil {return "",e}
	if v > uint32(p.data.Len()) || v<2 {return "",errors.New("wrong packet format: string length")}

	s:=p.data.Next(int(v))

	decoder := unicode.UTF16(unicode.LittleEndian, unicode.IgnoreBOM).NewDecoder()//UTF16转UTF8
	utf8, e := decoder.Bytes(s[:len(s)-2])//去掉终止符
	if e!=nil {return "",e}

	return bytes2string(utf8),e
}