#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <climits>
#include <cstddef>



//allocate    分配一个对象所需的内存空间
//
//deallocate   释放一个对象的内存（归还给内存池，不是给操作系统）
//
//construct   在已申请的内存空间上构造对象
//
//destroy  析构对象
//
//newElement  从内存池申请一个对象所需空间，并调用对象的构造函数
//
//deleteElement  析构对象，将内存空间归还给内存池
//
//allocateBlock  从操作系统申请一整块内存放入内存池


template <typename T, size_t BlockSize = 4096>
class MemoryPool {
public:
    // 成员类型
    typedef T value_type; // T的Value类型
    typedef T* pointer;
    typedef T& reference;
    typedef const T* constPointer;
    typedef const T& constReference;
    typedef size_t size_type;
    typedef ptrdiff_t differenceType; // 指针减法结果类型, 相当于指针间距离, 普通int类型
    typedef std::false_type propagate_on_container_copy_assinment;
    typedef std::true_type propagate_on_container_move_assinment;
    typedef std::true_type propagate_on_container_move_swap;

    /*
    rebind 是一个分配器 (Allocator) 的成员模板，它可以根据相同的策略得到另外一个类型的分配器。
    例如，如果你有一个 T 类型的分配器 A ，你可以用 A::rebind<U>::other 得到一个 U 类型的分配器 B 。
    这样做的目的是为了让容器可以根据不同的类型分配内存，而不用关心分配器的具体实现。
    */
    template <typename U> struct rebind {
        typedef MemoryPool<U> other;
    };

    /* 成员函数 */
    /*
    C++ 内存池设计中成员函数不能抛出异常的原因可能有以下几点：
    抛出异常会增加内存池的复杂度和开销，因为需要处理异常的传播和捕获。
    抛出异常会破坏内存池的封装性和安全性，因为可能导致内存泄漏或内存池状态不一致。
    抛出异常会降低内存池的可移植性和兼容性，因为不同的编译器和平台对异常的支持和处理方式可能不同 。
    因此，C++ 内存池设计中通常使用 noexcept 版本的 new 操作符来分配内存，如果分配失败，返回一个 NULL 指针，而不是抛出异常。
    这样可以避免上述问题，同时也可以让用户自己决定如何处理内存分配失败的情况。
    */
    MemoryPool() noexcept;
    MemoryPool(const MemoryPool& memoryPool) noexcept; // 拷贝构造函数
    MemoryPool(MemoryPool&& memoryPool) noexcept; // 移动构造函数

    /*
    模板类拷贝构造函数
    它接受一个不同类型的 MemoryPool 对象作为参数，并用它来初始化当前对象。这个函数有以下特点：
    它是一个模板函数，可以接受任何类型的 U ，只要 U 和 MemoryPool 的模板参数类型兼容。
    它是一个 noexcept 函数，表示它不会抛出任何异常，这样可以提高性能和安全性。
    它是一个 const 函数，表示它不会修改参数对象的状态，这样可以保证参数对象的不变性。
    */
    template <class U> MemoryPool(const MemoryPool<U>& MemoryPool) noexcept;

    ~MemoryPool() noexcept; // 析构函数

    /*
    拷贝赋值函数
    C++ 内存池设计中将拷贝赋值函数弃置或删除的原因可能有以下几点：
    拷贝赋值函数会导致内存池的状态不一致，因为它会改变内存池中对象的数量和位置。
    拷贝赋值函数会增加内存池的开销和风险，因为它需要分配和释放内存，可能抛出异常或导致内存泄漏。
    拷贝赋值函数会违反内存池的设计目的，因为内存池的目的是为了提高内存分配的效率和控制，而不是为了复制或转移对象。
    因此，C++ 内存池设计中通常将拷贝赋值函数弃置或删除，以避免上述问题，同时也可以遵循“禁止拷贝”的设计原则。
    */
    MemoryPool& operator=(const MemoryPool& memoryPool) = delete;
    /*
    移动赋值函数
    */
    MemoryPool& operator=(MemoryPool&& MemoryPool) noexcept;

    // 元素取地址
    pointer address(reference x) const noexcept; // 返回常量引用指针地址
    constPointer address(constReference x) const noexcept; // 返回非常量指针地址

    // Can only allocate one object at a time. n and hint are ignored
    //一次只能为一个目标分配空间 常量指针（const T*  -> 不可以修改该地址存放的数据）
    pointer allocate(size_type n = 1, constPointer hint = nullptr);
    void deallocate(pointer p, size_type n = 1); //回收内存 归还给Block内存块

    size_type maxSize() const noexcept; // 计算可使用的最大slot槽数

    template <class U, class... Args> void construct(U* p, Args&&... args);
    template <class U> void destroy(U* p); // 调用析构函数释放 p 所指的空间

    template <class... Args> pointer newElement(Args&&... args);
    void deleteElement(pointer p);

private:
    union Slot_ {
        value_type  element; // 使用时为 value_type 类型
        Slot_* next; // 需要回收时为Slot_* 类型并加入空闲链表中
    };

    /*
    使用char类型申请内存的原因可能有以下几点：
    1、char类型是最基本的数据类型，占用一个字节，可以表示任意的内存地址。
    2、char类型可以方便地进行指针运算，实现内存的切割和合并。
    3、char类型可以避免类型转换的开销，因为任何类型的指针都可以强制转换为char*类型。
    4、char类型可以避免内存对齐的问题，因为char类型不需要对齐。
    */
    typedef char* data_pointer_; // 字符类型指针
    typedef Slot_   slot_type_; // 槽类型
    typedef Slot_* slot_pointer_; // 槽类型指针

    slot_pointer_   currentBlock_; // 指向第一块 Block 内存块, 内存块链表的头指针
    slot_pointer_   currentSlot_; // 指向第一块可用的Slot槽，元素链表头指针
    slot_pointer_   lastSlot_; // 指向该Block中最后一块可用的Slot槽，可存放元素的最后指针
    slot_pointer_   freeSlot_; // 被释放元素的存放的空闲链表, 为链表头指针

    // 计算对齐所需空间
    size_type padPointer(data_pointer_ p, size_type align) const noexcept;
    void allocateBlock(); // 申请内存块放进内存池

    //  静态断言：当申请的BlockSize小于两个slot槽的内存时报错
    static_assert(BlockSize >= 2 * sizeof(size_type), "BlockSize too small.");
};

#include "./MemoryPool.tcc"

#endif // MEMORY_POOL_H
