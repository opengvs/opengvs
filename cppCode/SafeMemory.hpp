// memory standard header

// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef _SAFEMEMORY_
#define _SAFEMEMORY_
#include <memory>
#include <yvals_core.h>
#if _STL_COMPILER_PREPROCESSOR
#include <exception>
#include <iosfwd>
#include <type_traits>
#include <typeinfo>
#include <xmemory>
#include <mutex>

#if _HAS_CXX20
#include <atomic>
#endif // _HAS_CXX20

#pragma pack(push, _CRT_PACKING)
#pragma warning(push, _STL_WARNING_LEVEL)
#pragma warning(disable : _STL_DISABLED_WARNINGS)
_STL_DISABLE_CLANG_WARNINGS
#pragma push_macro("new")
#undef new

// TRANSITION, non-_Ugly attribute tokens
#pragma push_macro("msvc")
#undef msvc


_STD_BEGIN

class __declspec(novtable) _SafeRef_count_base { // common code for reference counting
private:
#ifdef _M_CEE_PURE
    // permanent workaround to avoid mentioning _purecall in msvcurt.lib, ptrustu.lib, or other support libs
    virtual void _Destroy() noexcept {
        _CSTD abort();
    }

    virtual void _Delete_this() noexcept {
        _CSTD abort();
    }
#else // ^^^ defined(_M_CEE_PURE) / !defined(_M_CEE_PURE) vvv
    virtual void _Destroy() noexcept = 0; // destroy managed resource
    virtual void _Delete_this() noexcept = 0; // destroy self
#endif // ^^^ !defined(_M_CEE_PURE) ^^^

    _Atomic_counter_t _Uses = 1;
    _Atomic_counter_t _Weaks = 1;
    mutex             _Mutex;

protected:
    constexpr _SafeRef_count_base() noexcept = default; // non-atomic initializations

public:
    _SafeRef_count_base(const _SafeRef_count_base&) = delete;
    _SafeRef_count_base& operator=(const _SafeRef_count_base&) = delete;

    virtual ~_SafeRef_count_base() noexcept {} // TRANSITION, should be non-virtual

    bool _Incref_nz() noexcept { // increment use count if not zero, return true if successful
        auto& _Volatile_uses = reinterpret_cast<volatile long&>(_Uses);
#ifdef _M_CEE_PURE
        long _Count = *_Atomic_address_as<const long>(&_Volatile_uses);
#else
        long _Count = __iso_volatile_load32(reinterpret_cast<volatile int*>(&_Volatile_uses));
#endif
        while (_Count != 0) {
            const long _Old_value = _INTRIN_RELAXED(_InterlockedCompareExchange)(&_Volatile_uses, _Count + 1, _Count);
            if (_Old_value == _Count) {
                return true;
            }

            _Count = _Old_value;
        }

        return false;
    }

    void _Incref() noexcept { // increment use count
        _MT_INCR(_Uses);
    }

    void _Incwref() noexcept { // increment weak reference count
        _MT_INCR(_Weaks);
    }

    void _Decref() noexcept { // decrement use count
        
        if (_MT_DECR(_Uses) == 0)
        {
            scoped_lock(_Mutex);
            _Destroy();
            //_Decwref();
            if (_MT_DECR(_Weaks) == 0) {
                _Delete_this();
            }
        }
    }

    void _Decwref() noexcept { // decrement weak reference count
        
        if (_MT_DECR(_Weaks) == 0) {
            scoped_lock(_Mutex);
            _Delete_this();
        }
    }

    long _Use_count() const noexcept {
        return static_cast<long>(_Uses);
    }

    virtual void* _Get_deleter(const type_info&) const noexcept {
        return nullptr;
    }
};
template <class _Ty>
class _SafeRef_count : public _SafeRef_count_base { // handle reference counting for pointer without deleter
public:
    explicit _SafeRef_count(_Ty* _Px) : _SafeRef_count_base(), _Ptr(_Px) {}

private:
    void _Destroy() noexcept override { // destroy managed resource
        delete _Ptr;
    }

    void _Delete_this() noexcept override { // destroy self
        delete this;
    }

    _Ty* _Ptr;
};
template <class _Resource, class _Dx>
class _SafeRef_count_resource : public _SafeRef_count_base { // handle reference counting for object with deleter
public:
    _SafeRef_count_resource(_Resource _Px, _Dx _Dt)
        : _Ref_count_base(), _Mypair(_One_then_variadic_args_t{}, _STD move(_Dt), _Px) {}

    ~_SafeRef_count_resource() noexcept override = default; // TRANSITION, should be non-virtual

    void* _Get_deleter(const type_info& _Typeid) const noexcept override {
#if _HAS_STATIC_RTTI
        if (_Typeid == typeid(_Dx)) {
            return const_cast<_Dx*>(_STD addressof(_Mypair._Get_first()));
        }
#else // ^^^ _HAS_STATIC_RTTI / !_HAS_STATIC_RTTI vvv
        (void)_Typeid;
#endif // ^^^ !_HAS_STATIC_RTTI ^^^

        return nullptr;
    }

private:
    void _Destroy() noexcept override { // destroy managed resource
        _Mypair._Get_first()(_Mypair._Myval2);
    }

    void _Delete_this() noexcept override { // destroy self
        delete this;
    }

    _Compressed_pair<_Dx, _Resource> _Mypair;
};
template <class _Resource, class _Dx, class _Alloc>
class _SafeRef_count_resource_alloc : public _SafeRef_count_base {
    // handle reference counting for object with deleter and allocator
public:
    _SafeRef_count_resource_alloc(_Resource _Px, _Dx _Dt, const _Alloc& _Ax)
        : _Ref_count_base(),
        _Mypair(_One_then_variadic_args_t{}, _STD move(_Dt), _One_then_variadic_args_t{}, _Ax, _Px) {}

    ~_SafeRef_count_resource_alloc() noexcept override = default; // TRANSITION, should be non-virtual

    void* _Get_deleter(const type_info& _Typeid) const noexcept override {
#if _HAS_STATIC_RTTI
        if (_Typeid == typeid(_Dx)) {
            return const_cast<_Dx*>(_STD addressof(_Mypair._Get_first()));
        }
#else // ^^^ _HAS_STATIC_RTTI / !_HAS_STATIC_RTTI vvv
        (void)_Typeid;
#endif // ^^^ !_HAS_STATIC_RTTI ^^^

        return nullptr;
    }

private:
    using _Myalty = _Rebind_alloc_t<_Alloc, _SafeRef_count_resource_alloc>;

    void _Destroy() noexcept override { // destroy managed resource
        _Mypair._Get_first()(_Mypair._Myval2._Myval2);
    }

    void _Delete_this() noexcept override { // destroy self
        _Myalty _Al = _Mypair._Myval2._Get_first();
        this->~_SafeRef_count_resource_alloc();
        _STD _Deallocate_plain(_Al, this);
    }

    _Compressed_pair<_Dx, _Compressed_pair<_Myalty, _Resource>> _Mypair;
};

_EXPORT_STD template <class _Ty>
class safeShared_ptr;

_EXPORT_STD template <class _Ty>
class safeWeak_ptr;

template <class _Ty>
class _SafePtr_base { // base class for shared_ptr and weak_ptr
public:
    using element_type = remove_extent_t<_Ty>;

    _NODISCARD long use_count() const noexcept {
        return _Rep ? _Rep->_Use_count() : 0;
    }

    template <class _Ty2>
    _NODISCARD bool owner_before(const _Ptr_base<_Ty2>& _Right) const noexcept { // compare addresses of manager objects
        return _Rep < _Right._Rep;
    }

    _SafePtr_base(const _SafePtr_base&) = delete;
    _SafePtr_base& operator=(const _SafePtr_base&) = delete;

protected:
    _NODISCARD element_type* get() const noexcept {
        return _Ptr;
    }

    constexpr _SafePtr_base() noexcept = default;

    ~_SafePtr_base() = default;

    template <class _Ty2>
    void _Move_construct_from(_SafePtr_base<_Ty2>&& _Right) noexcept {
        // implement shared_ptr's (converting) move ctor and weak_ptr's move ctor
        _Ptr = _Right._Ptr;
        _Rep = _Right._Rep;

        _Right._Ptr = nullptr;
        _Right._Rep = nullptr;
    }

    template <class _Ty2>
    void _Copy_construct_from(const safeShared_ptr<_Ty2>& _Other) noexcept {
        // implement shared_ptr's (converting) copy ctor
        _Other._Incref();

        _Ptr = _Other._Ptr;
        _Rep = _Other._Rep;
    }

    template <class _Ty2>
    void _Alias_construct_from(const safeShared_ptr<_Ty2>& _Other, element_type* _Px) noexcept {
        // implement shared_ptr's aliasing ctor
        _Other._Incref();

        _Ptr = _Px;
        _Rep = _Other._Rep;
    }

    template <class _Ty2>
    void _Alias_move_construct_from(safeShared_ptr<_Ty2>&& _Other, element_type* _Px) noexcept {
        // implement shared_ptr's aliasing move ctor
        _Ptr = _Px;
        _Rep = _Other._Rep;

        _Other._Ptr = nullptr;
        _Other._Rep = nullptr;
    }

    template <class _Ty0>
    friend class safeWeak_ptr; // specifically, safeWeak_ptr::lock()

    template <class _Ty2>
    bool _Construct_from_weak(const safeWeak_ptr<_Ty2>& _Other) noexcept {
        // implement shared_ptr's ctor from weak_ptr, and weak_ptr::lock()
        if (_Other._Rep && _Other._Rep->_Incref_nz()) {
            _Ptr = _Other._Ptr;
            _Rep = _Other._Rep;
            return true;
        }

        return false;
    }

    void _Incref() const noexcept {
        if (_Rep) {
            _Rep->_Incref();
        }
    }

    void _Decref() noexcept { // decrement reference count
        if (_Rep) {
            _Rep->_Decref();
        }
    }

    void _Swap(_SafePtr_base& _Right) noexcept { // swap pointers
        _STD swap(_Ptr, _Right._Ptr);
        _STD swap(_Rep, _Right._Rep);
    }

