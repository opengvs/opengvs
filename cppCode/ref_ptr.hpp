/*   FileName:ref_ptr.hpp   多线程智能安全指针的定义头文件   本文件从osg::ref_ptr改造过来，去除了对OpenThread的依赖
*      这个多线程条件下满足安全性的智能指针，最初时从OSG下抠出来的，用到了多个工程项目中，
                    使用了std::scoped_lock lock(_ObserverMutex);函数，需要c++17的支撑
      2024年12月闲着没事，突然看到多年前的代码，想到使用原子操作实现，于是改造了Refencenced.hpp文件
      1、本文件需要C++11的支持
      2、本指针为模板类，模板T类需要实现T->ref()/T->unref()/T->unref_nodelete()三个函数，建议用户从osg::Referenced类派生
      版权所有：刘文庆     2024年12月  于西双版纳家中
*/

#ifndef OSG_REF_PTR
#define OSG_REF_PTR  1

#include <utility>
#include "Referenced.hpp"
#include <thread>

namespace osg {

    template<typename T> class observer_ptr;

    /** Smart pointer for handling referenced counted objects.*/
    template<class T>
    class ref_ptr
    {
    public:
        typedef T element_type;

        ref_ptr() : _ptr(0) {}
        ref_ptr(T* ptr) : _ptr(ptr) { if (_ptr) _ptr->ref(); }
        ref_ptr(const ref_ptr& rp) : _ptr(rp._ptr) { if (_ptr) _ptr->ref(); }
       
        template<class Other> ref_ptr(const ref_ptr<Other>& rp)// : _ptr(rp._ptr) { if (_ptr) _ptr->ref(); }
        {
            _ptr = dynamic_cast<T*>(rp._ptr);//dynamic_cast<Derived*>(basePtr);

                 if (_ptr) _ptr->ref();
        }

        ref_ptr(observer_ptr<T>& optr) : _ptr(0) { optr.lock(*this); }

        //2024年11月增加右值构造函数，解决函数返回智能指针问题--以前智能指针返回时，需要在函数内部使用return ref_ptr->get()
        ref_ptr( ref_ptr&& rp) noexcept
        {
            _ptr = std::move_if_noexcept(rp._ptr);  // _ptr = rp._ptr;//使用这句就会引起异常
            
           
            rp._ptr = nullptr;
        }

        ~ref_ptr() { if (_ptr) _ptr->unref();  _ptr = 0; }

        ref_ptr& operator=(const ref_ptr& rp)noexcept
        {
            assign(rp);
            return *this;
        }
        //2024年11月增加右值赋值函数 解决函数返回智能指针问题
        ref_ptr& operator=( ref_ptr&& rp) noexcept
        {
            if (_ptr == rp._ptr) 
               return *this;
            if (this->_ptr != NULL)
            {
                this->release();
           }
            _ptr = std::move_if_noexcept(rp._ptr);//_ptr = rp._ptr;
            
            rp._ptr = nullptr;

            return *this;
        }
        template<class Other> ref_ptr& operator = (const ref_ptr<Other>& rp)
        {
            assign(rp);
            return *this;
        }

        inline ref_ptr& operator = (T* ptr)
        {
            if (_ptr == ptr) return *this;
            T* tmp_ptr = _ptr;
            _ptr = ptr;
            if (_ptr) _ptr->ref();
            // unref second to prevent any deletion of any object which might
            // be referenced by the other object. i.e rp is child of the
            // original _ptr.
            if (tmp_ptr) tmp_ptr->unref();
            return *this;
        }
      //  operator T* () const { return _ptr; }   //使用智能指针时不建议使用原始类型的指针处理问题
#ifdef OSG_USE_REF_PTR_IMPLICIT_OUTPUT_CONVERSION
        // implicit output conversion
//        operator T* () const { return _ptr; }  
#else
        // comparison operators for ref_ptr.
        bool operator == (const ref_ptr& rp) const { return (_ptr == rp._ptr); }
        bool operator == (const T* ptr) const { return (_ptr == ptr); }
        friend bool operator == (const T* ptr, const ref_ptr& rp) { return (ptr == rp._ptr); }

        bool operator != (const ref_ptr& rp) const { return (_ptr != rp._ptr); }
        bool operator != (const T* ptr) const { return (_ptr != ptr); }
        friend bool operator != (const T* ptr, const ref_ptr& rp) { return (ptr != rp._ptr); }

        bool operator < (const ref_ptr& rp) const { return (_ptr < rp._ptr); }
    private:
        typedef T* ref_ptr::* unspecified_bool_type;

