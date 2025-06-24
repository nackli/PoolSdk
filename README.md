#内存池主要由3个部分组成

##线程缓存 (thread cache)：

thread cache 其主要思想是为每个线程维护一个本地的内存缓存，以减少多线程并发操作时的锁竞争，提高内存分配的性能。thread cache 是每个线程独有的，用于小于256kb的内存分配，线程从这里申请内存不需要加锁，每个线程独享一个thread cache。这也是并发内存池高效的一个地方。

##中心缓存(central cache)：
中心缓存是所有线程共享的，thread cache 按需从central cache中获取对象。central cache周期性的回收thread cache的对象。避免一个线程占用太多的内存，导致其他线程内存吃紧。做到内存分配在多线程中更加均衡的按需调度的目的。central cache是存在竞争的，从这里取内存对象是需要加锁保护的。

##页缓存(page cache)：

页缓存是在central cache上面的一层缓存，存储的内存是以页为单位存储以及分配的。central cache没有内存时，从page cache分配出一定数量的page，并切割成定长大小的小块内存块，分配给central cache。page cache没有足够的内存时，就会向堆区申请。
