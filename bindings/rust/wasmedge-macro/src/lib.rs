#![doc(
    html_logo_url = "https://github.com/cncf/artwork/blob/master/projects/wasm-edge-runtime/icon/color/wasm-edge-runtime-icon-color.png?raw=true",
    html_favicon_url = "https://raw.githubusercontent.com/cncf/artwork/49169bdbc88a7ce3c4a722c641cc2d548bd5c340/projects/wasm-edge-runtime/icon/color/wasm-edge-runtime-icon-color.svg"
)]

//! # Overview
//! The [wasmedge-macro](https://crates.io/crates/wasmedge-macro) crate defines a group of procedural macros used by both [wasmedge-sdk](https://crates.io/crates/wasmedge-sdk) and [wasmedge-sys](https://crates.io/crates/wasmedge-sys) crates.

use proc_macro::TokenStream;
use quote::quote;
use syn::{parse_macro_input, parse_quote, spanned::Spanned, FnArg, Item, Pat, PatIdent, PatType};

/// Declare a native function that will be used to create a host function instance.
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
    // * define the signature of wrapper function
    // name of wrapper function
    let wrapper_fn_name_ident = item_fn.sig.ident.clone();
    let wrapper_fn_name_literal = wrapper_fn_name_ident.to_string();
    // arguments of wrapper function
    let wrapper_fn_inputs: syn::punctuated::Punctuated<FnArg, syn::token::Comma> = parse_quote!(
        frame: wasmedge_sdk::CallingFrame,
        args: Vec<wasmedge_sdk::WasmValue>,
    );
    // return type of wrapper function
    let wrapper_fn_return = item_fn.sig.output.clone();
    // visibility of wrapper function
    let wrapper_visibility = item_fn.vis.clone();

    // * define the signature of inner function
    // name of inner function
    let inner_fn_name_literal = format!("inner_{wrapper_fn_name_literal}");
    let inner_fn_name_ident = syn::Ident::new(&inner_fn_name_literal, item_fn.sig.span());
    // arguments of inner function
    let inner_fn_inputs = item_fn.sig.inputs.clone();
    // return type of inner function
    let inner_fn_return = item_fn.sig.output.clone();
    // body of inner function
    let inner_fn_block = item_fn.block.clone();

    // extract T from Option<&mut T>
    let ret = match item_fn.sig.inputs.len() {
        2 => {
            quote!(
                # wrapper_visibility fn #wrapper_fn_name_ident (#wrapper_fn_inputs) #wrapper_fn_return {
                    // define inner function
                    fn #inner_fn_name_ident (#inner_fn_inputs) #inner_fn_return {
                        #inner_fn_block
                    }

                    // create a Caller instance
                    let caller = Caller::new(frame);

                    #inner_fn_name_ident(caller, args)
                }
            )
        }
        _ => panic!("Invalid numbers of host function arguments"),
    };

    Ok(ret)
}

#[doc(hidden)]
#[proc_macro_attribute]
pub fn async_host_function_original(_attr: TokenStream, item: TokenStream) -> TokenStream {
    let body_ast = parse_macro_input!(item as Item);
    if let Item::Fn(item_fn) = body_ast {
        match expand_async_host_func_original(&item_fn) {
            Ok(token_stream) => token_stream.into(),
            Err(err) => err.to_compile_error().into(),
        }
    } else {
        TokenStream::new()
    }
}

