#ifndef MEMORY_POOL_TCC
#define MEMORY_POOL_TCC

#include "MemoryPool.h"

//计算对齐所需补的空间
template <typename T, size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::size_type
MemoryPool<T, BlockSize>::padPointer(data_pointer_ p, size_type align) const noexcept {
    uintptr_t result = reinterpret_cast<uintptr_t>(p); //将指针转换为整数，计算其所占有的空间
    return ((align - result) % result); //多余不够一个Slot槽大小的空间，需要跳过
}

// 构造函数，初始化所有指针为 nullptr
template <typename T, size_t BlockSize>
MemoryPool<T, BlockSize>::MemoryPool() noexcept {
    currentBlock_ = nullptr;  //指向第一块Block区 即Block内存块链表的头指针
    currentSlot_ = nullptr;   //当前第一个可用槽的位置
    lastSlot_ = nullptr;  //最后可用Slot的位置
    freeSlot_ = nullptr;  //空闲链表头指针
}

/*
拷贝构造函数
这个构造函数的作用是用一个已有的内存池（memoryPool）来初始化一个新的内存池，
它使用了noexcept关键字来表明这个操作不会抛出异常。
它调用了MemoryPool()的默认构造函数来设置一些默认值
*/
template <typename T, size_t BlockSize>
MemoryPool<T, BlockSize>::MemoryPool(const MemoryPool& memoryPool)
noexcept: MemoryPool() {}


/*
移动构造函数
*/
template <typename T, size_t BlockSize>
MemoryPool<T, BlockSize>::MemoryPool(MemoryPool&& memoryPool) noexcept {
    this->currentBlock_ = memoryPool.currentBlock_;
    memoryPool.currentBlock_ = nullptr;
    this->currentSlot_ = memoryPool.currentSlot_;
    // memoryPool.currentSlot_ = nullptr; // 为什么不置为空？
    this->lastSlot_ = memoryPool.lastSlot_;
    this->freeSlot_ = memoryPool.freeSlot_;
}


/*
构造函数
*/
template <typename T, size_t BlockSize>
template <class U>
MemoryPool<T, BlockSize>::MemoryPool(const MemoryPool<U>& memoryPool)
noexcept: MemoryPool() {}


/*
移动赋值
*/
template <typename T, size_t BlockSize>
MemoryPool<T, BlockSize>&
MemoryPool<T, BlockSize>::operator=(MemoryPool&& memoryPool) noexcept {
    if (this != &memoryPool) { //自赋值检测
        std::swap(this->currentBlock_, memoryPool.currentBlock_);
        this->currentSlot_ = memoryPool.currentSlot_;
        // memoryPool.currentSlot_ = nullptr; // 为什么不置为空？
        this->lastSlot_ = memoryPool.lastSlot_;
        this->freeSlot_ = memoryPool.freeSlot_;
    }
}

/*
析构函数
*/
template <typename T, size_t BlockSize>
MemoryPool<T, BlockSize>::~MemoryPool() noexcept {
    slot_pointer_ curr = currentBlock_;
    while (curr != nullptr) {
        slot_pointer_ prev = curr->next;
        // 转化为 void 指针，是因为 void 类型不需要调用析构函数,只释放空间
        operator delete(reinterpret_cast<void*>(curr));
        curr = prev;
    }
}


template <typename T, size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::pointer
MemoryPool<T, BlockSize>::address(reference x) const noexcept {
    return &x;
}


template <typename T, size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::constPointer
MemoryPool<T, BlockSize>::address(constReference x) const noexcept {
    return &x;
}

/*
申请一块新的分区Block
*/
template <typename T, size_t BlockSize>
void MemoryPool<T, BlockSize>::allocateBlock() {
    //  Allocate space for the new block and store a pointer to the previous one
    //  申请 BlockSize字节大小的一块空间并用char* 类型指针接收
    data_pointer_ newBolck = reinterpret_cast<data_pointer_>(operator new(BlockSize));
    //  newBlock成为新的block内存首址
    reinterpret_cast<slot_pointer_>(newBolck)->next = this->currentBlock_;
    //  currentBlock 更新为 newBlock的位置
    currentBlock_ = reinterpret_cast<slot_pointer_>(newBolck);
    //  Pad block body to staisfy the alignment requirements for elements
    //  保留第一个slot 用于Block链表的链接
    data_pointer_ body = newBolck + sizeof(slot_pointer_);
    // 求解空间对齐所需的字节数
    size_type  bodyPadding = padPointer(body, alignof(slot_type_));
    //若出现残缺不可以作为一个slot的空间，跳过这块空间作为第一个可用slot的位置
    currentSlot_ = reinterpret_cast<slot_pointer_>(body + bodyPadding);
    /*
    始址: newBlock  块大小 BlockSize 末址 newBlock + BlockSize 末址减去一个slot槽大小
    得到倒数第二个slot的末址 再加一得到最后一块slot的始址
    */
    lastSlot_ = reinterpret_cast<slot_pointer_>(newBolck + BlockSize - sizeof(slot_type_) + 1);
}


