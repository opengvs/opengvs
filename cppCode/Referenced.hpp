/*    FileName: References.hpp  本文件是根据OSG中的Referenced基类改造的新的，去除了原文件编译过程中对OpenThread的依赖。
*     1、文件需要C++17的支持，在使用智能安全指针osg::ref_ptr(ref_ptr.hpp)时，该osg::Refenced类可以作为基类，保证了智能指针线程安全
*     2、本文件可以使用 _OPENTHREADS_ATOMIC_USE_GCC_BUILTINS、
                     _OPENTHREADS_ATOMIC_USE_WIN32_INTERLOCKED、
                     _OPENTHREADS_ATOMIC_USE_BSD_ATOMIC 三中原子类型定义
      3、当不需要原子线程同步时，可以定义_OPENTHREADS_ATOMIC_USE_MUTEX 宏，从而系统会自动使用std::mutex互斥进行线程同步
*     4、问题：使用OpenThread::Mutex时，使用静态库链接时，需要调用 pthread_win32_attach、pthread_win32_detach函数，否则，Mutex析构函数不执行。
*           问题是静态连接库使用时，程序退出会发生异常，目前使用动态DLL文件，没有问题。
            建议：一般情况下，使用std:mutex进行信号量互斥，如果需要与OSG中的类链接时，使用__USER_OPENTHREAD__强制使用OpenThread的动态库链接
         问题解决：要使用静态链接库，2.9.0以后可以不调用attach、_detach函数，为保证能够正确释放资源，需包含jsoncpp.lib库
                   或者在pThreadMutex.cpp文件中添加#include<pThread.h>头文件，才能保证正确释放Mutex
                   以上测试问题均不对，现在问题是：当使用_OPENTHREADS_ATOMIC_USE_MUTEX时，由于不使用全局的原子操作，系统会定义多个Mutex,
                   在释放多个Mutex时，会出现异常
          问题解决：判断系统是否定义_OPENTHREADS_ATOMIC_USE_MUTEX宏，如果使用该宏定义，系统不要定义全局静态static InitGlobalMutexes s_initGlobalMutexes;
                     变量，但是用_OSG_REFERENCED_USE_ATOMIC_OPERATIONS原子操作时，系统使用全局唯一的mutexzuo作为原子



               版权所有所有    刘文庆   2016年9月  河北廊坊
*/

#ifndef OSG_REFERENCED
#define OSG_REFERENCED 1


#include <mutex>
#include <atomic>
#include <set>

namespace osg {

    // forward declare, declared after Referenced below.
    //class DeleteHandler;
    class Observer;
    class ObserverSet;

    /** template class to help enforce static initialization order. */
    template <typename T, T M()>
    struct depends_on
    {
        depends_on() { M(); }
    };

    /** Base class for providing reference counted objects.*/
    class  Referenced
    {
    public:
        Referenced() :
            _refMutex(0),
            _refCount(0),
            _observerSet(0)
        {
            _refMutex = new std::mutex;


        };
        //  Referenced(const Referenced&) = delete;
        // inline Referenced& operator=(const Referenced&) = delete;

        inline Referenced(const Referenced& _Ref)
        {
            _refMutex = _Ref._refMutex;
            _refCount = _Ref._refCount;
            _observerSet = _Ref._observerSet;

        }

        //2024年11月增加右值赋值函数 
        Referenced(const Referenced&& _Ref)noexcept
        {
            _refMutex = std::move_if_noexcept(_Ref._refMutex);
            _refCount = _Ref._refCount;
            _observerSet = std::move_if_noexcept(_Ref._observerSet);
            _Ref._refMutex = nullptr;
            _Ref._refCount = 0;
            _Ref._observerSet = nullptr;
        }
        Referenced& operator = (Referenced&& _Ref)noexcept
        {
            if ((&_Ref) == this)
            {
                return *this;
            }

            if (this->_refCount > 0)
                this->unref();
            _refMutex = std::move_if_noexcept(_Ref._refMutex);
            _refCount = _Ref._refCount;
            _observerSet = std::move_if_noexcept(_Ref._observerSet);
            _Ref._refMutex = nullptr;
            _Ref._refCount = 0;
            _Ref._observerSet = nullptr;
            return *this;
        }

        inline Referenced& operator = (const Referenced& _Ref)
        {
            if (this == &_Ref)
                return *this;
            if (this->_refCount > 0)
                this->unref();

            _refMutex = _Ref._refMutex;
            _refCount = _Ref._refCount;
            _observerSet = _Ref._observerSet;
            return *this;
        }

