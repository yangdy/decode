说明
================================

diag_decode(diag_decode.h/.c)是解码器的入口，整个解码过程就是通过code值层层查表，找到相应的解码器进行解码。目前是线性表，后期如出现性能问题，再改成hash表。

diag_decode_result.h中定义了返回的解码后数据。这个是根据用户需求来确定的。我们接下来要做的是，规范这个结果集，输出的解码后数据尽量的能覆盖不同用户的需求。

原始日志数据的数据结构见各个具体的解码器的实现文件中头部的结构体定义。如：log_data_decode_0x108a.c中，前面部分的结构体定义。

