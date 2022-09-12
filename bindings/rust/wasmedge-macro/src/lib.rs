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
        2 => quote!(
            fn #wrapper_fn_name_ident (#wrapper_fn_inputs) #wrapper_fn_return {
                // define inner function
                fn #inner_fn_name_ident (#inner_fn_inputs) #inner_fn_return {
                    #inner_fn_block
                }

                // create a Caller instance
                let caller = Caller::new(frame);

                #inner_fn_name_ident(&caller, args)
            }
        ),
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
                                            Some(arg) => {
                                                match arg {
                                                    syn::GenericArgument::Type(ty) => match ty {
                                                        syn::Type::Reference(
                                                            syn::TypeReference { ref elem, .. },
                                                        ) => syn::TypePtr {
                                                            star_token: parse_quote!(*),
                                                            const_token: None,
                                                            mutability: Some(parse_quote!(mut)),
                                                            elem: elem.clone(),
                                                        },
                                                        _ => panic!(
                                                "wrong wrong wrong wrong wrong wrong wrong wrong"
                                            ),
                                                    },
                                                    _ => {
                                                        panic!("wrong wrong wrong wrong wrong wrong wrong")
                                                    }
                                                }
                                            }
                                            None => panic!("wrong wrong wrong wrong wrong wrong"),
                                        }
                                    }
                                    _ => panic!("wrong wrong wrong wrong wrong"),
                                },
                                false => panic!("wrong wrong wrong wrong"),
                            }
                        }
                        None => panic!("wrong wrong wrong"),
                    },
                    _ => panic!("wrong wrong"),
                },
                _ => panic!("wrong"),
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
    let mut fn_inputs = item_fn.sig.inputs.clone();
    let data_arg = fn_inputs.last_mut().unwrap();
    let ty_ptr = if let FnArg::Typed(PatType { ref mut ty, .. }) = data_arg {
        let boxed_ty = if let syn::Type::Reference(syn::TypeReference { ref mut elem, .. }) = **ty {
            elem.clone()
        } else {
            panic!("wrong")
        };

        // *mut Vec<&str>
        let ty_ptr = syn::TypePtr {
            star_token: parse_quote!(*),
            const_token: None,
            mutability: Some(parse_quote!(mut)),
            elem: boxed_ty,
        };

        // let cloned_ty = ty.clone();

        **ty = parse_quote!(*mut std::os::raw::c_void); //syn::Type::Ptr(ty_ptr);

        ty_ptr
    } else {
        panic!("wrong wrong")
    };

    let fn_name_ident = &item_fn.sig.ident;
    let fn_return = &item_fn.sig.output;
    let mut fn_block = item_fn.block.clone();
    let stmts = &mut fn_block.stmts;
    // stmts.insert(
    //     0,
    //     parse_quote!(let data = unsafe { &mut *(data as #ty_ptr) };),
    // );
    stmts.insert(
        0,
        parse_quote!(let data = match data.is_null() {
            true => None,
            false => {
                let data = unsafe { &mut *(data as #ty_ptr) };
                Some(data)
            }
        };),
    );

    let ret = quote!(
        fn #fn_name_ident (#fn_inputs) #fn_return
            // #new_tokens
            #fn_block

    );

    Ok(ret)
}