    template <class _Ty2>
    void _Weakly_construct_from(const _SafePtr_base<_Ty2>& _Other) noexcept { // implement weak_ptr's ctors
        if (_Other._Rep) {
            _Ptr = _Other._Ptr;
            _Rep = _Other._Rep;
            _Rep->_Incwref();
        }
        else {
            _STL_INTERNAL_CHECK(!_Ptr && !_Rep);
        }
    }

    template <class _Ty2>
    void _Weakly_convert_lvalue_avoiding_expired_conversions(const _SafePtr_base<_Ty2>& _Other) noexcept {
        // implement weak_ptr's copy converting ctor
        if (_Other._Rep) {
            _Rep = _Other._Rep; // always share ownership
            _Rep->_Incwref();

            if (_Rep->_Incref_nz()) {
                _Ptr = _Other._Ptr; // keep resource alive during conversion, handling virtual inheritance
                _Rep->_Decref();
            }
            else {
                _STL_INTERNAL_CHECK(!_Ptr);
            }
        }
        else {
            _STL_INTERNAL_CHECK(!_Ptr && !_Rep);
        }
    }

    template <class _Ty2>
    void _Weakly_convert_rvalue_avoiding_expired_conversions(_SafePtr_base<_Ty2>&& _Other) noexcept {
        // implement weak_ptr's move converting ctor
        _Rep = _Other._Rep; // always transfer ownership
        _Other._Rep = nullptr;

        if (_Rep && _Rep->_Incref_nz()) {
            _Ptr = _Other._Ptr; // keep resource alive during conversion, handling virtual inheritance
            _Rep->_Decref();
        }
        else {
            _STL_INTERNAL_CHECK(!_Ptr);
        }

        _Other._Ptr = nullptr;
    }

    void _Incwref() const noexcept {
        if (_Rep) {
            _Rep->_Incwref();
        }
    }

    void _Decwref() noexcept { // decrement weak reference count
        if (_Rep) {
            _Rep->_Decwref();
        }
    }

private:
    element_type* _Ptr{ nullptr };
    _SafeRef_count_base* _Rep{ nullptr };

    template <class _Ty0>
    friend class _SafePtr_base;

    friend safeShared_ptr<_Ty>;

    template <class _Ty0>
    friend struct atomic;

    friend _Exception_ptr_access;

#if _HAS_STATIC_RTTI
    template <class _Dx, class _Ty0>
    friend _Dx* get_deleter(const safeShared_ptr<_Ty0>& _Sx) noexcept;
#endif // _HAS_STATIC_RTTI
};

/*
#if _HAS_STATIC_RTTI
_EXPORT_STD template <class _Dx, class _Ty>
_NODISCARD _Dx* get_deleter(const safeShared_ptr<_Ty>& _Sx) noexcept {
    // return pointer to shared_ptr's deleter object if its type is _Dx
    if (_Sx._Rep) {
        return static_cast<_Dx*>(_Sx._Rep->_Get_deleter(typeid(_Dx)));
    }

    return nullptr;
}
#else // ^^^ _HAS_STATIC_RTTI / !_HAS_STATIC_RTTI vvv
_EXPORT_STD template <class _Dx, class _Ty>
_Dx* get_deleter(const safeShared_ptr<_Ty>&) noexcept = delete; // requires static RTTI
#endif // ^^^ !_HAS_STATIC_RTTI ^^^
*/

_EXPORT_STD template <class _Ty>
class safeShared_ptr : public _SafePtr_base<_Ty> { // class for reference counted resource management
private:
    using _Mybase = _SafePtr_base<_Ty>;

public:
    using typename _Mybase::element_type;

#if _HAS_CXX17
    using safeWeak_type = safeWeak_ptr<_Ty>;
#endif // _HAS_CXX17

    constexpr safeShared_ptr() noexcept = default;

    constexpr safeShared_ptr(nullptr_t) noexcept {} // construct empty shared_ptr

    template <class _Ux,
        enable_if_t<conjunction_v<conditional_t<is_array_v<_Ty>, _Can_array_delete<_Ux>, _Can_scalar_delete<_Ux>>,
        _SP_convertible<_Ux, _Ty>>,
        int> = 0>
        explicit safeShared_ptr(_Ux* _Px) { // construct shared_ptr object that owns _Px
        if constexpr (is_array_v<_Ty>) {
            _Setpd(_Px, default_delete<_Ux[]>{});
        }
        else {
            _Temporary_owner<_Ux> _Owner(_Px);
            _Set_ptr_rep_and_enable_shared(_Owner._Ptr, new _SafeRef_count<_Ux>(_Owner._Ptr));
            _Owner._Ptr = nullptr;
        }
    }

    template <class _Ux, class _Dx,
        enable_if_t<conjunction_v<is_move_constructible<_Dx>, _Can_call_function_object<_Dx&, _Ux*&>,
        _SP_convertible<_Ux, _Ty>>,
        int> = 0>
        safeShared_ptr(_Ux* _Px, _Dx _Dt) { // construct with _Px, deleter
        _Setpd(_Px, _STD move(_Dt));
    }

    template <class _Ux, class _Dx, class _Alloc,
        enable_if_t<conjunction_v<is_move_constructible<_Dx>, _Can_call_function_object<_Dx&, _Ux*&>,
        _SP_convertible<_Ux, _Ty>>,
        int> = 0>
        safeShared_ptr(_Ux* _Px, _Dx _Dt, _Alloc _Ax) { // construct with _Px, deleter, allocator
        _Setpda(_Px, _STD move(_Dt), _Ax);
    }

    template <class _Dx,
        enable_if_t<conjunction_v<is_move_constructible<_Dx>, _Can_call_function_object<_Dx&, nullptr_t&>>, int> = 0>
    safeShared_ptr(nullptr_t, _Dx _Dt) { // construct with nullptr, deleter
        _Setpd(nullptr, _STD move(_Dt));
    }

    template <class _Dx, class _Alloc,
        enable_if_t<conjunction_v<is_move_constructible<_Dx>, _Can_call_function_object<_Dx&, nullptr_t&>>, int> = 0>
    safeShared_ptr(nullptr_t, _Dx _Dt, _Alloc _Ax) { // construct with nullptr, deleter, allocator
        _Setpda(nullptr, _STD move(_Dt), _Ax);
    }

    template <class _Ty2>
    safeShared_ptr(const shared_ptr<_Ty2>& _Right, element_type* _Px) noexcept {
        // construct shared_ptr object that aliases _Right
        this->_Alias_construct_from(_Right, _Px);
    }

    template <class _Ty2>
    safeShared_ptr(shared_ptr<_Ty2>&& _Right, element_type* _Px) noexcept {
        // move construct shared_ptr object that aliases _Right
        this->_Alias_move_construct_from(_STD move(_Right), _Px);
    }

    safeShared_ptr(const safeShared_ptr& _Other) noexcept { // construct shared_ptr object that owns same resource as _Other
        this->_Copy_construct_from(_Other);
    }

    template <class _Ty2, enable_if_t<_SP_pointer_compatible<_Ty2, _Ty>::value, int> = 0>
    safeShared_ptr(const shared_ptr<_Ty2>& _Other) noexcept {
        // construct shared_ptr object that owns same resource as _Other
        this->_Copy_construct_from(_Other);
    }

    safeShared_ptr(safeShared_ptr&& _Right) noexcept { // construct shared_ptr object that takes resource from _Right
        this->_Move_construct_from(_STD move(_Right));
    }

    template <class _Ty2, enable_if_t<_SP_pointer_compatible<_Ty2, _Ty>::value, int> = 0>
    safeShared_ptr(shared_ptr<_Ty2>&& _Right) noexcept { // construct shared_ptr object that takes resource from _Right
        this->_Move_construct_from(_STD move(_Right));
    }

    template <class _Ty2, enable_if_t<_SP_pointer_compatible<_Ty2, _Ty>::value, int> = 0>
    explicit safeShared_ptr(const weak_ptr<_Ty2>& _Other) { // construct shared_ptr object that owns resource *_Other
        if (!this->_Construct_from_weak(_Other)) {
            _Throw_bad_weak_ptr();
        }
    }

#if _HAS_AUTO_PTR_ETC
    template <class _Ty2, enable_if_t<is_convertible_v<_Ty2*, _Ty*>, int> = 0>
    safeShared_ptr(auto_ptr<_Ty2>&& _Other) { // construct shared_ptr object that owns *_Other.get()
        _Ty2* _Px = _Other.get();
        _Set_ptr_rep_and_enable_shared(_Px, new _Ref_count<_Ty2>(_Px));
        _Other.release();
    }
#endif // _HAS_AUTO_PTR_ETC

    template <class _Ux, class _Dx,
        enable_if_t<conjunction_v<_SP_pointer_compatible<_Ux, _Ty>,
        is_convertible<typename unique_ptr<_Ux, _Dx>::pointer, element_type*>>,
        int> = 0>
        safeShared_ptr(unique_ptr<_Ux, _Dx>&& _Other) {
        using _Fancy_t = typename unique_ptr<_Ux, _Dx>::pointer;
        using _Raw_t = typename unique_ptr<_Ux, _Dx>::element_type*;
        using _Deleter_t = conditional_t<is_reference_v<_Dx>, decltype(_STD ref(_Other.get_deleter())), _Dx>;

        const _Fancy_t _Fancy = _Other.get();

        if (_Fancy) {
            const _Raw_t _Raw = _Fancy;
            const auto _Rx =
                new _Ref_count_resource<_Fancy_t, _Deleter_t>(_Fancy, _STD forward<_Dx>(_Other.get_deleter()));
            _Set_ptr_rep_and_enable_shared(_Raw, _Rx);
            _Other.release();
        }
    }

    ~safeShared_ptr() noexcept { // release resource
        this->_Decref();
    }

    safeShared_ptr& operator=(const safeShared_ptr& _Right) noexcept {
        safeShared_ptr(_Right).swap(*this);
        return *this;
    }

    template <class _Ty2, enable_if_t<_SP_pointer_compatible<_Ty2, _Ty>::value, int> = 0>
    safeShared_ptr& operator=(const safeShared_ptr<_Ty2>& _Right) noexcept {
        safeShared_ptr(_Right).swap(*this);
        return *this;
    }