    public:
        // safe bool conversion
        operator unspecified_bool_type() const { return valid() ? &ref_ptr::_ptr : 0; }
#endif

        T& operator*() const
        {
            return *_ptr;
        }
        T* operator->() const
        {
            return _ptr;
        }

        T* get() const { return _ptr; }
       // T* get() { return _ptr; }
        bool operator!() const { return _ptr == 0; } // not required
        bool valid() const { return _ptr != 0; }

        /** release the pointer from ownership by this ref_ptr<>, decrementing the objects refencedCount() via unref_nodelete() to prevent the Object
          * object from being deleted even if the reference count goes to zero.  Use when using a local ref_ptr<> to an Object that you want to return
          * from a function/method via a C pointer, whilst preventing the normal ref_ptr<> destructor from cleaning up the object. When using release()
          * you are implicitly expecting other code to take over management of the object, otherwise a memory leak will result. */
        T* release() 
        {
            T* tmp = _ptr; 
            if (_ptr)
                _ptr->unref_nodelete();
            _ptr = 0; 
            
            return tmp; 
        }

        void swap(ref_ptr& rp) 
        { 
            T* tmp = _ptr; 
            _ptr = rp._ptr; 
            rp._ptr = tmp;
        }

    private:

        template<class Other> void assign(const ref_ptr<Other>& rp)
        {
            if (_ptr == rp._ptr) return;
            T* tmp_ptr = _ptr;
            _ptr = dynamic_cast<T*>( rp._ptr);
            if (_ptr) _ptr->ref();
            // unref second to prevent any deletion of any object which might
            // be referenced by the other object. i.e rp is child of the
            // original _ptr.
            if (tmp_ptr) 
                tmp_ptr->unref();
            return;
        }

        template<class Other> friend class ref_ptr;

        T* _ptr;
    };


    template<class T> inline
        void swap(ref_ptr<T>& rp1, ref_ptr<T>& rp2) { rp1.swap(rp2); }

    template<class T> inline
        T* get_pointer(const ref_ptr<T>& rp) { return rp.get(); }

    template<class T, class Y> inline
        ref_ptr<T> static_pointer_cast(const ref_ptr<Y>& rp) { return static_cast<T*>(rp.get()); }

    template<class T, class Y> inline
        ref_ptr<T> dynamic_pointer_cast(const ref_ptr<Y>& rp) { return dynamic_cast<T*>(rp.get()); }

    template<class T, class Y> inline
        ref_ptr<T> const_pointer_cast(const ref_ptr<Y>& rp) { return const_cast<T*>(rp.get()); }


    /** Smart pointer for observed objects, that automatically set pointers to them to null when they are deleted.
      * To use the observer_ptr<> robustly in multi-threaded applications it is recommend to access the pointer via
      * the lock() method that passes back a ref_ptr<> that safely takes a reference to the object to prevent deletion
      * during usage of the object.  In certain conditions it may be safe to use the pointer directly without using lock(),
      * which will confer a performance advantage, the conditions are:
      *   1) The data structure is only accessed/deleted in single threaded/serial way.
      *   2) The data strucutre is guaranteed by high level management of data strucutures and threads which avoid
      *      possible situations where the observer_ptr<>'s object may be deleted by one thread whilst being accessed
      *      by another.
      * If you are in any doubt about whether it is safe to access the object safe then use the
      * ref_ptr<> observer_ptr<>.lock() combination. */
    template<class T>
    class observer_ptr
    {
    public:
        typedef T element_type;
        observer_ptr() : _reference(0), _ptr(0) {}

        /**
         * Create a observer_ptr from a ref_ptr.
         */
        observer_ptr(const ref_ptr<T>& rp)
        {
            _reference = rp.valid() ? rp->getOrCreateObserverSet() : 0;
            _ptr = (_reference.valid() && _reference->getObserverdObject() != 0) ? rp.get() : 0;
        }

        /**
         * Create a observer_ptr from a raw pointer. For compatibility;
         * the result might not be lockable.
         */
        observer_ptr(T* rp)
        {
            _reference = rp ? rp->getOrCreateObserverSet() : 0;
            _ptr = (_reference.valid() && _reference->getObserverdObject() != 0) ? rp : 0;
        }

