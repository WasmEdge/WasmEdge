package org.wasmedge;

// In fact, it will use template
public class Async<T>{
    private long pointer;

    @SafeVarargs
    Async(T... args){
        nativeInit(args);
    }
    @SafeVarargs
    private native void nativeInit(T... args);
    // Async() noexcept = default;
    /*
    template <typename... FArgsT, typename... ArgsT>
    Async(T (VM::*FPtr)(FArgsT...), VM &TargetVM, ArgsT &&...Args)
            : VMPtr(&TargetVM) {
        std::promise<T> Promise;
        Future = Promise.get_future();
        Thread =
                std::thread([FPtr, P = std::move(Promise),
                Tuple = std::tuple(
                &TargetVM, std::forward<ArgsT>(Args)...)]() mutable {
            std::get<0>(Tuple)->newThread();
            P.set_value(std::apply(FPtr, Tuple));
        });
        Thread.detach();
    }
    Async(const Async &) noexcept = delete;
    Async(Async &&Other) noexcept : Async() { swap(*this, Other); }
    Async &operator=(const Async &) = delete;
    Async &operator=(Async &&Other) noexcept {
        swap(*this, Other);
        return *this;
    }
    */

    public native Boolean valid();

    // T
    public native T get();

    public native void asyncWait();

    // const std::chrono::duration<RT, PT> &Timeout
    public native Boolean waitFor(Object timeout);

    //const std::chrono::time_point<CT, DT> &Timeout
    public native Boolean waitUntil(Object timeout);

    public native void swap(Async lhs, Async rhs);

    public native void cancel();

}