    safeShared_ptr& operator=(safeShared_ptr&& _Right) noexcept { // take resource from _Right
        safeShared_ptr(_STD move(_Right)).swap(*this);
        return *this;
    }

    template <class _Ty2, enable_if_t<_SP_pointer_compatible<_Ty2, _Ty>::value, int> = 0>
    safeShared_ptr& operator=(safeShared_ptr<_Ty2>&& _Right) noexcept { // take resource from _Right
        safeShared_ptr(_STD move(_Right)).swap(*this);
        return *this;
    }

#if _HAS_AUTO_PTR_ETC
    template <class _Ty2, enable_if_t<is_convertible_v<_Ty2*, _Ty*>, int> = 0>
    safeShared_ptr& operator=(auto_ptr<_Ty2>&& _Right) {
        safeShared_ptr(_STD move(_Right)).swap(*this);
        return *this;
    }
#endif // _HAS_AUTO_PTR_ETC

    template <class _Ux, class _Dx,
        enable_if_t<conjunction_v<_SP_pointer_compatible<_Ux, _Ty>,
        is_convertible<typename unique_ptr<_Ux, _Dx>::pointer, element_type*>>,
        int> = 0>
        safeShared_ptr& operator=(unique_ptr<_Ux, _Dx>&& _Right) { // move from unique_ptr
        safeShared_ptr(_STD move(_Right)).swap(*this);
        return *this;
    }

    void swap(safeShared_ptr& _Other) noexcept {
        this->_Swap(_Other);
    }

    void reset() noexcept { // release resource and convert to empty shared_ptr object
        safeShared_ptr().swap(*this);
    }

    template <class _Ux,
        enable_if_t<conjunction_v<conditional_t<is_array_v<_Ty>, _Can_array_delete<_Ux>, _Can_scalar_delete<_Ux>>,
        _SP_convertible<_Ux, _Ty>>,
        int> = 0>
        void reset(_Ux* _Px) { // release, take ownership of _Px
        safeShared_ptr(_Px).swap(*this);
    }

    template <class _Ux, class _Dx,
        enable_if_t<conjunction_v<is_move_constructible<_Dx>, _Can_call_function_object<_Dx&, _Ux*&>,
        _SP_convertible<_Ux, _Ty>>,
        int> = 0>
        void reset(_Ux* _Px, _Dx _Dt) { // release, take ownership of _Px, with deleter _Dt
        safeShared_ptr(_Px, _Dt).swap(*this);
    }

    template <class _Ux, class _Dx, class _Alloc,
        enable_if_t<conjunction_v<is_move_constructible<_Dx>, _Can_call_function_object<_Dx&, _Ux*&>,
        _SP_convertible<_Ux, _Ty>>,
        int> = 0>
        void reset(_Ux* _Px, _Dx _Dt, _Alloc _Ax) { // release, take ownership of _Px, with deleter _Dt, allocator _Ax
        safeShared_ptr(_Px, _Dt, _Ax).swap(*this);
    }

    using _Mybase::get;

    template <class _Ty2 = _Ty, enable_if_t<!disjunction_v<is_array<_Ty2>, is_void<_Ty2>>, int> = 0>
    _NODISCARD _Ty2& operator*() const noexcept {
        return *get();
    }

    template <class _Ty2 = _Ty, enable_if_t<!is_array_v<_Ty2>, int> = 0>
    _NODISCARD _Ty2* operator->() const noexcept {
        return get();
    }

    template <class _Ty2 = _Ty, class _Elem = element_type, enable_if_t<is_array_v<_Ty2>, int> = 0>
    _NODISCARD _Elem& operator[](ptrdiff_t _Idx) const noexcept /* strengthened */ {
        return get()[_Idx];
    }

#if _HAS_DEPRECATED_SHARED_PTR_UNIQUE
    _CXX17_DEPRECATE_SHARED_PTR_UNIQUE _NODISCARD bool unique() const noexcept {
        // return true if no other shared_ptr object owns this resource
        return this->use_count() == 1;
    }
#endif // _HAS_DEPRECATED_SHARED_PTR_UNIQUE

    explicit operator bool() const noexcept {
        return get() != nullptr;
    }

private:
    template <class _UxptrOrNullptr, class _Dx>
    void _Setpd(const _UxptrOrNullptr _Px, _Dx _Dt) { // take ownership of _Px, deleter _Dt
        _Temporary_owner_del<_UxptrOrNullptr, _Dx> _Owner(_Px, _Dt);
        _Set_ptr_rep_and_enable_shared(
            _Owner._Ptr, new _SafeRef_count_resource<_UxptrOrNullptr, _Dx>(_Owner._Ptr, _STD move(_Dt)));
        _Owner._Call_deleter = false;
    }

    template <class _UxptrOrNullptr, class _Dx, class _Alloc>
    void _Setpda(const _UxptrOrNullptr _Px, _Dx _Dt, _Alloc _Ax) { // take ownership of _Px, deleter _Dt, allocator _Ax
        using _Alref_alloc = _Rebind_alloc_t<_Alloc, _SafeRef_count_resource_alloc<_UxptrOrNullptr, _Dx, _Alloc>>;

        _Temporary_owner_del<_UxptrOrNullptr, _Dx> _Owner(_Px, _Dt);
        _Alref_alloc _Alref(_Ax);
        _Alloc_construct_ptr<_Alref_alloc> _Constructor(_Alref);
        _Constructor._Allocate();
        _STD _Construct_in_place(*_Constructor._Ptr, _Owner._Ptr, _STD move(_Dt), _Ax);
        _Set_ptr_rep_and_enable_shared(_Owner._Ptr, _STD _Unfancy(_Constructor._Ptr));
        _Constructor._Ptr = nullptr;
        _Owner._Call_deleter = false;
    }

#if _HAS_CXX20
    template <_Not_builtin_array _Ty0, class... _Types>
    friend safeShared_ptr<_Ty0> make_shared(_Types&&... _Args);

    template <_Not_builtin_array _Ty0, class _Alloc, class... _Types>
    friend safeShared_ptr<_Ty0> allocate_shared(const _Alloc& _Al_arg, _Types&&... _Args);

    template <_Bounded_builtin_array _Ty0>
    friend safeShared_ptr<_Ty0> make_shared();

    template <_Bounded_builtin_array _Ty0, class _Alloc>
    friend safeShared_ptr<_Ty0> allocate_shared(const _Alloc& _Al_arg);

    template <_Bounded_builtin_array _Ty0>
    friend shared_ptr<_Ty0> make_shared(const remove_extent_t<_Ty0>& _Val);

    template <_Bounded_builtin_array _Ty0, class _Alloc>
    friend safeShared_ptr<_Ty0> allocate_shared(const _Alloc& _Al_arg, const remove_extent_t<_Ty0>& _Val);

    template <_Not_unbounded_builtin_array _Ty0>
    friend safeShared_ptr<_Ty0> make_shared_for_overwrite();

    template <_Not_unbounded_builtin_array _Ty0, class _Alloc>
    friend safeShared_ptr<_Ty0> allocate_shared_for_overwrite(const _Alloc& _Al_arg);

    template <class _Ty0, class... _ArgTypes>
    friend safeShared_ptr<_Ty0> _Make_shared_unbounded_array(size_t _Count, const _ArgTypes&... _Args);

    template <bool _IsForOverwrite, class _Ty0, class _Alloc, class... _ArgTypes>
    friend safeShared_ptr<_Ty0> _Allocate_shared_unbounded_array(
        const _Alloc& _Al, size_t _Count, const _ArgTypes&... _Args);
#else // ^^^ _HAS_CXX20 / !_HAS_CXX20 vvv
    template <class _Ty0, class... _Types>
    friend safeShared_ptr<_Ty0> make_shared(_Types&&... _Args);

    template <class _Ty0, class _Alloc, class... _Types>
    friend safeShared_ptr<_Ty0> allocate_shared(const _Alloc& _Al_arg, _Types&&... _Args);
#endif // ^^^ !_HAS_CXX20 ^^^

    template <class _Ux>
    void _Set_ptr_rep_and_enable_shared(_Ux* const _Px, _SafeRef_count_base* const _Rx) noexcept { // take ownership of _Px
        this->_Ptr = _Px;
        this->_Rep = _Rx;
        if constexpr (conjunction_v<negation<is_array<_Ty>>, negation<is_volatile<_Ux>>, _Can_enable_shared<_Ux>>) {
            if (_Px && _Px->_Wptr.expired()) {
                _Px->_Wptr = shared_ptr<remove_cv_t<_Ux>>(*this, const_cast<remove_cv_t<_Ux>*>(_Px));
            }
        }
    }

    void _Set_ptr_rep_and_enable_shared(nullptr_t, _SafeRef_count_base* const _Rx) noexcept { // take ownership of nullptr
        this->_Ptr = nullptr;
        this->_Rep = _Rx;
    }
};

#if _HAS_CXX17
template <class _Ty>
safeShared_ptr(safeWeak_ptr<_Ty>) -> safeShared_ptr<_Ty>;

template <class _Ty, class _Dx>
safeShared_ptr(unique_ptr<_Ty, _Dx>) -> safeShared_ptr<_Ty>;
#endif // _HAS_CXX17

_EXPORT_STD template <class _Ty1, class _Ty2>
_NODISCARD bool operator==(const safeShared_ptr<_Ty1>& _Left, const safeShared_ptr<_Ty2>& _Right) noexcept {
    return _Left.get() == _Right.get();
}

#if _HAS_CXX20
_EXPORT_STD template <class _Ty1, class _Ty2>
_NODISCARD strong_ordering operator<=>(const safeShared_ptr<_Ty1>& _Left, const safeShared_ptr<_Ty2>& _Right) noexcept {
    return _Left.get() <=> _Right.get();
}
#else // ^^^ _HAS_CXX20 / !_HAS_CXX20 vvv
template <class _Ty1, class _Ty2>
_NODISCARD bool operator!=(const safeShared_ptr<_Ty1>& _Left, const safeShared_ptr<_Ty2>& _Right) noexcept {
    return _Left.get() != _Right.get();
}

template <class _Ty1, class _Ty2>
_NODISCARD bool operator<(const safeShared_ptr<_Ty1>& _Left, const safeShared_ptr<_Ty2>& _Right) noexcept {
    return _Left.get() < _Right.get();
}

