#ifndef AllocatorH
#define AllocatorH

#include <memory>

namespace Moya
{

    template <class T, std::size_t growSize = 1024>
    class MemoryPool
    {
        struct Block
        {
            Block* next;
        };

        class Buffer
        {
            static const std::size_t blockSize = sizeof(T) > sizeof(Block) ? sizeof(T) : sizeof(Block);
            uint8_t data[blockSize * growSize];

        public:

            Buffer* const next;

            Buffer(Buffer* next) :
                next(next)
            {
            }

            T* getBlock(std::size_t index)
            {
                return reinterpret_cast<T*>(&data[blockSize * index]);
            }
        };

        Block* firstFreeBlock = nullptr;
        Buffer* firstBuffer = nullptr;
        std::size_t bufferedBlocks = growSize;


    public:

        MemoryPool() = default;
        MemoryPool(MemoryPool && memoryPool) = delete;
        MemoryPool(const MemoryPool & memoryPool) = delete;
        MemoryPool operator =(MemoryPool && memoryPool) = delete;
        MemoryPool operator =(const MemoryPool & memoryPool) = delete;

        ~MemoryPool()
        {
            while(firstBuffer)
            {
                Buffer* buffer = firstBuffer;
                firstBuffer = buffer->next;
                delete buffer;
            }
        }

        T* allocate()
        {
            if(firstFreeBlock)
            {
                Block* block = firstFreeBlock;
                firstFreeBlock = block->next;
                return reinterpret_cast<T*>(block);
            }

            if(bufferedBlocks >= growSize)
            {
                firstBuffer = new Buffer(firstBuffer);
                bufferedBlocks = 0;
            }

            return firstBuffer->getBlock(bufferedBlocks++);
        }

        void deallocate(T* pointer)
        {
            Block* block = reinterpret_cast<Block*>(pointer);
            block->next = firstFreeBlock;
            firstFreeBlock = block;
        }
    };

    template <class T, std::size_t growSize = 1024>
    class Allocator : private MemoryPool<T, growSize>
    {
#ifdef _WIN32
        Allocator* copyAllocator;
        std::allocator<T>* rebindAllocator = nullptr;
#endif

    public:

        typedef std::size_t size_type;
        typedef std::ptrdiff_t difference_type;
        typedef T* pointer;
        typedef const T* const_pointer;
        typedef T & reference;
        typedef const T & const_reference;
        typedef T value_type;

        template <class U>
        struct rebind
        {
            typedef Allocator<U, growSize> other;
        };

#ifdef _WIN32
        Allocator() = default;

        Allocator(Allocator & allocator) :
            copyAllocator(&allocator)
        {
        }

        template <class U>
        Allocator(const Allocator<U, growSize> & other)
        {
            if(!std::is_same<T, U>::value)
                rebindAllocator = new std::allocator<T>();
        }

        ~Allocator()
        {
            delete rebindAllocator;
        }
#endif

        pointer allocate(size_type n, const void* hint = 0)
        {
#ifdef _WIN32
            if(copyAllocator)
                return copyAllocator->allocate(n, hint);

            if(rebindAllocator)
                return rebindAllocator->allocate(n, hint);
#endif

            if(n != 1 || hint)
                throw std::bad_alloc();

            return MemoryPool<T, growSize>::allocate();
        }

        void deallocate(pointer p, size_type n)
        {
#ifdef _WIN32
            if(copyAllocator)
            {
                copyAllocator->deallocate(p, n);
                return;
            }

            if(rebindAllocator)
            {
                rebindAllocator->deallocate(p, n);
                return;
            }
#endif

            MemoryPool<T, growSize>::deallocate(p);
        }

        void construct(pointer p, const_reference val)
        {
            new(p) T(val);
        }

        void destroy(pointer p)
        {
            p->~T();
        }
    };

}

#endif
