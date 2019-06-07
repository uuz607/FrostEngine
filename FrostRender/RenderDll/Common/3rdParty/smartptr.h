
#ifndef _SMART_PTR_H_
#define _SMART_PTR_H_

//! Smart Pointer.
template<class _I> class _smart_ptr
{
public:
	_smart_ptr() : p(NULL) {}
	_smart_ptr(_I* p_)
	{
		p = p_;
		if (p)
			p->AddRef();
	}
	
	_smart_ptr(const _smart_ptr& p_)
	{
		p = p_.p;
		if (p)
			p->AddRef();
	}

	_smart_ptr(_smart_ptr&& p_) noexcept
	{
		p = p_.p;
		p_.p = nullptr;
	}

	template<typename _Y>
	_smart_ptr(const _smart_ptr<_Y>& p_)
	{
		p = p_.get();
		if (p)
			p->AddRef();
	}

	~_smart_ptr()
	{
		if (p)
			p->Release();
	}
	operator _I*() const { return p; }

	_I&         operator*() const { return *p; }
 	_I*         operator->(void) const { return p; }
	_I*         get() const { return p; }
	
	_smart_ptr& operator=(_I* newp)
	{
		if (newp)
			newp->AddRef();
		if (p)
			p->Release();
		p = newp;
		return *this;
	}

	_I** operator&()
	{
		return &p;
	}

	void reset()
	{
		_smart_ptr<_I>().swap(*this);
	}

	void reset(_I* p)
	{
		if (p != this->p)
		{
			_smart_ptr<_I>(p).swap(*this);
		}
	}

	_smart_ptr& operator=(const _smart_ptr& newp)
	{
		if (newp.p)
			newp.p->AddRef();
		if (p)
			p->Release();
		p = newp.p;
		return *this;
	}

	template<typename _Y>
	_smart_ptr& operator=(const _smart_ptr<_Y>& newp)
	{
		_I* const p2 = newp.get();
		if (p2)
			p2->AddRef();
		if (p)
			p->Release();
		p = p2;
		return *this;
	}

	void swap(_smart_ptr<_I>& other)
	{
		std::swap(p, other.p);
	}

	//! Assigns a pointer without increasing ref count.
	void Assign_NoAddRef(_I* ptr)
	{
		FROST_ASSERT_MESSAGE(!p, "Assign_NoAddRef should only be used on a default-constructed, not-yet-assigned smart_ptr instance");
		p = ptr;
	}

	//! Set our contained pointer to null, but don't call Release().
	//! Useful for when we want to do that ourselves, or when we know that
	//! the contained pointer is invalid.
	_I* ReleaseOwnership()
	{
		_I* ret = p;
		p = 0;
		return ret;
	}

private:
	_I * p;
};

template<typename T>
void swap(_smart_ptr<T>& a, _smart_ptr<T>& b)
{
	a.swap(b);
}

class CMultiThreadRefCount
{
public:
	CMultiThreadRefCount() : m_nCount(0) {}
	virtual ~CMultiThreadRefCount() {}

	void AddRef()
	{
		::_InterlockedIncrement(&m_nCount);
	}

	void Release()
	{
		const int count = ::_InterlockedDecrement(&m_nCount);
		assert(count >= 0);
		if (count == 0)
		{
			DeleteThis();
		}
		else if (count < 0)
		{
			assert(0);
		}
	}

	bool Unique() const
	{
		return m_nCount == 1 ? true : false;
	}
protected:
	// Allows the memory for the object to be deallocated in the dynamic module where it was originally constructed, as it may use different memory manager (Debug/Release configurations)
	virtual void DeleteThis() { delete this; }

private:
	// Private: Discourage inheriting classes to observe m_nRefCounter and act on its value which is not thread safe.
	volatile long m_nCount; 
};


#endif //_SMART_PTR_H_