template <class _Ty1, class _Ty2>
_NODISCARD bool operator>=(const safeShared_ptr<_Ty1>& _Left, const safeShared_ptr<_Ty2>& _Right) noexcept {
    return _Left.get() >= _Right.get();
}

template <class _Ty1, class _Ty2>
_NODISCARD bool operator>(const safeShared_ptr<_Ty1>& _Left, const safeShared_ptr<_Ty2>& _Right) noexcept {
    return _Left.get() > _Right.get();
}

template <class _Ty1, class _Ty2>
_NODISCARD bool operator<=(const safeShared_ptr<_Ty1>& _Left, const safeShared_ptr<_Ty2>& _Right) noexcept {
    return _Left.get() <= _Right.get();
}
#endif // ^^^ !_HAS_CXX20 ^^^

_EXPORT_STD template <class _Ty>
_NODISCARD bool operator==(const safeShared_ptr<_Ty>& _Left, nullptr_t) noexcept {
    return _Left.get() == nullptr;
}

#if _HAS_CXX20
_EXPORT_STD template <class _Ty>
_NODISCARD strong_ordering operator<=>(const safeShared_ptr<_Ty>& _Left, nullptr_t) noexcept {
    return _Left.get() <=> static_cast<safeShared_ptr<_Ty>::element_type*>(nullptr);
}
#else // ^^^ _HAS_CXX20 / !_HAS_CXX20 vvv
template <class _Ty>
_NODISCARD bool operator==(nullptr_t, const safeShared_ptr<_Ty>& _Right) noexcept {
    return nullptr == _Right.get();
}

template <class _Ty>
_NODISCARD bool operator!=(const safeShared_ptr<_Ty>& _Left, nullptr_t) noexcept {
    return _Left.get() != nullptr;
}

template <class _Ty>
_NODISCARD bool operator!=(nullptr_t, const safeShared_ptr<_Ty>& _Right) noexcept {
    return nullptr != _Right.get();
}

template <class _Ty>
_NODISCARD bool operator<(const safeShared_ptr<_Ty>& _Left, nullptr_t) noexcept {
    return _Left.get() < static_cast<typename safeShared_ptr<_Ty>::element_type*>(nullptr);
}

template <class _Ty>
_NODISCARD bool operator<(nullptr_t, const safeShared_ptr<_Ty>& _Right) noexcept {
    return static_cast<typename safeShared_ptr<_Ty>::element_type*>(nullptr) < _Right.get();
}

template <class _Ty>
_NODISCARD bool operator>=(const safeShared_ptr<_Ty>& _Left, nullptr_t) noexcept {
    return _Left.get() >= static_cast<typename safeShared_ptr<_Ty>::element_type*>(nullptr);
}

template <class _Ty>
_NODISCARD bool operator>=(nullptr_t, const safeShared_ptr<_Ty>& _Right) noexcept {
    return static_cast<typename safeShared_ptr<_Ty>::element_type*>(nullptr) >= _Right.get();
}

template <class _Ty>
_NODISCARD bool operator>(const safeShared_ptr<_Ty>& _Left, nullptr_t) noexcept {
    return _Left.get() > static_cast<typename safeShared_ptr<_Ty>::element_type*>(nullptr);
}

template <class _Ty>
_NODISCARD bool operator>(nullptr_t, const safeShared_ptr<_Ty>& _Right) noexcept {
    return static_cast<typename safeShared_ptr<_Ty>::element_type*>(nullptr) > _Right.get();
}

template <class _Ty>
_NODISCARD bool operator<=(const safeShared_ptr<_Ty>& _Left, nullptr_t) noexcept {
    return _Left.get() <= static_cast<typename safeShared_ptr<_Ty>::element_type*>(nullptr);
}

template <class _Ty>
_NODISCARD bool operator<=(nullptr_t, const safeShared_ptr<_Ty>& _Right) noexcept {
    return static_cast<typename safeShared_ptr<_Ty>::element_type*>(nullptr) <= _Right.get();
}
#endif // ^^^ !_HAS_CXX20 ^^^

_EXPORT_STD template <class _Elem, class _Traits, class _Ty>
basic_ostream<_Elem, _Traits>& operator<<(basic_ostream<_Elem, _Traits>& _Out, const safeShared_ptr<_Ty>& _Px) {
    // write contained pointer to stream
    return _Out << _Px.get();
}

_EXPORT_STD template <class _Ty>
void swap(safeShared_ptr<_Ty>& _Left, safeShared_ptr<_Ty>& _Right) noexcept {
    _Left.swap(_Right);
}

_EXPORT_STD template <class _Ty1, class _Ty2>
_NODISCARD safeShared_ptr<_Ty1> static_pointer_cast(const safeShared_ptr<_Ty2>& _Other) noexcept {
    // static_cast for shared_ptr that properly respects the reference count control block
    const auto _Ptr = static_cast<typename safeShared_ptr<_Ty1>::element_type*>(_Other.get());
    return safeShared_ptr<_Ty1>(_Other, _Ptr);
}

_EXPORT_STD template <class _Ty1, class _Ty2>
_NODISCARD safeShared_ptr<_Ty1> static_pointer_cast(safeShared_ptr<_Ty2>&& _Other) noexcept {
    // static_cast for shared_ptr that properly respects the reference count control block
    const auto _Ptr = static_cast<typename safeShared_ptr<_Ty1>::element_type*>(_Other.get());
    return safeShared_ptr<_Ty1>(_STD move(_Other), _Ptr);
}

_EXPORT_STD template <class _Ty1, class _Ty2>
_NODISCARD safeShared_ptr<_Ty1> const_pointer_cast(const safeShared_ptr<_Ty2>& _Other) noexcept {
    // const_cast for shared_ptr that properly respects the reference count control block
    const auto _Ptr = const_cast<typename safeShared_ptr<_Ty1>::element_type*>(_Other.get());
    return safeShared_ptr<_Ty1>(_Other, _Ptr);
}

_EXPORT_STD template <class _Ty1, class _Ty2>
_NODISCARD safeShared_ptr<_Ty1> const_pointer_cast(safeShared_ptr<_Ty2>&& _Other) noexcept {
    // const_cast for shared_ptr that properly respects the reference count control block
    const auto _Ptr = const_cast<typename safeShared_ptr<_Ty1>::element_type*>(_Other.get());
    return safeShared_ptr<_Ty1>(_STD move(_Other), _Ptr);
}

_EXPORT_STD template <class _Ty1, class _Ty2>
_NODISCARD safeShared_ptr<_Ty1> reinterpret_pointer_cast(const safeShared_ptr<_Ty2>& _Other) noexcept {
    // reinterpret_cast for shared_ptr that properly respects the reference count control block
    const auto _Ptr = reinterpret_cast<typename safeShared_ptr<_Ty1>::element_type*>(_Other.get());
    return safeShared_ptr<_Ty1>(_Other, _Ptr);
}

_EXPORT_STD template <class _Ty1, class _Ty2>
_NODISCARD safeShared_ptr<_Ty1> reinterpret_pointer_cast(safeShared_ptr<_Ty2>&& _Other) noexcept {
    // reinterpret_cast for shared_ptr that properly respects the reference count control block
    const auto _Ptr = reinterpret_cast<typename safeShared_ptr<_Ty1>::element_type*>(_Other.get());
    return safeShared_ptr<_Ty1>(_STD move(_Other), _Ptr);
}

#ifdef _CPPRTTI
_EXPORT_STD template <class _Ty1, class _Ty2>
_NODISCARD safeShared_ptr<_Ty1> dynamic_pointer_cast(const safeShared_ptr<_Ty2>& _Other) noexcept {
    // dynamic_cast for shared_ptr that properly respects the reference count control block
    const auto _Ptr = dynamic_cast<typename safeShared_ptr<_Ty1>::element_type*>(_Other.get());

    if (_Ptr) {
        return safeShared_ptr<_Ty1>(_Other, _Ptr);
    }

    return {};
}

_EXPORT_STD template <class _Ty1, class _Ty2>
_NODISCARD safeShared_ptr<_Ty1> dynamic_pointer_cast(safeShared_ptr<_Ty2>&& _Other) noexcept {
    // dynamic_cast for shared_ptr that properly respects the reference count control block
    const auto _Ptr = dynamic_cast<typename safeShared_ptr<_Ty1>::element_type*>(_Other.get());

    if (_Ptr) {
        return safeShared_ptr<_Ty1>(_STD move(_Other), _Ptr);
    }

    return {};
}
#else // ^^^ defined(_CPPRTTI) / !defined(_CPPRTTI) vvv
_EXPORT_STD template <class _Ty1, class _Ty2>
safeShared_ptr<_Ty1> dynamic_pointer_cast(const safeShared_ptr<_Ty2>&) noexcept = delete; // requires /GR option
_EXPORT_STD template <class _Ty1, class _Ty2>
safeShared_ptr<_Ty1> dynamic_pointer_cast(safeShared_ptr<_Ty2>&&) noexcept = delete; // requires /GR option
#endif // ^^^ !defined(_CPPRTTI) ^^^

#if _HAS_STATIC_RTTI
_EXPORT_STD template <class _Dx, class _Ty>
_NODISCARD _Dx* get_deleter(const safeShared_ptr<_Ty>& _Sx) noexcept {
    // return pointer to shared_ptr's deleter object if its type is _Dx
    if (_Sx._Rep) {
        return static_cast<_Dx*>(_Sx._Rep->_Get_deleter(typeid(_Dx)));
    }

    return nullptr;
}
#else // ^^^ _HAS_STATIC_RTTI / !_HAS_STATIC_RTTI vvv
_EXPORT_STD template <class _Dx, class _Ty>
_Dx* get_deleter(const safeShared_ptr<_Ty>&) noexcept = delete; // requires static RTTI
#endif // ^^^ !_HAS_STATIC_RTTI ^^^

//#if _HAS_CXX20
//struct _For_overwrite_tag {
//    explicit _For_overwrite_tag() = default;
//};
//#endif // _HAS_CXX20

