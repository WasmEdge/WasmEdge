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

#[repr(i8)]
pub enum Sign {
    Minus = -2,
    Zero = 0,
    Plus = 1,
}

#[repr(i16)]
pub enum Tagged {
    Low(u32) = -3,
    High(u32) = 5,
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

#[inline(never)]
fn weigh(s: &Sign, t: &Tagged) -> u32 {
    let v = match t {
        Tagged::Low(v) => *v,
        Tagged::High(v) => *v,
    };
    match s {
        Sign::Minus | Sign::Plus => v,
        Sign::Zero => 0,
    }
}

#[no_mangle]
pub extern "C" fn pick() -> u32 {
    let s = Shape::Circle(2);
    let t = Shape::Square { side: 1 };
    let e = Shape::Empty;
    area(&s) + area(&t) + area(&e) + weigh(&Sign::Zero, &Tagged::Low(1))
}