fn expand_async_host_func_original(item_fn: &syn::ItemFn) -> syn::Result<proc_macro2::TokenStream> {
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
    let inner_fn_name_literal = format!("inner_{outer_fn_name_literal}");
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

#[doc(hidden)]
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
    let inner_fn_name_literal = format!("inner_{outer_fn_name_literal}");
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

/// Declare a native async function that will be used to create an async host function instance.
#[proc_macro_attribute]
pub fn async_host_function(_attr: TokenStream, item: TokenStream) -> TokenStream {
    let body_ast = parse_macro_input!(item as Item);
    if let Item::Fn(item_fn) = body_ast {
        match expand_async_host_func(&item_fn) {
            Ok(token_stream) => token_stream.into(),
            Err(err) => err.to_compile_error().into(),
        }
    } else {
        TokenStream::new()
    }
}

fn expand_async_host_func(item_fn: &syn::ItemFn) -> syn::Result<proc_macro2::TokenStream> {
    // extract T from Option<&mut T>
    let ret = match &item_fn.sig.inputs.len() {
        2 => expand_async_host_func_with_two_args(item_fn),
        _ => panic!("Invalid numbers of host function arguments"),
    };

    Ok(ret)
}

fn expand_async_host_func_with_two_args(item_fn: &syn::ItemFn) -> proc_macro2::TokenStream {
    let fn_name_ident = &item_fn.sig.ident;
    let fn_visibility = &item_fn.vis;

    // * prepare the function arguments
    let mut fn_inputs = item_fn.sig.inputs.clone();
    // process the first argument
    let mut original_first_arg_ident: syn::Ident =
        syn::Ident::new("placeholder", proc_macro2::Span::call_site());
    if let Some(first) = fn_inputs.first_mut() {
        if let FnArg::Typed(PatType { pat, .. }) = first {
            if let Pat::Ident(PatIdent { ident, .. }) = &**pat {
                // extract the name from the first argument
                original_first_arg_ident = ident.clone();
            }
        }

        // replace the first argument
        *first = parse_quote!(frame: wasmedge_sdk::CallingFrame);
    }

    // * prepare the function body
    let fn_block = &item_fn.block;

    // compose the final function
    quote!(
        #fn_visibility fn #fn_name_ident (#fn_inputs) -> Box<(dyn std::future::Future<Output = Result<Vec<WasmValue>, HostFuncError>> + Send + 'static)> {
            Box::new(async move {
                let #original_first_arg_ident = Caller::new(frame);
                #fn_block
            })
        }
    )
}

#[doc(hidden)]
#[proc_macro_attribute]
pub fn sys_async_host_function_original(_attr: TokenStream, item: TokenStream) -> TokenStream {
    let body_ast = parse_macro_input!(item as Item);
    if let Item::Fn(item_fn) = body_ast {
        match sys_async_expand_host_func_original(&item_fn) {
            Ok(token_stream) => token_stream.into(),
            Err(err) => err.to_compile_error().into(),
        }
    } else {
        TokenStream::new()
    }
}