template <class _Ty>
class _SafeRef_count_obj2 : public _SafeRef_count_base { // handle reference counting for object in control block, no allocator
public:
    template <class... _Types>
    explicit _SafeRef_count_obj2(_Types&&... _Args) : _SafeRef_count_base() {
#if _HAS_CXX20
        if constexpr (sizeof...(_Types) == 1 && (is_same_v<_For_overwrite_tag, remove_cvref_t<_Types>> && ...)) {
            _STD _Default_construct_in_place(_Storage._Value);
            ((void)_Args, ...);
        }
        else
#endif // _HAS_CXX20
        {
            _STD _Construct_in_place(_Storage._Value, _STD forward<_Types>(_Args)...);
        }
    }

    ~_SafeRef_count_obj2() noexcept override { // TRANSITION, should be non-virtual
        // nothing to do, _Storage._Value was already destroyed in _Destroy

        // N4950 [class.dtor]/7:
        // "A defaulted destructor for a class X is defined as deleted if:
        // X is a union-like class that has a variant member with a non-trivial destructor"
    }

    union {
        _Wrap<remove_cv_t<_Ty>> _Storage;
    };

private:
    void _Destroy() noexcept override { // destroy managed resource
        _STD _Destroy_in_place(_Storage._Value);
    }

    void _Delete_this() noexcept override { // destroy self
        delete this;
    }
};

#if _HAS_CXX20

template <class _Ty, bool = is_trivially_destructible_v<remove_extent_t<_Ty>>>
class _SafeRef_count_unbounded_array : public _SafeRef_count_base {
    // handle reference counting for unbounded array with trivial destruction in control block, no allocator
public:
    static_assert(is_unbounded_array_v<_Ty>);

    using _Element_type = remove_extent_t<_Ty>;

    explicit _SafeRef_count_unbounded_array(const size_t _Count) : _SafeRef_count_base() {
        _STD _Uninitialized_value_construct_multidimensional_n(_Get_ptr(), _Count);
    }

    template <class _Arg>
    explicit _SafeRef_count_unbounded_array(const size_t _Count, const _Arg& _Val) : _SafeRef_count_base() {
        if constexpr (is_same_v<_For_overwrite_tag, _Arg>) {
            _STD _Uninitialized_default_construct_multidimensional_n(_Get_ptr(), _Count);
        }
        else {
            _STD _Uninitialized_fill_multidimensional_n(_Get_ptr(), _Count, _Val);
        }
    }

    _NODISCARD auto _Get_ptr() noexcept {
        return _STD addressof(_Storage._Value);
    }

private:
    union {
        _Wrap<remove_cv_t<_Element_type>> _Storage; // flexible array must be last member
    };

    ~_SafeRef_count_unbounded_array() noexcept override { // TRANSITION, should be non-virtual
        // nothing to do, _Ty is trivially destructible

        // See N4950 [class.dtor]/7.
    }

    void _Destroy() noexcept override { // destroy managed resource
        // nothing to do, _Ty is trivially destructible
    }

    void _Delete_this() noexcept override { // destroy self
        this->~_SafeRef_count_unbounded_array();
        _STD _Deallocate_flexible_array(this);
    }
};

template <class _Ty>
class _SafeRef_count_unbounded_array<_Ty, false> : public _SafeRef_count_base {
    // handle reference counting for unbounded array with non-trivial destruction in control block, no allocator
public:
    static_assert(is_unbounded_array_v<_Ty>);

    using _Element_type = remove_extent_t<_Ty>;

    explicit _SafeRef_count_unbounded_array(const size_t _Count) : _SafeRef_count_base(), _Size(_Count) {
        _STD _Uninitialized_value_construct_multidimensional_n(_Get_ptr(), _Size);
    }

    template <class _Arg>
    explicit _SafeRef_count_unbounded_array(const size_t _Count, const _Arg& _Val) : _SafeRef_count_base(), _Size(_Count) {
        if constexpr (is_same_v<_For_overwrite_tag, _Arg>) {
            _STD _Uninitialized_default_construct_multidimensional_n(_Get_ptr(), _Size);
        }
        else {
            _STD _Uninitialized_fill_multidimensional_n(_Get_ptr(), _Size, _Val);
        }
    }

    _NODISCARD auto _Get_ptr() noexcept {
        return _STD addressof(_Storage._Value);
    }

private:
    size_t _Size;

    union {
        _Wrap<remove_cv_t<_Element_type>> _Storage; // flexible array must be last member
    };

    ~_SafeRef_count_unbounded_array() noexcept override { // TRANSITION, should be non-virtual
        // nothing to do, _Storage was already destroyed in _Destroy

        // See N4950 [class.dtor]/7.
    }

    void _Destroy() noexcept override { // destroy managed resource
        _STD _Reverse_destroy_multidimensional_n(_Get_ptr(), _Size);
    }

    void _Delete_this() noexcept override { // destroy self
        this->~_SafeRef_count_unbounded_array();
        _STD _Deallocate_flexible_array(this);
    }
};

template <class _Ty>
class _SafeRef_count_bounded_array : public _SafeRef_count_base {
    // handle reference counting for bounded array in control block, no allocator
public:
    static_assert(is_bounded_array_v<_Ty>);

    _SafeRef_count_bounded_array() : _SafeRef_count_base(), _Storage() {} // value-initializing _Storage is necessary here

    template <class _Arg>
    explicit _SafeRef_count_bounded_array(const _Arg& _Val) : _SafeRef_count_base() { // don't value-initialize _Storage
        if constexpr (is_same_v<_For_overwrite_tag, _Arg>) {
            _STD _Uninitialized_default_construct_multidimensional_n(_Storage._Value, extent_v<_Ty>);
        }
        else {
            _STD _Uninitialized_fill_multidimensional_n(_Storage._Value, extent_v<_Ty>, _Val);
        }
    }

    union {
        _Wrap<remove_cv_t<_Ty>> _Storage;
    };

private:
    ~_SafeRef_count_bounded_array() noexcept override { // TRANSITION, should be non-virtual
        // nothing to do, _Storage was already destroyed in _Destroy

        // See N4950 [class.dtor]/7.
    }

    void _Destroy() noexcept override { // destroy managed resource
        // not _Storage._Value as _Ty is an array type (not a class type or a scalar type),
        // and thus cannot be used as a pseudo-destructor (N4950 [expr.prim.id.dtor]).
        _STD _Destroy_in_place(_Storage);
    }

    void _Delete_this() noexcept override { // destroy self
        delete this;
    }
};
#endif // _HAS_CXX20


template <class _Ty, class _Alloc>
class _SafeRef_count_obj_alloc3 : public _Ebco_base<_Rebind_alloc_t<_Alloc, _Ty>>, public _SafeRef_count_base {
    // handle reference counting for object in control block, allocator
private:
    static_assert(is_same_v<_Ty, remove_cv_t<_Ty>>, "allocate_shared should remove_cv_t");

    using _Rebound = _Rebind_alloc_t<_Alloc, _Ty>;

public:
    template <class... _Types>
    explicit _SafeRef_count_obj_alloc3(const _Alloc& _Al_arg, _Types&&... _Args)
        : _Ebco_base<_Rebound>(_Al_arg), _SafeRef_count_base() {
#if _HAS_CXX20 && defined(_ENABLE_STL_INTERNAL_CHECK)
        if constexpr (sizeof...(_Types) == 1) {
            // allocate_shared_for_overwrite should use another type for the control block
            _STL_INTERNAL_STATIC_ASSERT(!(is_same_v<_For_overwrite_tag, remove_cvref_t<_Types>> && ...));
        }
#endif // _HAS_CXX20 && defined(_ENABLE_STL_INTERNAL_CHECK)
        allocator_traits<_Rebound>::construct(
            this->_Get_val(), _STD addressof(_Storage._Value), _STD forward<_Types>(_Args)...);
    }

    union {
        _Wrap<_Ty> _Storage;
    };

private:
    ~_SafeRef_count_obj_alloc3() noexcept override { // TRANSITION, should be non-virtual
        // nothing to do; _Storage._Value already destroyed by _Destroy()

        // See N4950 [class.dtor]/7.
    }

    void _Destroy() noexcept override { // destroy managed resource
        allocator_traits<_Rebound>::destroy(this->_Get_val(), _STD addressof(_Storage._Value));
    }

    void _Delete_this() noexcept override { // destroy self
        _Rebind_alloc_t<_Alloc, _Ref_count_obj_alloc3> _Al(this->_Get_val());
        this->~_SafeRef_count_obj_alloc3();
        _STD _Deallocate_plain(_Al, this);
    }
};

#if _HAS_CXX20
template <class _Ty, class _Alloc>
class _SafeRef_count_obj_alloc_for_overwrite : public _Ebco_base<_Rebind_alloc_t<_Alloc, _Ty>>, public _SafeRef_count_base {
    // handle reference counting for object in control block, allocator
    // initialize and destroy objects by the default mechanism
private:
    static_assert(is_same_v<_Ty, remove_cv_t<_Ty>>, "allocate_shared_for_overwrite should remove_cv_t");

    using _Rebound = _Rebind_alloc_t<_Alloc, _Ty>;

public:
    template <class... _Types>
    explicit _SafeRef_count_obj_alloc_for_overwrite(const _Alloc& _Al_arg)
        : _Ebco_base<_Rebound>(_Al_arg), _SafeRef_count_base() {
        _STD _Default_construct_in_place(_Storage._Value);
    }

    union {
        _Wrap<_Ty> _Storage;
    };

private:
    ~_SafeRef_count_obj_alloc_for_overwrite() noexcept override { // TRANSITION, should be non-virtual
        // nothing to do; _Storage._Value already destroyed by _Destroy()

        // See N4964 [class.dtor]/7.
    }

    void _Destroy() noexcept override { // destroy managed resource
        _STD _Destroy_in_place(_Storage._Value); // use the default mechanism per LWG-4024
    }

    void _Delete_this() noexcept override { // destroy self
        _Rebind_alloc_t<_Alloc, _SafeRef_count_obj_alloc_for_overwrite> _Al(this->_Get_val());
        this->~_SafeRef_count_obj_alloc_for_overwrite();
        _STD _Deallocate_plain(_Al, this);
    }
};

