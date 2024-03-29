package CGOLZ4

/*
#include <lz4.h>
*/
import "C"
import (
	"errors"
	"reflect"
	"unsafe"
)
func LZ4_compress(src []byte) ([]byte,error){
	srcSize:=int32(len(src))
	if srcSize>int32(C.LZ4_MAX_INPUT_SIZE/* 2 113 929 216 bytes */) {return nil,errors.New("LZ4 Unsupported Size, too large (or negative)")}

	maxLen:=LZ4_compressBound(srcSize)
	compress:=make([]byte,maxLen+4,maxLen+4)//指定cap//4=sizeof(int32)

	cl:=LZ4_compress_default(src,compress[4:],srcSize,maxLen)
	if cl<=0 {return nil,errors.New("LZ4 compress failed")}

	//前4字节写入压缩前的大小
	*(*int32)(unsafe.Pointer((*reflect.SliceHeader)(unsafe.Pointer(&compress)).Data))=srcSize

	return compress[:cl+4],nil
}
func LZ4_decompress(src []byte) ([]byte,error){
	//读取前4字节压缩前的大小
	srcSize:=*(*int32)(unsafe.Pointer((*reflect.SliceHeader)(unsafe.Pointer(&src)).Data))
	if srcSize>int32(C.LZ4_MAX_INPUT_SIZE/* 2 113 929 216 bytes */) {return nil,errors.New("LZ4 Unsupported Size, too large (or negative)")}

	regen:=make([]byte,srcSize,srcSize)//指定cap

	decompressed_size:=LZ4_decompress_safe(src[4:],regen,int32(len(src)-4),srcSize)
	if decompressed_size < 0 {return nil,errors.New("LZ4 decompress failed")}
	if decompressed_size!=srcSize {return nil,errors.New("LZ4 decompress failed (wrong size)")}
	return regen,nil
}

func LZ4_compressBound(isize int32) int32{
	return int32(C.LZ4_compressBound(C.int(isize)))
}

func LZ4_compress_default(src,dst []byte,srcSize,maxOutputSize int32) int32{
	return int32(C.LZ4_compress_default(
		(*C.char)(unsafe.Pointer((*reflect.SliceHeader)(unsafe.Pointer(&src)).Data)),
		(*C.char)(unsafe.Pointer((*reflect.SliceHeader)(unsafe.Pointer(&dst)).Data)),
		C.int(srcSize),C.int(maxOutputSize)))
}

func LZ4_decompress_safe(src,dst []byte,compressedSize,dstCapacity int32) int32{
	return int32(C.LZ4_decompress_safe(
		(*C.char)(unsafe.Pointer((*reflect.SliceHeader)(unsafe.Pointer(&src)).Data)),
		(*C.char)(unsafe.Pointer((*reflect.SliceHeader)(unsafe.Pointer(&dst)).Data)),
		C.int(compressedSize),C.int(dstCapacity)))
}