        observer_ptr(const observer_ptr& wp) :
            _reference(wp._reference),
            _ptr(wp._ptr)
        {
        }
        //2024年11月增加右值赋值函数 
        observer_ptr(observer_ptr&& wp) noexcept
        {
            _ptr = std::move_if_noexcept(wp._ptr);
            _reference = std::move_if_noexcept(wp._reference);

            wp._ptr = nullptr;
           // if (wp._reference.valid())  //通过osg::ref_ptr类型的右值引用已经将wp._reference变量清空了
           //        wp._reference->unref();

        }

        ~observer_ptr()
        {

        }

        observer_ptr& operator = (const observer_ptr& wp)
        {
            if (&wp == this) 
                return *this;

            _reference = wp._reference;
            _ptr = wp._ptr;
            return *this;
        }
        //2024年11月增加右值赋值函数 
        observer_ptr& operator = (observer_ptr&& wp)
        {
            if (&wp == this) 
                return *this;

           // if (this->valid() == true)
           // {
           //     this->_reference->unref();
           //     this->_ptr = nullptr;
           // }
            this->_reference = std::move_if_noexcept(wp._reference);
            _ptr = std::move_if_noexcept(wp._ptr);
            wp._ptr = nullptr;
            return *this;
        }
        observer_ptr& operator = (const ref_ptr<T>& rp)
        {
            _reference = rp.valid() ? rp->getOrCreateObserverSet() : 0;
            _ptr = (_reference.valid() && _reference->getObserverdObject() != 0) ? rp.get() : 0;
            return *this;
        }

        observer_ptr& operator = (T* rp)
        {
            _reference = rp ? rp->getOrCreateObserverSet() : 0;
            _ptr = (_reference.valid() && _reference->getObserverdObject() != 0) ? rp : 0;
            return *this;
        }

        /**
         * Assign the observer_ptr to a ref_ptr. The ref_ptr will be valid if the
         * referenced object hasn't been deleted and has a ref count > 0.
         */
        bool lock(ref_ptr<T>& rptr) const
        {
            if (!_reference)
            {
                rptr = 0;
                return false;
            }

            Referenced* obj = _reference->addRefLock();
            if (!obj)
            {
                rptr = 0;
                return false;
            }

            rptr = _ptr;
            obj->unref_nodelete();
            return rptr.valid();
        }

        /** Comparison operators. These continue to work even after the
         * observed object has been deleted.
         */
        bool operator == (const observer_ptr& wp) const { return _reference == wp._reference; }
        bool operator != (const observer_ptr& wp) const { return _reference != wp._reference; }
        bool operator < (const observer_ptr& wp) const { return _reference < wp._reference; }
        bool operator > (const observer_ptr& wp) const { return wp._reference < _reference; }

        // Non-strict interface, for compatibility
        // comparison operator for const T*.
        inline bool operator == (const T* ptr) const { return _ptr == ptr; }
        inline bool operator != (const T* ptr) const { return _ptr != ptr; }
        inline bool operator < (const T* ptr) const { return _ptr < ptr; }
        inline bool operator > (const T* ptr) const { return _ptr > ptr; }

        // Convenience methods for operating on object, however, access is not automatically threadsafe.
        // To make thread safe, one should either ensure at a high level
        // that the object will not be deleted while operating on it, or
        // by using the observer_ptr<>::lock() to get a ref_ptr<> that
        // ensures the objects stay alive throughout all access to it.

        // Throw an error if _reference is null?
        inline T& operator*() const { return *_ptr; }
        inline T* operator->() const { return _ptr; }

        // get the raw C pointer
        inline T* get() const { return (_reference.valid() && _reference->getObserverdObject() != 0) ? _ptr : 0; }

        inline bool operator!() const { return get() == 0; }
        inline bool valid() const { return get() != 0; }

    protected:

        osg::ref_ptr<ObserverSet>   _reference;
        T* _ptr;
    };

}


#endif

/*
class  Person : public osg::Referenced
{
    int* p;
public:
    osg::ref_ptr<Person> next;
    //osg::ref_ptr<Person> Ptr;
    osg::observer_ptr<Person> Ptr;

    Person()
    {
        p = new int[10];
    }
    virtual ~Person()
    {
        delete[]p;
        static int i = 1;
        std::cout << "我被析构了" << i++ << std::endl;
    }
};
void main()
{
   osg::ref_ptr<Person>  person1(new Person());
    osg::ref_ptr<Person>  person2(new Person());

    person1->next = person2.get();
    person1->next->Ptr = person1;

    osg::ref_ptr<Person>  person3(new Teacher());

    osg::ref_ptr<Teacher>  teacher1 = person3;
}
*/
