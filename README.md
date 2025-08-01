# 内存池主要由3个部分组成

## 线程缓存 (thread cache)：

thread cache 其主要思想是为每个线程维护一个本地的内存缓存，以减少多线程并发操作时的锁竞争，提高内存分配的性能。thread cache 是每个线程独有的，用于小于256kb的内存分配，线程从这里申请内存不需要加锁，每个线程独享一个thread cache。这也是并发内存池高效的一个地方。

## 中心缓存(central cache)：
中心缓存是所有线程共享的，thread cache 按需从central cache中获取对象。central cache周期性的回收thread cache的对象。避免一个线程占用太多的内存，导致其他线程内存吃紧。做到内存分配在多线程中更加均衡的按需调度的目的。central cache是存在竞争的，从这里取内存对象是需要加锁保护的。

## 页缓存(page cache)：

页缓存是在central cache上面的一层缓存，存储的内存是以页为单位存储以及分配的。central cache没有内存时，从page cache分配出一定数量的page，并切割成定长大小的小块内存块，分配给central cache。page cache没有足够的内存时，就会向堆区申请。

# 日志系统FileLogger
## 引言
日志系统无论是在嵌入式系统还是在PC端，都是关键的调试和监控手段。所以大家在选择日志系统时，都会进行不同选择。目前流行的几个日志系统，性能以及使用中往往各有千秋。FileLogger目前支持网络输出、文件输出、控制台输出，通过配置文件进行不同的选择。
## 配置文件说明
#file_name 日志文件保存目录以及名称前缀，支持绝对路径和相对路径例如
file_name=./Data/log/logfile.log
#单个日志文件最大数据量，采用的byte计算
max_size=52428800
#最大保存日志文件个数
max_files=5
#显示日志级别，不区分大小写，主要支持以下几种[trace、debug、info、warning、error、fatal] 
log_level=trace
#日志输出模式，可以控制台，文件，以及网络模式[console、file、netudp]，只有配置了网络模式，netIp才有效
out_put=netudp
netIp = 127.0.0.1:9000
#日志交互模式，可以同步也可以异步[async、sync]
out_mode = async
#日志输出格式，包括时间、级别、文件、线程ID，文件行号、文件名称 日志消息[{time} {level} {file} {tid} {line} {func} {message}]
log_format=[{time}] [{level}] [tid : {tid}] [line : {line}] [{func}] {message}