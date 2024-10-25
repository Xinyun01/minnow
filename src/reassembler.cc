#include "reassembler.hh"
#include <iostream>
#include <utility>
#include <vector>
using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // Your code here.
  //首先要判断是不是字节最后一组
  if(is_last_substring)
  {
    total_pushed_len_ = first_index +data.length();
  }
  //判断是数据决定是否插入还是暂存
  insert_or_store(first_index,std::move( data ));
  //决定是否插入还是暂存
  write_stored_str();
  if ( total_pushed_len_.has_value() && output_.writer().bytes_pushed() == *total_pushed_len_ ) [[unlikely]] 
  {
    output_.writer().close();
  }
}


void Reassembler::insert_or_store(uint64_t first_index, std::string data)
{
  if ( first_index < next_index_ ) {
    first_index = truncate_head( first_index, data );//当前收到的自序小于期望自序截断处理
  }
  if ( first_index > next_index_ ) {
    store( first_index, std::move( data ) ); //当前收到的自序大于期望自序存储处理
  } else {
    write( std::move( data ) );//当前收到的自序恰好为期望自序直接写入
  }
 // ==  >  <
}

void Reassembler::write_stored_str()
{
  for ( auto& [first_index, data] : pending_substr_ ) 
  {
    if ( first_index <= next_index_ ) 
    {
      auto buf = std::exchange( data, "" );
      bytes_pending_ -= buf.length();
      insert_or_store( first_index, std::move( buf ) );
    }
  }
  std::erase_if( pending_substr_, []( const auto& elem ) { return elem.second.empty(); } );

}
uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return bytes_pending_;
}
//截断处理
 uint64_t Reassembler::truncate_head( uint64_t first_index, std::string& data ) //截断头部  该函数会截断掉这些已经处理的字节，只保留尚未处理的部分
 {
     data.erase( 0, next_index_ - first_index );//使用引用对data进行截断处理
     return next_index_;
 }
void Reassembler::write( std::string data )
{
  //output_ 是 ByteStream类  ByteStream类中有Writer& writer();函数
  //class Writer : public ByteStream  含有下面函数
  // void push( std::string data ); // Push data to stream, but only as much as available capacity allows.
  //output_.writer() 调用 ByteStream 的 writer() 方法，返回一个 Writer 对象的引用。
  output_.writer().push( std::move( data ) );

  //bytes_pushed：Total number of bytes cumulatively pushed to the stream 
  //多少字节已经推送到流中
  next_index_ = output_.writer().bytes_pushed(); 
}

//进行存储到map中间
void Reassembler::store( uint64_t first_index, std::string data )
{
  //先检查容量是否够
  
  if (auto len = output_.writer().available_capacity() - (first_index - next_index_ );
    data.length() >= len)
  {
    //容量不够  删除data开始长度len以后的字节
    data.erase(data.begin()+len,data.end());
  }
  //删除后字节如果为空直接返回了 也间接说明没有空间传送
  if(data.empty())
  {
    return;
  }

  //使用map进行存储 存储又分是否存在当前键值 重复 接头 或者去尾
  //如果为空直接存储
  if(pending_substr_.empty())
  {
    //计算pending_substr_含有的字节长度
    bytes_pending_ += data.length();
    //将传入来的数据送入map容器中
    pending_substr_.emplace(first_index,std::move(data));  
  }
  else 
  {
    //情况1  map 中间已经存在first_index 
    auto final_index =  first_index + data.length() -1;

    if( pending_substr_.count(first_index) ) //count() 函数：直接返回某个键是否存在，存在返回 1，不存在返回 0。
    {
      if ( pending_substr_[first_index].length() >= data.length() ) 
      {
        return;
      }
      //将first 相关的数据删除  后面直接
      bytes_pending_ -=pending_substr_[first_index].length();
      pending_substr_.erase( first_index );
      
    }
    //情况2  map 中间不存在first_index 考虑是否覆盖
    //新传进来的元素和map元素进行对比是覆盖还是被覆盖的问题
    for ( auto it = pending_substr_.begin(); it != pending_substr_.end(); ) 
    {
      const auto& idx = it->first;
      const auto& str = it->second;
      
      //情况1  新传来的元素将原始元素覆盖完全 将原始元素包括键值全部删除在后续处理
      // 检查当前子字符串是否完全被新数据覆盖
      if ( idx >= first_index && idx + str.length() - 1 <= final_index ) 
      {
          bytes_pending_ -= str.length();
          it = pending_substr_.erase(it);  //会在删除元素后返回下一个有效的迭代器，这样可以避免对已删除元素的访问。完成++it
      }
       //情况2  新传来的元素被原始元素覆盖完全 新传进来的元素直接丢弃 
      else if (first_index >=idx && final_index <= idx + str.length() - 1)
      {
          return ;
      }
      else 
      {
          ++it;
      }
    }
   

   
    //后续处理
    bytes_pending_ += data.length();
    pending_substr_.emplace( first_index, std::move( data ) );
    //进行合并处理
    //这个调用会返回一个迭代器，指向第一个不小于 first_index 的元素。
    //如果 first_index 存在于 pending_substr_ 中，返回该元素的迭代器；否则，返回下一个大于 first_index 的元素
    auto begin_node = pending_substr_.lower_bound( first_index );
    //这个调用会返回一个迭代器，指向第一个大于 final_index 的元素
    //这意味着该迭代器指向的元素的索引会大于 final_index，即包含范围内的最后一个有效元素的下一个位置
    auto end_node = pending_substr_.upper_bound( final_index );
    //进行合并处理指向first_index的前一个元素
    if( begin_node != pending_substr_.begin() )
    {
      begin_node = std::prev( begin_node );
    }

    for ( auto node = begin_node; std::next( node ) != end_node; ++node ) 
    {
      auto next_node = std::next( node );
      auto this_final_index = node->first + node->second.length() - 1;
      auto next_first_index = next_node->first;
      //如果前一个元素的结尾大于后一个元素的开始说明这2个元素重合可以进行合并处理，bytes_pending_也需要减去重复的部分
      if ( this_final_index >= next_first_index )
      {
        auto len = this_final_index - next_first_index + 1;
        bytes_pending_ -= len;
        //删除当前元素重复的部分
        node->second.erase( node->second.begin() + node->second.length() - len, node->second.end() );
      }
    }
    

  }

}