template <class _Ty, class _Alloc>
class _SafeRef_count_unbounded_array_alloc : public _Ebco_base<_Rebind_alloc_t<_Alloc, remove_all_extents_t<_Ty>>>,
    public _SafeRef_count_base {
    // handle reference counting for unbounded array in control block, allocator
private:
    static_assert(is_unbounded_array_v<_Ty>);
    static_assert(is_same_v<_Ty, remove_cv_t<_Ty>>, "allocate_shared should remove_cv_t");

    using _Item = remove_all_extents_t<_Ty>;
    using _Rebound = _Rebind_alloc_t<_Alloc, _Item>;

public:
    using _Element_type = remove_extent_t<_Ty>;

    explicit _SafeRef_count_unbounded_array_alloc(const _Alloc& _Al_arg, const size_t _Count)
        : _Ebco_base<_Rebound>(_Al_arg), _SafeRef_count_base(), _Size(_Count) {
        _STD _Uninitialized_value_construct_multidimensional_n_al(_Get_ptr(), _Size, this->_Get_val());
    }

    template <class _Arg>
    explicit _SafeRef_count_unbounded_array_alloc(const _Alloc& _Al_arg, const size_t _Count, const _Arg& _Val)
        : _Ebco_base<_Rebound>(_Al_arg), _SafeRef_count_base(), _Size(_Count) {
        // allocate_shared_for_overwrite should use another type for the control block
        _STL_INTERNAL_STATIC_ASSERT(!is_same_v<_For_overwrite_tag, _Arg>);

        _STD _Uninitialized_fill_multidimensional_n_al(_Get_ptr(), _Size, _Val, this->_Get_val());
    }

    _NODISCARD auto _Get_ptr() noexcept {
        return _STD addressof(_Storage._Value);
    }

private:
    size_t _Size;

    union {
        _Wrap<_Element_type> _Storage; // flexible array must be last member
    };

    ~_SafeRef_count_unbounded_array_alloc() noexcept override { // TRANSITION, should be non-virtual
        // nothing to do; _Storage._Value already destroyed by _Destroy()

        // See N4950 [class.dtor]/7.
    }

    void _Destroy() noexcept override { // destroy managed resource
        if constexpr (!conjunction_v<is_trivially_destructible<_Item>, _Uses_default_destroy<_Rebound, _Item*>>) {
            _STD _Reverse_destroy_multidimensional_n_al(_Get_ptr(), _Size, this->_Get_val());
        }
    }

    void _Delete_this() noexcept override { // destroy self
        constexpr size_t _Align = alignof(_SafeRef_count_unbounded_array_alloc);
        using _Storage = _Alignas_storage_unit<_Align>;
        using _Rebound_alloc = _Rebind_alloc_t<_Alloc, _Storage>;

        _Rebound_alloc _Al(this->_Get_val());
        const size_t _Bytes =
            _Calculate_bytes_for_flexible_array<_SafeRef_count_unbounded_array_alloc, _Check_overflow::_Nope>(_Size);
        const size_t _Storage_units = _Bytes / sizeof(_Storage);

        this->~_SafeRef_count_unbounded_array_alloc();

        _Al.deallocate(_STD _Refancy<_Alloc_ptr_t<_Rebound_alloc>>(reinterpret_cast<_Storage*>(this)),
            static_cast<_Alloc_size_t<_Rebound_alloc>>(_Storage_units));
    }
};
template <class _Ty, class _Alloc>
class _SafeRef_count_unbounded_array_alloc_for_overwrite
    : public _Ebco_base<_Rebind_alloc_t<_Alloc, remove_all_extents_t<_Ty>>>,
    public _SafeRef_count_base {
    // handle reference counting for unbounded array in control block, allocator
    // initialize and destroy objects by the default mechanism
private:
    static_assert(is_unbounded_array_v<_Ty>);
    static_assert(is_same_v<_Ty, remove_cv_t<_Ty>>, "allocate_shared_for_overwrite should remove_cv_t");

    using _Item = remove_all_extents_t<_Ty>;
    using _Rebound = _Rebind_alloc_t<_Alloc, _Item>;

public:
    using _Element_type = remove_extent_t<_Ty>;

    explicit _SafeRef_count_unbounded_array_alloc_for_overwrite(const _Alloc& _Al_arg, const size_t _Count)
        : _Ebco_base<_Rebound>(_Al_arg), _SafeRef_count_base(), _Size(_Count) {
        _STD _Uninitialized_default_construct_multidimensional_n(_Get_ptr(), _Size); // the allocator isn't needed
    }

    _NODISCARD auto _Get_ptr() noexcept {
        return _STD addressof(_Storage._Value);
    }

private:
    size_t _Size;

    union {
        _Wrap<_Element_type> _Storage; // flexible array must be last member
    };

    ~_SafeRef_count_unbounded_array_alloc_for_overwrite() noexcept override { // TRANSITION, should be non-virtual
        // nothing to do; _Storage._Value already destroyed by _Destroy()

        // See N4964 [class.dtor]/7.
    }

    void _Destroy() noexcept override { // destroy managed resource
        _STD _Reverse_destroy_multidimensional_n(_Get_ptr(), _Size); // use the default mechanism per LWG-4024
    }

    void _Delete_this() noexcept override { // destroy self
        constexpr size_t _Align = alignof(_SafeRef_count_unbounded_array_alloc_for_overwrite);
        using _Storage = _Alignas_storage_unit<_Align>;
        using _Rebound_alloc = _Rebind_alloc_t<_Alloc, _Storage>;

        _Rebound_alloc _Al(this->_Get_val());
        const size_t _Bytes = _Calculate_bytes_for_flexible_array< //
            _SafeRef_count_unbounded_array_alloc_for_overwrite, _Check_overflow::_Nope>(_Size);
        const size_t _Storage_units = _Bytes / sizeof(_Storage);

        this->~_SafeRef_count_unbounded_array_alloc_for_overwrite();

        _Al.deallocate(_STD _Refancy<_Alloc_ptr_t<_Rebound_alloc>>(reinterpret_cast<_Storage*>(this)),
            static_cast<_Alloc_size_t<_Rebound_alloc>>(_Storage_units));
    }
};

template <class _Ty, class _Alloc>
class _SafeRef_count_bounded_array_alloc : public _Ebco_base<_Rebind_alloc_t<_Alloc, remove_all_extents_t<_Ty>>>,
    public _SafeRef_count_base {
    // handle reference counting for bounded array in control block, allocator
private:
    static_assert(is_bounded_array_v<_Ty>);
    static_assert(is_same_v<_Ty, remove_cv_t<_Ty>>, "allocate_shared should remove_cv_t");

    using _Item = remove_all_extents_t<_Ty>;
    using _Rebound = _Rebind_alloc_t<_Alloc, _Item>;

public:
    explicit _SafeRef_count_bounded_array_alloc(const _Alloc& _Al_arg)
        : _Ebco_base<_Rebound>(_Al_arg), _SafeRef_count_base() { // don't value-initialize _Storage
        _STD _Uninitialized_value_construct_multidimensional_n_al(_Storage._Value, extent_v<_Ty>, this->_Get_val());
    }

    template <class _Arg>
    explicit _SafeRef_count_bounded_array_alloc(const _Alloc& _Al_arg, const _Arg& _Val)
        : _Ebco_base<_Rebound>(_Al_arg), _SafeRef_count_base() { // don't value-initialize _Storage
        // allocate_shared_for_overwrite should use another type for the control block
        _STL_INTERNAL_STATIC_ASSERT(!is_same_v<_For_overwrite_tag, _Arg>);

        _STD _Uninitialized_fill_multidimensional_n_al(_Storage._Value, extent_v<_Ty>, _Val, this->_Get_val());
    }

    union {
        _Wrap<_Ty> _Storage;
    };

private:
    ~_SafeRef_count_bounded_array_alloc() noexcept override { // TRANSITION, should be non-virtual
        // nothing to do; _Storage._Value already destroyed by _Destroy()

        // See N4950 [class.dtor]/7.
    }

    void _Destroy() noexcept override { // destroy managed resource
        if constexpr (!conjunction_v<is_trivially_destructible<_Item>, _Uses_default_destroy<_Rebound, _Item*>>) {
            _STD _Reverse_destroy_multidimensional_n_al(_Storage._Value, extent_v<_Ty>, this->_Get_val());
        }
    }

    void _Delete_this() noexcept override { // destroy self
        _Rebind_alloc_t<_Alloc, _Ref_count_bounded_array_alloc> _Al(this->_Get_val());
        this->~_SafeRef_count_bounded_array_alloc();
        _STD _Deallocate_plain(_Al, this);
    }
};
template <class _Ty, class _Alloc>
class _SafeRef_count_bounded_array_alloc_for_overwrite
    : public _Ebco_base<_Rebind_alloc_t<_Alloc, remove_all_extents_t<_Ty>>>,
    public _SafeRef_count_base {
    // handle reference counting for bounded array in control block, allocator
    // initialize and destroy objects by the default mechanism
private:
    static_assert(is_bounded_array_v<_Ty>);
    static_assert(is_same_v<_Ty, remove_cv_t<_Ty>>, "allocate_shared_for_overwrite should remove_cv_t");

    using _Item = remove_all_extents_t<_Ty>;
    using _Rebound = _Rebind_alloc_t<_Alloc, _Item>;

public:
    explicit _SafeRef_count_bounded_array_alloc_for_overwrite(const _Alloc& _Al_arg)
        : _Ebco_base<_Rebound>(_Al_arg), _SafeRef_count_base() { // don't value-initialize _Storage
        _STD _Uninitialized_default_construct_multidimensional_n(
            _Storage._Value, extent_v<_Ty>); // the allocator isn't needed
    }

    union {
        _Wrap<_Ty> _Storage;
    };

private:
    ~_SafeRef_count_bounded_array_alloc_for_overwrite() noexcept override { // TRANSITION, should be non-virtual
        // nothing to do; _Storage._Value already destroyed by _Destroy()

        // See N4964 [class.dtor]/7.
    }

    void _Destroy() noexcept override { // destroy managed resource
        // not _Storage._Value as _Ty is an array type (not a class type or a scalar type),
        // and thus cannot be used as a pseudo-destructor (N4964 [expr.prim.id.dtor]).
        _STD _Destroy_in_place(_Storage); // use the default mechanism per LWG-4024
    }

    void _Delete_this() noexcept override { // destroy self
        _Rebind_alloc_t<_Alloc, _SafeRef_count_bounded_array_alloc_for_overwrite> _Al(this->_Get_val());
        this->~_SafeRef_count_bounded_array_alloc_for_overwrite();
        _STD _Deallocate_plain(_Al, this);
    }
};
#endif // _HAS_CXX20

