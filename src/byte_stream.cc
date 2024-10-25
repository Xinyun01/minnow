#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) 
  : capacity_( capacity ) , Byte_pushed( 0 ), Byte_poped( 0 ), Closed_flag( false )
{}

bool Writer::is_closed() const
{
  //返回bool 类型是否关闭
  return Closed_flag;
}

void Writer::push( string data )
{
  //流关闭或者发生错误时 不再推送
  if( is_closed() || has_error()){
    return;
  }
  //当正常推送时
  //判断当前推送长度是否大于可用容量
  const size_t len = min ( available_capacity() , data.length() );
  //将长度为len的数据推送至缓存区
  //append(const basic_string& __str)：这是一个用于将整个字符串__str追加到当前字符串的函数
  //data.substr(0, len) 从字符串 data 中提取从索引 0 开始的 len 个字符，并返回一个新的 std::string 对象
  buffer_.append( data.substr( 0 , len ) );
  Byte_pushed += len;
  return;
}
//function：完成写入
void Writer::close()
{
    Closed_flag =true;
    return;
}
//可用容量
uint64_t Writer::available_capacity() const
{
  //可用容量 = 总容量 - 堆栈中容量（推送的容量-弹出的容量）
  return capacity_ - ( Byte_pushed - Byte_poped ) ;
}

uint64_t Writer::bytes_pushed() const
{
  return Byte_pushed;
}

//是否读取完成
bool Reader::is_finished() const
{
  //当前写入完成并且buffer中没有字节
  return Closed_flag && bytes_buffered() == 0;
}

uint64_t Reader::bytes_popped() const
{
  //弹出数据
  return Byte_poped ;
}

string_view Reader::peek() const
{
  return this->buffer_;
}

//len : 表示能读取多少
void Reader::pop( uint64_t len )
{
  //当前buffer数据已经完成
  if( bytes_buffered() == 0 ){
    return;
  }
  
  len = min ( len , buffer_.length() );

  //累加弹出数据
  Byte_poped += len;
  buffer_.erase(0,len);
  return;
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  //多少数据在栈中
  //栈中数据 = 推送 - 弹出
  return Byte_pushed - Byte_poped;
}
//总结：
//每个函数都是拥自己的作用模块化编程，在byte_steam流中 buffer应该是不需要考虑后进先出