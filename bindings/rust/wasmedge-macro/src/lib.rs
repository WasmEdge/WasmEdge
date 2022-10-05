#![doc(
    html_logo_url = "https://github.com/cncf/artwork/blob/master/projects/wasm-edge-runtime/icon/color/wasm-edge-runtime-icon-color.png?raw=true",
    html_favicon_url = "https://raw.githubusercontent.com/cncf/artwork/49169bdbc88a7ce3c4a722c641cc2d548bd5c340/projects/wasm-edge-runtime/icon/color/wasm-edge-runtime-icon-color.svg"
)]

//! # Overview
//! The [wasmedge-macro](https://crates.io/crates/wasmedge-macro) crate defines a group of procedural macros used by both [wasmedge-sdk](https://crates.io/crates/wasmedge-sdk) and [wasmedge-sys](https://crates.io/crates/wasmedge-sys) crates.

#![feature(extend_one)]

use proc_macro::TokenStream;
use quote::quote;
use syn::{parse_macro_input, parse_quote, spanned::Spanned, FnArg, Item, Pat, PatType};

/// Expand a synchronous host function defined with `wasmedge-sdk` crate.
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
        frame: &wasmedge_sys::CallingFrame,
        args: Vec<wasmedge_sys::WasmValue>,
        data: *mut std::os::raw::c_void
    );
    // return type of wrapper function
    let wrapper_fn_return = item_fn.sig.output.clone();

    // * define the signature of inner function
    // name of inner function
    let inner_fn_name_literal = format!("inner_{}", wrapper_fn_name_literal);
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
                fn #wrapper_fn_name_ident (#wrapper_fn_inputs) #wrapper_fn_return {
                    // define inner function
                    fn #inner_fn_name_ident (#inner_fn_inputs) #inner_fn_return {
                        #inner_fn_block
                    }

                    // create a Caller instance
                    let caller = Caller::new(frame);

                    #inner_fn_name_ident(&caller, args)
                }
            )
        }
        3 => {
            let data_arg = item_fn.sig.inputs.last().unwrap().clone();
            let ty_ptr = match &data_arg {
                FnArg::Typed(PatType { ref ty, .. }) => match **ty {
                    syn::Type::Reference(syn::TypeReference { ref elem, .. }) => syn::TypePtr {
                        star_token: parse_quote!(*),
                        const_token: None,
                        mutability: Some(parse_quote!(mut)),
                        elem: elem.clone(),
                    },
                    syn::Type::Path(syn::TypePath { ref path, .. }) => match path.segments.last() {
                        Some(segment) => {
                            let id = segment.ident.to_string();
                            match id == "Option" {
                                true => match segment.arguments {
                                    syn::PathArguments::AngleBracketed(
                                        syn::AngleBracketedGenericArguments { ref args, .. },
                                    ) => {
                                        let last_generic_arg = args.last();
                                        match last_generic_arg {
                                            Some(arg) => match arg {
                                                syn::GenericArgument::Type(ty) => match ty {
                                                    syn::Type::Reference(syn::TypeReference {
                                                        ref elem,
                                                        ..
                                                    }) => syn::TypePtr {
                                                        star_token: parse_quote!(*),
                                                        const_token: None,
                                                        mutability: Some(parse_quote!(mut)),
                                                        elem: elem.clone(),
                                                    },
                                                    _ => panic!("Not found syn::Type::Reference"),
                                                },
                                                _ => {
                                                    panic!("Not found syn::GenericArgument::Type")
                                                }
                                            },
                                            None => panic!("Not found the last GenericArgument"),
                                        }
                                    }
                                    _ => panic!("Not found syn::PathArguments::AngleBracketed"),
                                },
                                false => panic!("Not found segment ident: Option"),
                            }
                        }
                        None => panic!("Not found path segments"),
                    },
                    _ => panic!("Unsupported syn::Type type"),
                },
                _ => panic!("Unsupported syn::FnArg type"),
            };

            // generate token stream
            quote!(
                fn #wrapper_fn_name_ident (#wrapper_fn_inputs) #wrapper_fn_return {
                    // define inner function
                    fn #inner_fn_name_ident (#inner_fn_inputs) #inner_fn_return {
                        #inner_fn_block
                    }

                    // create a Caller instance
                    let caller = Caller::new(frame);

                    let data = unsafe { &mut *(data as #ty_ptr) };

                    #inner_fn_name_ident(&caller, args, data)
                }
            )
        }
        _ => panic!("Invalid numbers of host function arguments"),
    };

    Ok(ret)
}

