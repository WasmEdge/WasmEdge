// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#![no_std]

#[panic_handler]
fn panic(_: &core::panic::PanicInfo) -> ! {
    loop {}
}

pub enum Shape {
    Circle(u32),
    Square { side: u32 },
    Empty,
}

static mut CALLS: u32 = 0;

#[inline(never)]
fn area(s: &Shape) -> u32 {
    unsafe {
        CALLS += 1;
    }
    match s {
        Shape::Circle(r) => r * 3,
        Shape::Square { side } => side * side,
        Shape::Empty => 0,
    }
}

#[no_mangle]
pub extern "C" fn pick() -> u32 {
    let s = Shape::Circle(2);
    let t = Shape::Square { side: 1 };
    let e = Shape::Empty;
    area(&s) + area(&t) + area(&e)
}