template <typename T, size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::pointer
MemoryPool<T, BlockSize>::allocate(size_type n, constPointer hint) {
    pointer result;
    if (freeSlot_ != nullptr) {
        result = reinterpret_cast<pointer>(freeSlot_);
        freeSlot_ = freeSlot_->next;
    } else {
        // 如果当前可用slot槽已经到了最后的位置,用完之后需要再创建一块Block方便后续操作
        if (currentBlock_ >= lastSlot_) {
            allocateBlock();
            //返回分配的空间并更新 currentSlot
            //currentSlot 指向Block内存块中最前面可用的Slot槽 (freeSlot链表中不算)
            result = reinterpret_cast<pointer>(currentBlock_++);
        }
    }
    return result;
}


template <typename T, size_t BlockSize>
inline void MemoryPool<T, BlockSize>::deallocate(pointer p, size_type n) {
    if (p != nullptr) {
        //union指针域被赋值 之前的数据销毁
        reinterpret_cast<slot_pointer_>(p)->next = freeSlot_;
        //空闲链表头结点更新
        freeSlot_ = reinterpret_cast<slot_pointer_>(p);
    }
}


//计算最大可用槽Slot的个数
template <typename T, size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::size_type
MemoryPool<T, BlockSize>::maxSize() const noexcept {
    // 无符号整型 unsiged int 类型运算,先将 -1 转换为无符号整型 即无符号整型最大值
    // 无符号类型数值可以表示的Block的个数
    // unsigned int MAX / BlockSize 向下取整
    size_type maxBlock = -1 / BlockSize;

    // (BlockSize - sizeof(data_pointer)) / sizeof(slot_type_) →指每一块Block中可以存放的slot槽个数
    // 最大可以有 maxBlocks 块内存块
    // 因此最大的可用槽个数 为每一块Block可以使用的Slot槽个数乘以最大可用Block
    return (BlockSize - sizeof(data_pointer_) / sizeof(slot_type_) * maxBlock);
}


/*
construct函数的作用是在内存池中给定的地址p处，使用new运算符和完美转发（perfect forwarding）的技术，
调用U类型的构造函数，将可变参数包args转发给U的构造函数12。这样可以在内存池中创建任意类型和数量的参数的对象。

construct函数是一个模板函数，它有两个模板参数：U和Args。U是要构造的对象的类型，Args是一个可变参数包，
表示U构造函数的参数列表。construct函数的第一个参数是一个U类型的指针，表示要构造对象的地址；
第二个参数是一个可变参数包，表示要转发给U构造函数的参数。
construct函数使用了C++11的新特性：变长模板（variadic template），完美转发（perfect forwarding），
以及new运算符的定位形式（placement new）12。
*/
template <typename T, size_t BlockSize>
template <class U, class... Args>
inline void MemoryPool<T, BlockSize>::construct(U* p, Args&&... args) {
    //完美转发 保存参数原有的属性 是左值还是左值 是右值还是右值
    //placement new 的应用，即在已经申请的空间上申请内存分配
    new (p) U(std::forward<Args>(args)...);
}


// 调用析构函数释放指针所指的内存
template <typename T, size_t BlockSize>
template <class U>
inline void MemoryPool<T, BlockSize>::destroy(U* p) {
    p->~U();
}


template <typename T, size_t BlockSize>
template <class... Args>
inline typename MemoryPool<T, BlockSize>::pointer
MemoryPool<T, BlockSize>::newElement(Args&&... args) {
    pointer result = allocate();
    construct<value_type>(result, std::forward<Args>(args)...); // 参数包解包
    //返回每一个新申请空间的地址
    return result;
}


// 删除元素
template <typename T, size_t BlockSize>
inline void MemoryPool<T, BlockSize>::deleteElement(pointer p) {
    if (p != nullptr) {
        p->~value_type(); // 调用值类型调用值类型的析构函数  typedef T  value_type;
        deallocate(p);
    }
}

#endif // MEMORY_POOL_TCC