#if _HAS_CXX20
_EXPORT_STD template <_Not_builtin_array _Ty, class... _Types>
#else // ^^^ _HAS_CXX20 / !_HAS_CXX20 vvv
template <class _Ty, class... _Types>
#endif // ^^^ !_HAS_CXX20 ^^^
_NODISCARD_SMART_PTR_ALLOC safeShared_ptr<_Ty> make_shared(_Types&&... _Args) { // make a shared_ptr to non-array object
    const auto _Rx = new _SafeRef_count_obj2<_Ty>(_STD forward<_Types>(_Args)...);
    safeShared_ptr<_Ty> _Ret;
    _Ret._Set_ptr_rep_and_enable_shared(_STD addressof(_Rx->_Storage._Value), _Rx);
    return _Ret;
}

#if _HAS_CXX20
/* template <class _Refc>
struct _NODISCARD _Global_delete_guard {
    _Refc* _Target;

    ~_Global_delete_guard() {
        // While this branch is technically unnecessary because N4950 [new.delete.single]/16 requires
        // `::operator delete(nullptr)` to be a no-op, it's here to help optimizers see that after
        // `_Guard._Target = nullptr;`, this destructor can be eliminated.
        if (_Target) {
            _STD _Deallocate_flexible_array(_Target);
        }
    }
};
*/
template <class _Ty, class... _ArgTypes>
_NODISCARD safeShared_ptr<_Ty> _Make_shared_unbounded_array(const size_t _Count, const _ArgTypes&... _Args) {
    // make a shared_ptr to an unbounded array
    static_assert(is_unbounded_array_v<_Ty>);
    using _SafeRefc = _SafeRef_count_unbounded_array<_Ty>;
    const auto _Rx = _Allocate_flexible_array<_SafeRefc>(_Count);
    _Global_delete_guard<_SafeRefc> _Guard{ _Rx };
    ::new (static_cast<void*>(_Rx)) _SafeRefc(_Count, _Args...);
    _Guard._Target = nullptr;
    safeShared_ptr<_Ty> _Ret;
    _Ret._Set_ptr_rep_and_enable_shared(_Rx->_Get_ptr(), _Rx);
    return _Ret;
}

_EXPORT_STD template <_Unbounded_builtin_array _Ty>
_NODISCARD_SMART_PTR_ALLOC safeShared_ptr<_Ty> make_shared(const size_t _Count) {
    return _STD _Make_shared_unbounded_array<_Ty>(_Count);
}

_EXPORT_STD template <_Unbounded_builtin_array _Ty>
_NODISCARD_SMART_PTR_ALLOC safeShared_ptr<_Ty> make_shared(const size_t _Count, const remove_extent_t<_Ty>& _Val) {
    return _STD _Make_shared_unbounded_array<_Ty>(_Count, _Val);
}

_EXPORT_STD template <_Bounded_builtin_array _Ty>
_NODISCARD_SMART_PTR_ALLOC safeShared_ptr<_Ty> make_shared() {
    // make a shared_ptr to a bounded array
    const auto _Rx = new _SafeRef_count_bounded_array<_Ty>();
    safeShared_ptr<_Ty> _Ret;
    _Ret._Set_ptr_rep_and_enable_shared(_Rx->_Storage._Value, _Rx);
    return _Ret;
}

_EXPORT_STD template <_Bounded_builtin_array _Ty>
_NODISCARD_SMART_PTR_ALLOC safeShared_ptr<_Ty> make_shared(const remove_extent_t<_Ty>& _Val) {
    // make a shared_ptr to a bounded array
    const auto _Rx = new _SafeRef_count_bounded_array<_Ty>(_Val);
    shared_ptr<_Ty> _Ret;
    _Ret._Set_ptr_rep_and_enable_shared(_Rx->_Storage._Value, _Rx);
    return _Ret;
}

_EXPORT_STD template <_Not_unbounded_builtin_array _Ty>
_NODISCARD_SMART_PTR_ALLOC safeShared_ptr<_Ty> make_shared_for_overwrite() {
    shared_ptr<_Ty> _Ret;
    if constexpr (is_array_v<_Ty>) {
        // make a shared_ptr to a bounded array
        const auto _Rx = new _SafeRef_count_bounded_array<_Ty>(_For_overwrite_tag{});
        _Ret._Set_ptr_rep_and_enable_shared(_Rx->_Storage._Value, _Rx);
    }
    else {
        // make a shared_ptr to non-array object
        const auto _Rx = new _SafeRef_count_obj2<_Ty>(_For_overwrite_tag{});
        _Ret._Set_ptr_rep_and_enable_shared(_STD addressof(_Rx->_Storage._Value), _Rx);
    }
    return _Ret;
}

_EXPORT_STD template <_Unbounded_builtin_array _Ty>
_NODISCARD_SMART_PTR_ALLOC safeShared_ptr<_Ty> make_shared_for_overwrite(const size_t _Count) {
    return _STD _Make_shared_unbounded_array<_Ty>(_Count, _For_overwrite_tag{});
}
#endif // _HAS_CXX20

#if _HAS_CXX20
_EXPORT_STD template <_Not_builtin_array _Ty, class _Alloc, class... _Types>
#else // ^^^ _HAS_CXX20 / !_HAS_CXX20 vvv
template <class _Ty, class _Alloc, class... _Types>
#endif // ^^^ !_HAS_CXX20 ^^^
_NODISCARD_SMART_PTR_ALLOC safeShared_ptr<_Ty> allocate_shared(const _Alloc& _Al, _Types&&... _Args) {
    // make a shared_ptr to non-array object
    // Note: As of 2019-05-28, this implements the proposed resolution of LWG-3210 (which controls whether
    // allocator::construct sees T or const T when _Ty is const qualified)
    using _SafeRefoa = _SafeRef_count_obj_alloc3<remove_cv_t<_Ty>, _Alloc>;
    using _SafeAlblock = _Rebind_alloc_t<_Alloc, _SafeRefoa>;
    _SafeAlblock _Rebound(_Al);
    _Alloc_construct_ptr<_SafeAlblock> _Constructor{ _Rebound };
    _Constructor._Allocate();
    _STD _Construct_in_place(*_Constructor._Ptr, _Al, _STD forward<_Types>(_Args)...);
    shared_ptr<_Ty> _Ret;
    const auto _Ptr = reinterpret_cast<_Ty*>(_STD addressof(_Constructor._Ptr->_Storage._Value));
    _Ret._Set_ptr_rep_and_enable_shared(_Ptr, _STD _Unfancy(_Constructor._Release()));
    return _Ret;
}

#if _HAS_CXX20
/*template <class _Alloc>
struct _Allocate_n_ptr {
    _Alloc& _Al;
    _Alloc_ptr_t<_Alloc> _Ptr;
    size_t _Nx;

    _Allocate_n_ptr(_Alloc& _Al_, const size_t _Nx_)
        : _Al(_Al_), _Ptr(_Al_.allocate(_Convert_size<_Alloc_size_t<_Alloc>>(_Nx_))), _Nx(_Nx_) {}

    ~_Allocate_n_ptr() {
        if (_Ptr) {
            _Al.deallocate(_Ptr, static_cast<_Alloc_size_t<_Alloc>>(_Nx));
        }
    }

    _Allocate_n_ptr(const _Allocate_n_ptr&) = delete;
    _Allocate_n_ptr& operator=(const _Allocate_n_ptr&) = delete;
};
*/
template <bool _IsForOverwrite, class _Ty, class _Alloc, class... _ArgTypes>
_NODISCARD safeShared_ptr<_Ty> _Allocate_shared_unbounded_array(
    const _Alloc& _Al, const size_t _Count, const _ArgTypes&... _Args) {
    // make a shared_ptr to an unbounded array
    static_assert(is_unbounded_array_v<_Ty>);
    using _Refc = conditional_t<_IsForOverwrite, //
        _SafeRef_count_unbounded_array_alloc_for_overwrite<remove_cv_t<_Ty>, _Alloc>,
        _SafeRef_count_unbounded_array_alloc<remove_cv_t<_Ty>, _Alloc>>;
    constexpr size_t _Align = alignof(_Refc);
    using _Storage = _Alignas_storage_unit<_Align>;
    _Rebind_alloc_t<_Alloc, _Storage> _Rebound(_Al);
    const size_t _Bytes = _Calculate_bytes_for_flexible_array<_Refc, _Check_overflow::_Yes>(_Count);
    const size_t _Storage_units = _Bytes / sizeof(_Storage);
    _Allocate_n_ptr _Guard{ _Rebound, _Storage_units };
    const auto _Rx = reinterpret_cast<_Refc*>(_STD _Unfancy(_Guard._Ptr));
    ::new (static_cast<void*>(_Rx)) _Refc(_Al, _Count, _Args...);
    _Guard._Ptr = nullptr;
    safeShared_ptr<_Ty> _Ret;
    _Ret._Set_ptr_rep_and_enable_shared(_Rx->_Get_ptr(), _Rx);
    return _Ret;
}

_EXPORT_STD template <_Unbounded_builtin_array _Ty, class _Alloc>
_NODISCARD_SMART_PTR_ALLOC safeShared_ptr<_Ty> allocate_shared(const _Alloc& _Al, const size_t _Count) {
    return _STD _Allocate_shared_unbounded_array<false, _Ty>(_Al, _Count);
}

_EXPORT_STD template <_Unbounded_builtin_array _Ty, class _Alloc>
_NODISCARD_SMART_PTR_ALLOC safeShared_ptr<_Ty> allocate_shared(
    const _Alloc& _Al, const size_t _Count, const remove_extent_t<_Ty>& _Val) {
    return _STD _Allocate_shared_unbounded_array<false, _Ty>(_Al, _Count, _Val);
}

