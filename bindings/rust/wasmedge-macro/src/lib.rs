use proc_macro::TokenStream;
use quote::quote;
use syn::{parse_macro_input, spanned::Spanned, FnArg, Item, Pat, PatType};

#[proc_macro_attribute]
pub fn host_function(_attr: TokenStream, item: TokenStream) -> TokenStream {
    let body_ast = parse_macro_input!(item as Item);
    if let Item::Fn(item_fn) = body_ast {
        match expand_host_func(&item_fn) {
            Ok(token_stream) => token_stream.into(),
            Err(err) => err.to_compile_error().into(),
        }
    } else {
        TokenStream::new()
    }
}

fn expand_host_func(item_fn: &syn::ItemFn) -> syn::Result<proc_macro2::TokenStream> {
    let outer_fn_name_ident = &item_fn.sig.ident;
    let outer_fn_name_literal = outer_fn_name_ident.to_string();
    let outer_fn_inputs = &item_fn.sig.inputs;
    let outer_fn_return = &item_fn.sig.output;
    let args = outer_fn_inputs.iter().map(|fn_arg| match fn_arg {
        FnArg::Typed(PatType { pat, .. }) => match &**pat {
            Pat::Ident(ident) => ident,
            _ => panic!("argument pattern is not a simple ident"),
        },
        FnArg::Receiver(_) => panic!("argument is a receiver"),
    });
    let inner_fn_name_literal = format!("inner_{}", outer_fn_name_literal);
    let inner_fn_name_ident = syn::Ident::new(&inner_fn_name_literal, item_fn.sig.span());
    let inner_fn_inputs = &item_fn.sig.inputs;
    let inner_fn_return = &item_fn.sig.output;
    let inner_fn_block = &item_fn.block;

    let ret = match &item_fn.sig.asyncness {
        Some(inner_asyncness) => {
            quote!(
                fn #outer_fn_name_ident (#outer_fn_inputs) -> std::pin::Pin<
                Box<dyn std::future::Future<Output = Result<Vec<WasmValue>, HostFuncError>> + '_>> {
                    #inner_asyncness fn #inner_fn_name_ident (#inner_fn_inputs) #inner_fn_return {
                        #inner_fn_block
                    }
                    Box::pin(#inner_fn_name_ident(#(#args),*))
                }
            )
        }
        None => {
            quote!(
                fn #outer_fn_name_ident (#outer_fn_inputs) #outer_fn_return {
                    fn #inner_fn_name_ident (#inner_fn_inputs) #inner_fn_return {
                        #inner_fn_block
                    }
                    #inner_fn_name_ident(#(#args),*)
                }
            )
        }
    };

    Ok(ret)
}

#[proc_macro_attribute]
pub fn async_host_function2(_attr: TokenStream, item: TokenStream) -> TokenStream {
    let body_ast = parse_macro_input!(item as Item);
    if let Item::Fn(item_fn) = body_ast {
        match expand_async_host_func2(&item_fn) {
            Ok(token_stream) => token_stream.into(),
            Err(err) => err.to_compile_error().into(),
        }
    } else {
        TokenStream::new()
    }
}

fn expand_async_host_func2(item_fn: &syn::ItemFn) -> syn::Result<proc_macro2::TokenStream> {
    let outer_fn_name_ident = &item_fn.sig.ident;
    let outer_fn_name_literal = outer_fn_name_ident.to_string();
    let outer_fn_inputs = &item_fn.sig.inputs;
    let args = outer_fn_inputs.iter().map(|fn_arg| match fn_arg {
        FnArg::Typed(PatType { pat, .. }) => match &**pat {
            Pat::Ident(ident) => ident,
            _ => panic!("argument pattern is not a simple ident"),
        },
        FnArg::Receiver(_) => panic!("argument is a receiver"),
    });
    let inner_fn_name_literal = format!("inner_{}", outer_fn_name_literal);
    let inner_fn_name_ident = syn::Ident::new(&inner_fn_name_literal, item_fn.sig.span());
    let inner_fn_inputs = &item_fn.sig.inputs;
    let inner_fn_return = &item_fn.sig.output;
    let inner_fn_block = &item_fn.block;

    let ret = quote!(
        fn #outer_fn_name_ident (#outer_fn_inputs) -> wasmedge_sys::WasmEdgeHostFuncFuture {
            async fn #inner_fn_name_ident (#inner_fn_inputs) #inner_fn_return {
                #inner_fn_block
            }

            let stack = switcher2::stack::EightMbStack::new().unwrap();
            AsyncWasmEdgeResult::<switcher2::stack::EightMbStack, wasmedge_sys::WasmEdgeHostFuncResult<Vec<wasmedge_sys::WasmValue>>, fn()>::new(
                stack,
                |mut yielder| -> Result<Vec<wasmedge_sys::WasmValue>, wasmedge_types::error::HostFuncError> {
                    yielder.async_suspend(#inner_fn_name_ident(#(#args),*))
                },
            )
            .unwrap()
        }
    );

    Ok(ret)
}

