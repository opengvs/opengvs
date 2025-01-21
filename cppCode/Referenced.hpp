/*    FileName: References.hpp  ���ļ��Ǹ���OSG�е�Referenced���������µģ�ȥ����ԭ�ļ���������ж�OpenThread��������
*     1���ļ���ҪC++17��֧�֣���ʹ�����ܰ�ȫָ��osg::ref_ptr(ref_ptr.hpp)ʱ����osg::Refenced�������Ϊ���࣬��֤������ָ���̰߳�ȫ
*     2�����ļ�����ʹ�� _OPENTHREADS_ATOMIC_USE_GCC_BUILTINS��
                     _OPENTHREADS_ATOMIC_USE_WIN32_INTERLOCKED��
                     _OPENTHREADS_ATOMIC_USE_BSD_ATOMIC ����ԭ�����Ͷ���
      3��������Ҫԭ���߳�ͬ��ʱ�����Զ���_OPENTHREADS_ATOMIC_USE_MUTEX �꣬�Ӷ�ϵͳ���Զ�ʹ��std::mutex��������߳�ͬ��
*     4�����⣺ʹ��OpenThread::Mutexʱ��ʹ�þ�̬������ʱ����Ҫ���� pthread_win32_attach��pthread_win32_detach����������Mutex����������ִ�С�
*           �����Ǿ�̬���ӿ�ʹ��ʱ�������˳��ᷢ���쳣��Ŀǰʹ�ö�̬DLL�ļ���û�����⡣
            ���飺һ������£�ʹ��std:mutex�����ź������⣬�����Ҫ��OSG�е�������ʱ��ʹ��__USER_OPENTHREAD__ǿ��ʹ��OpenThread�Ķ�̬������
         ��������Ҫʹ�þ�̬���ӿ⣬2.9.0�Ժ���Բ�����attach��_detach������Ϊ��֤�ܹ���ȷ�ͷ���Դ�������jsoncpp.lib��
                   ������pThreadMutex.cpp�ļ������#include<pThread.h>ͷ�ļ������ܱ�֤��ȷ�ͷ�Mutex
                   ���ϲ�����������ԣ����������ǣ���ʹ��_OPENTHREADS_ATOMIC_USE_MUTEXʱ�����ڲ�ʹ��ȫ�ֵ�ԭ�Ӳ�����ϵͳ�ᶨ����Mutex,
                   ���ͷŶ��Mutexʱ��������쳣
          ���������ж�ϵͳ�Ƿ���_OPENTHREADS_ATOMIC_USE_MUTEX�꣬���ʹ�øú궨�壬ϵͳ��Ҫ����ȫ�־�̬static InitGlobalMutexes s_initGlobalMutexes;
                     ������������_OSG_REFERENCED_USE_ATOMIC_OPERATIONSԭ�Ӳ���ʱ��ϵͳʹ��ȫ��Ψһ��mutexzuo��Ϊԭ��



               ��Ȩ��������    ������   2016��9��  �ӱ��ȷ�
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

        //2024��11��������ֵ��ֵ���� 
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

        //���û�ж������ԭ�Ӳ���ʵ�ֵ������������廥����
#ifndef  _ATOMIC_SELF_ROTATE_TYPE_LOCK
        mutable   std::mutex* _refMutex;
#define  _refMUTEX_LOCK_BEGIN      if(_refMutex!=nullptr) (*_refMutex).lock();
#define  _refMUTEX_LOCK_END        if(_refMutex!=nullptr) (*_refMutex).unlock(); 

#else
            std::atomic<unsigned long> _refAtomicLock = ATOMIC_FLAG_INIT;       //��ʼ��ԭ�Ӳ�������
#define  refAtomicLockLOCK_BEGIN    while (_refAtomicLock.test_and_set(std::memory_order_acquire)) ;
#define  refAtomicLockLOCK_END    _refAtomicLock.clear(std::memory_order_release);

#endif

#ifndef _USE_ATOMIC_REF_COUNT_
        mutable unsigned long                     _refCount=0;
#else
            std::atomic<unsigned long>                 _refCount=0; // ����һ��ԭ������������
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

#ifndef  _ATOMIC_SELF_ROTATE_TYPE_LOCK   //���û�ж������ԭ�Ӳ���ʵ�ֵ������������廥����
        mutable std::mutex              _ObserverMutex;
#define  _refMUTEX_LOCK_BEGIN      if(_refMutex!=nullptr) (_ObserverMutex).lock();
#define  _refMUTEX_LOCK_END        if(_refMutex!=nullptr) (_ObserverMutex).unlock(); 
#else
        mutable    std::atomic_flag _refAtomicLock = ATOMIC_FLAG_INIT;       //��ʼ��ԭ�Ӳ�������
#define  refAtomicLockLOCK_BEGIN    while (_refAtomicLock.test_and_set(std::memory_order_acquire)) ;
#define  refAtomicLockLOCK_END    _refAtomicLock.clear(std::memory_order_release);
#endif
#else
        //ʹ��ԭ�Ӳ���,,����Ҫ����_ObserverMutex������
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

        {  //������Ų���ɾ��
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