#pragma once

// =============================================================================
// deferred_call:
// --------------
// This struct enables us to implement deferred function calls simply in
// the defer() function below.  It forces a given function to automatically
// be called at the end of scope using move-only semantics.  Most
// commonly, the given function will be a lambda but that is not required.
// See the defer() function (below) for more on this
// =============================================================================
template <typename FUNC>
struct deferred_call
{
    // Disallow assignment and copy


    deferred_call(const deferred_call& that) = delete;
    deferred_call& operator=(const deferred_call& that) = delete;

    // Pass in a lambda

    deferred_call(FUNC&& f) 
        : m_func(std::forward<FUNC>(f)), m_bOwner(true) 
    {
    }

    // Move constructor, since we disallow the copy

    deferred_call(deferred_call&& that)
        : m_func(std::move(that.m_func)), m_bOwner(that.m_bOwner)
    {
        that.m_bOwner = false;
    }

    // Destructor forces deferred call to be executed

    ~deferred_call()
    {
        execute();
    }

    // Prevent the deferred call from ever being invoked

    bool cancel()
    {
        bool bWasOwner = m_bOwner;
        m_bOwner = false;
        return bWasOwner;
    }

    // Cause the deferred call to be invoked NOW

    bool execute()
    {
        const auto bWasOwner = m_bOwner;

        if (m_bOwner)
        {
            m_bOwner = false;
            m_func();
        }

        return bWasOwner;
    }

private:
    FUNC m_func;
    bool m_bOwner;
};


// -----------------------------------------------------------------------------
// defer:  Generic, deferred function calls
// ----------------------------------------
//      This function template the user the ability to easily set up any 
//      arbitrary  function to be called *automatically* at the end of 
//      the current scope, even if return is called or an exception is 
//      thrown.  This is sort of a fire-and-forget.  Saves you from having
//      to repeat the same code over and over or from having to add
//      exception blocks just to be sure that the given function is called.
//
//      If you wish, you may cancel the deferred call as well as force it
//      to be executed BEFORE the end of scope.
//
// Example:
//      void Foo()
//      {
//          auto callOnException  = defer([]{ SomeGlobalFunction(); });
//          auto callNoMatterWhat = defer([pObj](pObj->SomeMemberFunction(); });
//
//          // Do dangerous stuff that might throw an exception ...
//
//          ...
//          ... blah blah blah
//          ...
//
//          // Done with dangerous code.  We can now...
//          //      a) cancel either of the above calls (i.e. call cancel()) OR
//          //      b) force them to be executed (i.e. call execute()) OR
//          //      c) do nothing and they'll be executed at end of scope.
//
//          callOnException.cancel();    // no exception, prevent this from happening
//
//          // End of scope,  If we had not canceled or executed the two
//          // above objects, they'd both be executed now.
//      }
// -----------------------------------------------------------------------------

template <typename F>
deferred_call<F> defer(F&& f)
{
    return deferred_call<F>(std::forward<F>(f));
}