#[proc_macro_attribute]
pub fn sys_async_host_function(_attr: TokenStream, item: TokenStream) -> TokenStream {
    let body_ast = parse_macro_input!(item as Item);
    if let Item::Fn(item_fn) = body_ast {
        match sys_async_expand_host_func(&item_fn) {
            Ok(token_stream) => token_stream.into(),
            Err(err) => err.to_compile_error().into(),
        }
    } else {
        TokenStream::new()
    }
}

fn sys_async_expand_host_func(item_fn: &syn::ItemFn) -> syn::Result<proc_macro2::TokenStream> {
    let outer_fn_name_ident = &item_fn.sig.ident;
    let outer_fn_name_literal = outer_fn_name_ident.to_string();
    let outer_fn_inputs = &item_fn.sig.inputs;
    let outer_fn_return = &item_fn.sig.output;
    let args = outer_fn_inputs.iter().map(|fn_arg| match fn_arg {
        FnArg::Typed(PatType { pat, .. }) => match &**pat {
            Pat::Ident(ident) => ident,
            _ => panic!("argument pattern is not a simple ident"),
        },
        FnArg::Receiver(_) => panic!("argument is a receiver"),
    });
    let inner_fn_name_literal = format!("inner_{}", outer_fn_name_literal);
    let inner_fn_name_ident = syn::Ident::new(&inner_fn_name_literal, item_fn.sig.span());
    let inner_fn_inputs = &item_fn.sig.inputs;
    let inner_fn_return = &item_fn.sig.output;
    let inner_fn_block = &item_fn.block;

    let ret = match &item_fn.sig.asyncness {
        Some(inner_asyncness) => {
            quote!(
                fn #outer_fn_name_ident (#outer_fn_inputs) -> std::pin::Pin<
                Box<dyn std::future::Future<Output = Result<Vec<WasmValue>, HostFuncError>> + '_>> {
                    #inner_asyncness fn #inner_fn_name_ident (#inner_fn_inputs) #inner_fn_return {
                        #inner_fn_block
                    }
                    Box::pin(#inner_fn_name_ident(#(#args),*))
                }
            )
        }
        None => {
            quote!(
                fn #outer_fn_name_ident (#outer_fn_inputs) #outer_fn_return {
                    fn #inner_fn_name_ident (#inner_fn_inputs) #inner_fn_return {
                        #inner_fn_block
                    }
                    #inner_fn_name_ident(#(#args),*)
                }
            )
        }
    };

    Ok(ret)
}

#[proc_macro_attribute]
pub fn sys_async_host_function2(_attr: TokenStream, item: TokenStream) -> TokenStream {
    let body_ast = parse_macro_input!(item as Item);
    if let Item::Fn(item_fn) = body_ast {
        match sys_expand_async_host_func2(&item_fn) {
            Ok(token_stream) => token_stream.into(),
            Err(err) => err.to_compile_error().into(),
        }
    } else {
        TokenStream::new()
    }
}

fn sys_expand_async_host_func2(item_fn: &syn::ItemFn) -> syn::Result<proc_macro2::TokenStream> {
    let outer_fn_name_ident = &item_fn.sig.ident;
    let outer_fn_name_literal = outer_fn_name_ident.to_string();
    let outer_fn_inputs = &item_fn.sig.inputs;
    let args = outer_fn_inputs.iter().map(|fn_arg| match fn_arg {
        FnArg::Typed(PatType { pat, .. }) => match &**pat {
            Pat::Ident(ident) => ident,
            _ => panic!("argument pattern is not a simple ident"),
        },
        FnArg::Receiver(_) => panic!("argument is a receiver"),
    });
    let inner_fn_name_literal = format!("inner_{}", outer_fn_name_literal);
    let inner_fn_name_ident = syn::Ident::new(&inner_fn_name_literal, item_fn.sig.span());
    let inner_fn_inputs = &item_fn.sig.inputs;
    let inner_fn_return = &item_fn.sig.output;
    let inner_fn_block = &item_fn.block;

    let ret = quote!(
        fn #outer_fn_name_ident (#outer_fn_inputs) -> wasmedge_sys::WasmEdgeHostFuncFuture {
            async fn #inner_fn_name_ident (#inner_fn_inputs) #inner_fn_return {
                #inner_fn_block
            }

            let stack = switcher2::stack::EightMbStack::new().unwrap();
            AsyncWasmEdgeResult::<switcher2::stack::EightMbStack, wasmedge_sys::WasmEdgeHostFuncResult<Vec<wasmedge_sys::WasmValue>>, fn()>::new(
                stack,
                |mut yielder| -> Result<Vec<wasmedge_sys::WasmValue>, wasmedge_types::error::HostFuncError> {
                    yielder.async_suspend(#inner_fn_name_ident(#(#args),*))
                },
            )
            .unwrap()
        }
    );

    Ok(ret)
}