fn sys_async_expand_host_func_original(
    item_fn: &syn::ItemFn,
) -> syn::Result<proc_macro2::TokenStream> {
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
    let inner_fn_name_literal = format!("inner_{outer_fn_name_literal}");
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

#[doc(hidden)]
#[proc_macro_attribute]
pub fn sys_async_host_function_switcher2(_attr: TokenStream, item: TokenStream) -> TokenStream {
    let body_ast = parse_macro_input!(item as Item);
    if let Item::Fn(item_fn) = body_ast {
        match sys_expand_async_host_func_switcher2(&item_fn) {
            Ok(token_stream) => token_stream.into(),
            Err(err) => err.to_compile_error().into(),
        }
    } else {
        TokenStream::new()
    }
}

fn sys_expand_async_host_func_switcher2(
    item_fn: &syn::ItemFn,
) -> syn::Result<proc_macro2::TokenStream> {
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
    let inner_fn_name_literal = format!("inner_{outer_fn_name_literal}");
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
pub fn sys_host_function(_attr: TokenStream, item: TokenStream) -> TokenStream {
    let body_ast = parse_macro_input!(item as Item);
    if let Item::Fn(item_fn) = body_ast {
        match sys_expand_host_func(&item_fn) {
            Ok(token_stream) => token_stream.into(),
            Err(err) => err.to_compile_error().into(),
        }
    } else {
        TokenStream::new()
    }
}

fn sys_expand_host_func(item_fn: &syn::ItemFn) -> syn::Result<proc_macro2::TokenStream> {
    // * define the signature of wrapper function
    // name of wrapper function
    let wrapper_fn_name_ident = item_fn.sig.ident.clone();
    let wrapper_fn_name_literal = wrapper_fn_name_ident.to_string();
    // return type of wrapper function
    let wrapper_fn_return = item_fn.sig.output.clone();
    // visiblity of wrapper function
    let wrapper_fn_visibility = item_fn.vis.clone();

    // * define the signature of inner function
    // name of inner function
    let inner_fn_name_literal = format!("inner_{wrapper_fn_name_literal}");
    let inner_fn_name_ident = syn::Ident::new(&inner_fn_name_literal, item_fn.sig.span());
    // arguments of inner function
    let inner_fn_inputs = item_fn.sig.inputs.clone();
    // return type of inner function
    let inner_fn_return = item_fn.sig.output.clone();
    // body of inner function
    let inner_fn_block = item_fn.block.clone();

    // extract the identities of the first two arguments
    let arg1 = match &item_fn.sig.inputs[0] {
        FnArg::Typed(PatType { pat, .. }) => match &**pat {
            Pat::Ident(pat_ident) => pat_ident.ident.clone(),
            Pat::Wild(_) => proc_macro2::Ident::new("_", proc_macro2::Span::call_site()),
            _ => panic!("argument pattern is not a simple ident"),
        },
        FnArg::Receiver(_) => panic!("argument is a receiver"),
    };
    let arg2 = match &item_fn.sig.inputs[1] {
        FnArg::Typed(PatType { pat, .. }) => match &**pat {
            Pat::Ident(pat_ident) => pat_ident.ident.clone(),
            Pat::Wild(_) => proc_macro2::Ident::new("_", proc_macro2::Span::call_site()),
            _ => panic!("argument pattern is not a simple ident"),
        },
        FnArg::Receiver(_) => panic!("argument is a receiver"),
    };

    // extract T from Option<&mut T>
    let ret = match item_fn.sig.inputs.len() {
        2 => {
            // insert the third argument
            let wrapper_fn_inputs = item_fn.sig.inputs.clone();

            quote!(
                #wrapper_fn_visibility fn #wrapper_fn_name_ident (#wrapper_fn_inputs) #wrapper_fn_return {
                    // define inner function
                    fn #inner_fn_name_ident (#inner_fn_inputs) #inner_fn_return {
                        #inner_fn_block
                    }

                    #inner_fn_name_ident(#arg1, #arg2)
                }
            )
        }
        _ => panic!("Invalid numbers of host function arguments"),
    };

    Ok(ret)
}

#[proc_macro_attribute]
pub fn sys_async_host_function(_attr: TokenStream, item: TokenStream) -> TokenStream {
    let body_ast = parse_macro_input!(item as Item);
    if let Item::Fn(item_fn) = body_ast {
        match sys_expand_async_host_func(&item_fn) {
            Ok(token_stream) => token_stream.into(),
            Err(err) => err.to_compile_error().into(),
        }
    } else {
        TokenStream::new()
    }
}

fn sys_expand_async_host_func(item_fn: &syn::ItemFn) -> syn::Result<proc_macro2::TokenStream> {
    // extract T from Option<&mut T>
    let ret = match &item_fn.sig.inputs.len() {
        2 => sys_expand_async_host_func_with_two_args(item_fn),
        _ => panic!("Invalid numbers of host function arguments"),
    };

    Ok(ret)
}

fn sys_expand_async_host_func_with_two_args(item_fn: &syn::ItemFn) -> proc_macro2::TokenStream {
    let fn_name_ident = &item_fn.sig.ident;
    let fn_visibility = &item_fn.vis;
    let fn_inputs = item_fn.sig.inputs.clone();

    let fn_block = &item_fn.block;

    let ret = quote!(
        #fn_visibility fn #fn_name_ident (#fn_inputs) -> Box<(dyn std::future::Future<Output = Result<Vec<WasmValue>, HostFuncError>> + Send + 'static)> {
            Box::new(async move {
                #fn_block
            })
        }
    );

    ret
}