        /** Deprecated, Referenced is always theadsafe so there method now has no effect and does not need to be called.*/
        virtual void setThreadSafeRefUnref(bool /*threadSafe*/) {}

        /** Get whether a mutex is used to ensure ref() and unref() are thread safe.*/

        bool getThreadSafeRefUnref() const { return _refMutex != 0; }


        /** Get the mutex used to ensure thread safety of ref()/unref(). */

        std::mutex* getRefMutex() const { return _refMutex; }

        /** Increment the reference count by one, indicating that
            this object has another pointer which is referencing it.*/
        inline int ref() const;

        /** Decrement the reference count by one, indicating that
            a pointer to this object is no longer referencing it.  If the
            reference count goes to zero, it is assumed that this object
            is no longer referenced and is automatically deleted.*/
        inline int unref() const;

        /** Decrement the reference count by one, indicating that
            a pointer to this object is no longer referencing it.  However, do
            not delete it, even if ref count goes to 0.  Warning, unref_nodelete()
            should only be called if the user knows exactly who will
            be responsible for, one should prefer unref() over unref_nodelete()
            as the latter can lead to memory leaks.*/
        int unref_nodelete() const;

        /** Return the number of pointers currently referencing this object. */
        inline int referenceCount() const { return _refCount; }


        /** Get the ObserverSet if one is attached, otherwise return NULL.*/

        ObserverSet* getObserverSet() const
        {

            return static_cast<ObserverSet*>(_observerSet);

        }

        /** Get the ObserverSet if one is attached, otherwise create an ObserverSet, attach it, then return this newly created ObserverSet.*/
        ObserverSet* getOrCreateObserverSet() const;

        /** Add a Observer that is observing this object, notify the Observer when this object gets deleted.*/
        void addObserver(Observer* observer) const;

        /** Remove Observer that is observing this object.*/
        void removeObserver(Observer* observer) const;

    public:

    protected:

        virtual ~Referenced();

        void signalObserversAndDelete(bool signalDelete, bool doDelete) const;

        //如果没有定义采用原子操作实现的自旋锁，定义互斥锁
#ifndef  _ATOMIC_SELF_ROTATE_TYPE_LOCK
        mutable   std::mutex* _refMutex;
#define  _refMUTEX_LOCK_BEGIN      if(_refMutex!=nullptr) (*_refMutex).lock();
#define  _refMUTEX_LOCK_END        if(_refMutex!=nullptr) (*_refMutex).unlock(); 

#else
            std::atomic<unsigned long> _refAtomicLock = ATOMIC_FLAG_INIT;       //初始化原子布尔类型
#define  refAtomicLockLOCK_BEGIN    while (_refAtomicLock.test_and_set(std::memory_order_acquire)) ;
#define  refAtomicLockLOCK_END    _refAtomicLock.clear(std::memory_order_release);

#endif

#ifndef _USE_ATOMIC_REF_COUNT_
        mutable unsigned long                     _refCount=0;
#else
            std::atomic<unsigned long>                 _refCount=0; // 定义一个原子整数计数器
#endif


        mutable void* _observerSet;

    };
    /** Observer base class for tracking when objects are unreferenced (their reference count goes to 0) and are being deleted.*/
    class  Observer
    {
    public:
        Observer() {};
        virtual ~Observer() {};

        /** objectDeleted is called when the observed object is about to be deleted.  The observer will be automatically
        * removed from the observed object's observer set so there is no need for the objectDeleted implementation
        * to call removeObserver() on the observed object. */
        virtual void objectDeleted(void*) {}

    };

    /** Class used by osg::Referenced to track the observers associated with it.*/
    /** Class used by osg::Referenced to track the observers associated with it.*/

    class  ObserverSet : public osg::Referenced
    {
    public:
        ObserverSet() = delete;
        // ObserverSet() :osg::Referenced() {};
        ObserverSet(const Referenced* observedObject) :_observedObject(const_cast<Referenced*>(observedObject)) {};

        Referenced* getObserverdObject() { return _observedObject; }
        const Referenced* getObserverdObject() const { return _observedObject; }

        /** "Lock" a Referenced object i.e., protect it from being deleted
          *  by incrementing its reference count.
          *
          * returns null if object doesn't exist anymore. */
        Referenced* addRefLock();

        inline std::mutex* getObserverSetMutex() const { return &_ObserverMutex; }

        void addObserver(Observer* observer);
        void removeObserver(Observer* observer);

        void signalObjectDeleted(void* ptr);

        // typedef std::set<Observer*> Observers;

