package main

import "C"
import (
	"CGOLZ4"
	"fmt"
)

func main(){
	/*
	s:="Lorem ipsum dolor sit amet, consectetur adipiscing elit. Lorem ipsum dolor site amat."
	b:=[]byte(s)
	//b= append(b, 0)
	srcSize :=int32(len(b))
	fmt.Println(srcSize)

	max_l:=CGOLZ4.LZ4_compressBound(srcSize)

	compress:=make([]byte,max_l)
	cl:=CGOLZ4.LZ4_compress_default(b,compress,int32(len(b)),max_l)
	fmt.Println(cl)
	fmt.Println(compress)

	regen_buffer:=make([]byte,len(b))
	CGOLZ4.LZ4_decompress_safe(compress,regen_buffer,cl,srcSize)

	fmt.Println(string(regen_buffer))
	*/
	str:="Lorem ipsum dolor sit amet, consectetur adipiscing elit. Lorem ipsum dolor site amat."
	fmt.Println(len(str))
	b,e:=CGOLZ4.LZ4_compress([]byte(str))
	fmt.Println(e)
	s,e:=CGOLZ4.LZ4_decompress(b)
	fmt.Println(len(string(s)),string(s),e)
}