#[doc(hidden)]
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

#[doc(hidden)]
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

#[doc(hidden)]
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

/// Expand a synchronous host function defined with `wasmedge-sys` crate.
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

    // * define the signature of inner function
    // name of inner function
    let inner_fn_name_literal = format!("inner_{}", wrapper_fn_name_literal);
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
            let mut wrapper_fn_inputs = item_fn.sig.inputs.clone();
            wrapper_fn_inputs.push(parse_quote!(_data: *mut std::os::raw::c_void));

            quote!(
                fn #wrapper_fn_name_ident (#wrapper_fn_inputs) #wrapper_fn_return {
                    // define inner function
                    fn #inner_fn_name_ident (#inner_fn_inputs) #inner_fn_return {
                        #inner_fn_block
                    }

                    #inner_fn_name_ident(&#arg1, #arg2)
                }
            )
        }
        3 => {
            let data_arg = item_fn.sig.inputs.last().unwrap().clone();
            let ty_ptr = match &data_arg {
                FnArg::Typed(PatType { ref ty, .. }) => match **ty {
                    syn::Type::Reference(syn::TypeReference { ref elem, .. }) => syn::TypePtr {
                        star_token: parse_quote!(*),
                        const_token: None,
                        mutability: Some(parse_quote!(mut)),
                        elem: elem.clone(),
                    },
                    syn::Type::Path(syn::TypePath { ref path, .. }) => match path.segments.last() {
                        Some(segment) => {
                            let id = segment.ident.to_string();
                            match id == "Option" {
                                true => match segment.arguments {
                                    syn::PathArguments::AngleBracketed(
                                        syn::AngleBracketedGenericArguments { ref args, .. },
                                    ) => {
                                        let last_generic_arg = args.last();
                                        match last_generic_arg {
                                            Some(arg) => match arg {
                                                syn::GenericArgument::Type(ty) => match ty {
                                                    syn::Type::Reference(syn::TypeReference {
                                                        ref elem,
                                                        ..
                                                    }) => syn::TypePtr {
                                                        star_token: parse_quote!(*),
                                                        const_token: None,
                                                        mutability: Some(parse_quote!(mut)),
                                                        elem: elem.clone(),
                                                    },
                                                    _ => panic!("Not found syn::Type::Reference"),
                                                },
                                                _ => {
                                                    panic!("Not found syn::GenericArgument::Type")
                                                }
                                            },
                                            None => panic!("Not found the last GenericArgument"),
                                        }
                                    }
                                    _ => panic!("Not found syn::PathArguments::AngleBracketed"),
                                },
                                false => panic!("Not found segment ident: Option"),
                            }
                        }
                        None => panic!("Not found path segments"),
                    },
                    _ => panic!("Unsupported syn::Type type"),
                },
                _ => panic!("Unsupported syn::FnArg type"),
            };

            // inputs of wrapper function
            let mut wrapper_fn_inputs = item_fn.sig.inputs.clone();
            wrapper_fn_inputs.pop();
            wrapper_fn_inputs.push(parse_quote!(data: *mut std::os::raw::c_void));

            // generate token stream
            quote!(
                fn #wrapper_fn_name_ident (#wrapper_fn_inputs) #wrapper_fn_return {
                    // define inner function
                    fn #inner_fn_name_ident (#inner_fn_inputs) #inner_fn_return {
                        #inner_fn_block
                    }

                    let data = unsafe { &mut *(data as #ty_ptr) };

                    #inner_fn_name_ident(&#arg1, #arg2, data)
                }
            )
        }
        _ => panic!("Invalid numbers of host function arguments"),
    };

    Ok(ret)
}