        std::set<Observer*>& getObservers() { return _observers; }
        const std::set<Observer*>& getObservers() const { return _observers; }

    protected:
#pragma warning(disable : 26495)    
        ObserverSet(const ObserverSet& rhs) : osg::Referenced(rhs) {}
#pragma warning(suppress : 26495)    
        ObserverSet& operator = (const ObserverSet& /*rhs*/) { return *this; }
        //public:
        virtual ~ObserverSet()
        {
        }

#ifndef _USE_ATOMIC_REF_COUNT_

#ifndef  _ATOMIC_SELF_ROTATE_TYPE_LOCK   //如果没有定义采用原子操作实现的自旋锁，定义互斥锁
        mutable std::mutex              _ObserverMutex;
#define  _refMUTEX_LOCK_BEGIN      if(_refMutex!=nullptr) (_ObserverMutex).lock();
#define  _refMUTEX_LOCK_END        if(_refMutex!=nullptr) (_ObserverMutex).unlock(); 
#else
        mutable    std::atomic_flag _refAtomicLock = ATOMIC_FLAG_INIT;       //初始化原子布尔类型
#define  refAtomicLockLOCK_BEGIN    while (_refAtomicLock.test_and_set(std::memory_order_acquire)) ;
#define  refAtomicLockLOCK_END    _refAtomicLock.clear(std::memory_order_release);
#endif
#else
        //使用原子操作,,不需要定义_ObserverMutex互斥量
#endif

        Referenced* _observedObject;
        std::set<Observer*>                       _observers;

    };



    inline void ObserverSet::addObserver(Observer* observer)
    {
#ifndef _USE_ATOMIC_REF_COUNT_
#ifndef _ATOMIC_SELF_ROTATE_TYPE_LOCK
        std::scoped_lock lock(_ObserverMutex);
#else
        refAtomicLockLOCK_BEGIN
#endif
#endif
            _observers.insert(observer);

#ifndef _USE_ATOMIC_REF_COUNT_
#ifndef _ATOMIC_SELF_ROTATE_TYPE_LOCK

#else
        refAtomicLockLOCK_END
#endif
#endif
    }

    inline void ObserverSet::removeObserver(Observer* observer)
    {
#ifndef _USE_ATOMIC_REF_COUNT_
#ifndef _ATOMIC_SELF_ROTATE_TYPE_LOCK
        std::scoped_lock lock(_ObserverMutex);
#else
        refAtomicLockLOCK_BEGIN
#endif
#endif

            _observers.erase(observer);

#ifndef _USE_ATOMIC_REF_COUNT_
#ifndef _ATOMIC_SELF_ROTATE_TYPE_LOCK

#else
        refAtomicLockLOCK_END
#endif
#endif
    }

    inline Referenced* ObserverSet::addRefLock()
    {
        if (_observedObject == nullptr)
            return 0;

#ifndef _USE_ATOMIC_REF_COUNT_
#ifndef _ATOMIC_SELF_ROTATE_TYPE_LOCK
        std::scoped_lock lock(_ObserverMutex);
#else
        refAtomicLockLOCK_BEGIN
#endif
#endif

            int refCount = _observedObject->ref();
        if (refCount == 1)
        {
            // The object is in the process of being deleted, but our
            // objectDeleted() method hasn't been run yet (and we're
            // blocking it -- and the final destruction -- with our lock).
            _observedObject->unref_nodelete();

#ifndef _USE_ATOMIC_REF_COUNT_
#ifndef _ATOMIC_SELF_ROTATE_TYPE_LOCK

#else
            refAtomicLockLOCK_END
#endif
#endif
                return 0;
        }

#ifndef _USE_ATOMIC_REF_COUNT_
#ifndef _ATOMIC_SELF_ROTATE_TYPE_LOCK

#else
        refAtomicLockLOCK_END
#endif
#endif

            return _observedObject;
    }

    inline void ObserverSet::signalObjectDeleted(void* ptr)
    {
#ifndef _USE_ATOMIC_REF_COUNT_

#ifndef _ATOMIC_SELF_ROTATE_TYPE_LOCK
        std::scoped_lock lock(_ObserverMutex);
#else
        refAtomicLockLOCK_BEGIN
#endif

#endif


            for (std::set<Observer*>::iterator itr = _observers.begin();
                itr != _observers.end();
                ++itr)
        {
            (*itr)->objectDeleted(ptr);
        }
        _observers.clear();

        // reset the observed object so that we know that it's now detached.
        _observedObject = 0;

#ifndef _USE_ATOMIC_REF_COUNT_
#ifndef _ATOMIC_SELF_ROTATE_TYPE_LOCK

#else
        refAtomicLockLOCK_END
#endif
#endif
    }