_EXPORT_STD template <_Bounded_builtin_array _Ty, class _Alloc>
_NODISCARD_SMART_PTR_ALLOC safeShared_ptr<_Ty> allocate_shared(const _Alloc& _Al) {
    // make a shared_ptr to a bounded array
    using _Refc = _SafeRef_count_bounded_array_alloc<remove_cv_t<_Ty>, _Alloc>;
    using _Alblock = _Rebind_alloc_t<_Alloc, _Refc>;
    _Alblock _Rebound(_Al);
    _Alloc_construct_ptr _Constructor{ _Rebound };
    _Constructor._Allocate();
    ::new (_STD _Voidify_unfancy(_Constructor._Ptr)) _Refc(_Al);
    safeShared_ptr<_Ty> _Ret;
    const auto _Ptr = static_cast<remove_extent_t<_Ty>*>(_Constructor._Ptr->_Storage._Value);
    _Ret._Set_ptr_rep_and_enable_shared(_Ptr, _STD _Unfancy(_Constructor._Release()));
    return _Ret;
}

_EXPORT_STD template <_Bounded_builtin_array _Ty, class _Alloc>
_NODISCARD_SMART_PTR_ALLOC safeShared_ptr<_Ty> allocate_shared(const _Alloc& _Al, const remove_extent_t<_Ty>& _Val) {
    // make a shared_ptr to a bounded array
    using _SafeRefc = _SafeRef_count_bounded_array_alloc<remove_cv_t<_Ty>, _Alloc>;
    using _SafeAlblock = _Rebind_alloc_t<_Alloc, _SafeRefc>;
    _SafeAlblock _Rebound(_Al);
    _Alloc_construct_ptr _Constructor{ _Rebound };
    _Constructor._Allocate();
    ::new (_STD _Voidify_unfancy(_Constructor._Ptr)) _SafeRefc(_Al, _Val);
    safeShared_ptr<_Ty> _Ret;
    const auto _Ptr = static_cast<remove_extent_t<_Ty>*>(_Constructor._Ptr->_Storage._Value);
    _Ret._Set_ptr_rep_and_enable_shared(_Ptr, _STD _Unfancy(_Constructor._Release()));
    return _Ret;
}

_EXPORT_STD template <_Not_unbounded_builtin_array _Ty, class _Alloc>
_NODISCARD_SMART_PTR_ALLOC safeShared_ptr<_Ty> allocate_shared_for_overwrite(const _Alloc& _Al) {
    safeShared_ptr<_Ty> _Ret;
    if constexpr (is_array_v<_Ty>) {
        // make a shared_ptr to a bounded array
        using _SafeRefc = _SafeRef_count_bounded_array_alloc_for_overwrite<remove_cv_t<_Ty>, _Alloc>;
        using _SafeAlblock = _Rebind_alloc_t<_Alloc, _SafeRefc>;
        _SafeAlblock _Rebound(_Al);
        _Alloc_construct_ptr _Constructor{ _Rebound };
        _Constructor._Allocate();
        ::new (_STD _Voidify_unfancy(_Constructor._Ptr)) _SafeRefc(_Al);
        const auto _Ptr = static_cast<remove_extent_t<_Ty>*>(_Constructor._Ptr->_Storage._Value);
        _Ret._Set_ptr_rep_and_enable_shared(_Ptr, _STD _Unfancy(_Constructor._Release()));
    }
    else {
        // make a shared_ptr to non-array object
        using _SafeRefoa = _SafeRef_count_obj_alloc_for_overwrite<remove_cv_t<_Ty>, _Alloc>;
        using _SafeAlblock = _Rebind_alloc_t<_Alloc, _SafeRefoa>;
        _SafeAlblock _Rebound(_Al);
        _Alloc_construct_ptr<_SafeAlblock> _Constructor{ _Rebound };
        _Constructor._Allocate();
        _STD _Construct_in_place(*_Constructor._Ptr, _Al);
        const auto _Ptr = reinterpret_cast<_Ty*>(_STD addressof(_Constructor._Ptr->_Storage._Value));
        _Ret._Set_ptr_rep_and_enable_shared(_Ptr, _STD _Unfancy(_Constructor._Release()));
    }

    return _Ret;
}

_EXPORT_STD template <_Unbounded_builtin_array _Ty, class _Alloc>
_NODISCARD_SMART_PTR_ALLOC safeShared_ptr<_Ty> allocate_shared_for_overwrite(const _Alloc& _Al, const size_t _Count) {
    return _STD _Allocate_shared_unbounded_array<true, _Ty>(_Al, _Count);
}
#endif // _HAS_CXX20

_EXPORT_STD template <class _Ty>
class safeWeak_ptr : public _SafePtr_base<_Ty> { // class for pointer to reference counted resource
public:
#ifndef _M_CEE_PURE
    // When a constructor is converting from weak_ptr<_Ty2> to weak_ptr<_Ty>, the below type trait intentionally asks
    // whether it would be possible to static_cast from _Ty* to const _Ty2*; see N4950 [expr.static.cast]/12.

    // Primary template, the value is used when the substitution fails.
    template <class _Ty2, class = const _Ty2*>
    static constexpr bool _Must_avoid_expired_conversions_from = true;

    // Template specialization, the value is used when the substitution succeeds.
    template <class _Ty2>
    static constexpr bool
        _Must_avoid_expired_conversions_from<_Ty2, decltype(static_cast<const _Ty2*>(static_cast<_Ty*>(nullptr)))> =
        false;
#endif // ^^^ !defined(_M_CEE_PURE) ^^^

    constexpr safeWeak_ptr() noexcept {}

    safeWeak_ptr(const safeWeak_ptr& _Other) noexcept {
        this->_Weakly_construct_from(_Other); // same type, no conversion
    }

    template <class _Ty2, enable_if_t<_SP_pointer_compatible<_Ty2, _Ty>::value, int> = 0>
    safeWeak_ptr(const safeShared_ptr<_Ty2>& _Other) noexcept {
        this->_Weakly_construct_from(_Other); // shared_ptr keeps resource alive during conversion
    }

    template <class _Ty2, enable_if_t<_SP_pointer_compatible<_Ty2, _Ty>::value, int> = 0>
    safeWeak_ptr(const safeWeak_ptr<_Ty2>& _Other) noexcept {
#ifdef _M_CEE_PURE
        constexpr bool _Avoid_expired_conversions = true; // slow, but always safe; avoids error LNK1179
#else
        constexpr bool _Avoid_expired_conversions = _Must_avoid_expired_conversions_from<_Ty2>;
#endif

        if constexpr (_Avoid_expired_conversions) {
            this->_Weakly_convert_lvalue_avoiding_expired_conversions(_Other);
        }
        else {
            this->_Weakly_construct_from(_Other);
        }
    }

    safeWeak_ptr(safeWeak_ptr&& _Other) noexcept {
        this->_Move_construct_from(_STD move(_Other));
    }

    template <class _Ty2, enable_if_t<_SP_pointer_compatible<_Ty2, _Ty>::value, int> = 0>
    safeWeak_ptr(safeWeak_ptr<_Ty2>&& _Other) noexcept {
#ifdef _M_CEE_PURE
        constexpr bool _Avoid_expired_conversions = true; // slow, but always safe; avoids error LNK1179
#else
        constexpr bool _Avoid_expired_conversions = _Must_avoid_expired_conversions_from<_Ty2>;
#endif

        if constexpr (_Avoid_expired_conversions) {
            this->_Weakly_convert_rvalue_avoiding_expired_conversions(_STD move(_Other));
        }
        else {
            this->_Move_construct_from(_STD move(_Other));
        }
    }

    ~safeWeak_ptr() noexcept {
        this->_Decwref();
    }

    safeWeak_ptr& operator=(const safeWeak_ptr& _Right) noexcept {
        safeWeak_ptr(_Right).swap(*this);
        return *this;
    }

    template <class _Ty2, enable_if_t<_SP_pointer_compatible<_Ty2, _Ty>::value, int> = 0>
    safeWeak_ptr& operator=(const weak_ptr<_Ty2>& _Right) noexcept {
        safeWeak_ptr(_Right).swap(*this);
        return *this;
    }

    safeWeak_ptr& operator=(safeWeak_ptr&& _Right) noexcept {
        safeWeak_ptr(_STD move(_Right)).swap(*this);
        return *this;
    }

    template <class _Ty2, enable_if_t<_SP_pointer_compatible<_Ty2, _Ty>::value, int> = 0>
    safeWeak_ptr& operator=(weak_ptr<_Ty2>&& _Right) noexcept {
        safeWeak_ptr(_STD move(_Right)).swap(*this);
        return *this;
    }

    template <class _Ty2, enable_if_t<_SP_pointer_compatible<_Ty2, _Ty>::value, int> = 0>
    safeWeak_ptr& operator=(const shared_ptr<_Ty2>& _Right) noexcept {
        safeWeak_ptr(_Right).swap(*this);
        return *this;
    }

    void reset() noexcept { // release resource, convert to null weak_ptr object
        safeWeak_ptr{}.swap(*this);
    }

    void swap(safeWeak_ptr& _Other) noexcept {
        this->_Swap(_Other);
    }

    _NODISCARD bool expired() const noexcept {
        return this->use_count() == 0;
    }

    _NODISCARD safeShared_ptr<_Ty> lock() const noexcept { // convert to shared_ptr
        safeShared_ptr<_Ty> _Ret;
        (void)_Ret._Construct_from_weak(*this);
        return _Ret;
    }
};
#if _HAS_CXX17
template <class _Ty>
safeWeak_ptr(safeShared_ptr<_Ty>) -> safeWeak_ptr<_Ty>;
#endif // _HAS_CXX17

_EXPORT_STD template <class _Ty>
void swap(safeWeak_ptr<_Ty>& _Left, safeWeak_ptr<_Ty>& _Right) noexcept {
    _Left.swap(_Right);
}

_STD_END

// TRANSITION, non-_Ugly attribute tokens
#pragma pop_macro("msvc")

#pragma pop_macro("new")
_STL_RESTORE_CLANG_WARNINGS
#pragma warning(pop)
#pragma pack(pop)
#endif // _STL_COMPILER_PREPROCESSOR
#endif // _MEMORY_