    inline int Referenced::ref() const
    {
#ifndef _USE_ATOMIC_REF_COUNT_

#ifndef _ATOMIC_SELF_ROTATE_TYPE_LOCK
        std::scoped_lock lock(*_refMutex);
#else
        refAtomicLockLOCK_BEGIN
#endif

#endif
            ++_refCount;

#ifndef _USE_ATOMIC_REF_COUNT_
#ifndef _ATOMIC_SELF_ROTATE_TYPE_LOCK

#else
        refAtomicLockLOCK_END
#endif
#endif

            return _refCount;
    }

    inline int Referenced::unref() const
    {
        int newRef;

        bool needDelete = false;

        {  //这个括号不能删除
#ifndef _USE_ATOMIC_REF_COUNT_

#ifndef _ATOMIC_SELF_ROTATE_TYPE_LOCK
            std::scoped_lock lock(*_refMutex);
#else
            refAtomicLockLOCK_BEGIN
#endif

#endif


                --_refCount;
            newRef = _refCount;
            needDelete = newRef == 0;
#ifndef _USE_ATOMIC_REF_COUNT_
#ifndef _ATOMIC_SELF_ROTATE_TYPE_LOCK

#else
            refAtomicLockLOCK_END
#endif
#endif
        }
        if (needDelete)
        {
            signalObserversAndDelete(true, true);
        }

        return newRef;
    }

    inline Referenced::~Referenced()
    {
        if (_refCount > 0)
        {
            // OSG_WARN<<"Warning: deleting still referenced object "<<this<<" of type '"<<typeid(this).name()<<"'"<<std::endl;
             //OSG_WARN<<"         the final reference count was "<<_refCount<<", memory corruption possible."<<std::endl;
        }

        // signal observers that we are being deleted.
        signalObserversAndDelete(true, false);

        if (_observerSet)
            static_cast<ObserverSet*>(_observerSet)->unref();


        if (_refMutex)
        {
            delete _refMutex;
            _refMutex = nullptr;
        }

    }

    inline ObserverSet* Referenced::getOrCreateObserverSet() const
    {
#ifndef _USE_ATOMIC_REF_COUNT_

#ifndef _ATOMIC_SELF_ROTATE_TYPE_LOCK
        std::scoped_lock lock(*_refMutex);
#else
        refAtomicLockLOCK_BEGIN
#endif

#endif
            if (!_observerSet)
            {
                _observerSet = new ObserverSet(this);
                static_cast<ObserverSet*>(_observerSet)->ref();
            }

#ifndef _USE_ATOMIC_REF_COUNT_
#ifndef _ATOMIC_SELF_ROTATE_TYPE_LOCK

#else
        refAtomicLockLOCK_END
#endif
#endif

            return static_cast<ObserverSet*>(_observerSet);
    }

    inline void Referenced::addObserver(Observer* observer) const
    {
        getOrCreateObserverSet()->addObserver(observer);
    }

    inline void Referenced::removeObserver(Observer* observer) const
    {
        getOrCreateObserverSet()->removeObserver(observer);
    }

    inline void Referenced::signalObserversAndDelete(bool signalDelete, bool doDelete) const
    {

        ObserverSet* observerSet = static_cast<ObserverSet*>(_observerSet);


        if (observerSet && signalDelete)
        {
            observerSet->signalObjectDeleted(const_cast<Referenced*>(this));
        }

        if (doDelete)
        {
            if (_refCount == 0)
                delete this;
        }
    }

    inline int Referenced::unref_nodelete() const
    {
#ifndef _USE_ATOMIC_REF_COUNT_

#ifndef _ATOMIC_SELF_ROTATE_TYPE_LOCK
        std::scoped_lock lock(*_refMutex);
#else
        refAtomicLockLOCK_BEGIN
#endif

#endif
            --_refCount;

#ifndef _USE_ATOMIC_REF_COUNT_
#ifndef _ATOMIC_SELF_ROTATE_TYPE_LOCK

#else
        refAtomicLockLOCK_END
#endif
#endif

            return  _refCount;



    }
    // intrusive_ptr_add_ref and intrusive_ptr_release allow
    // use of osg Referenced classes with boost::intrusive_ptr
    inline void intrusive_ptr_add_ref(Referenced* p) { p->ref(); }
    inline void intrusive_ptr_release(Referenced* p) { p->unref(); }


}

#endif