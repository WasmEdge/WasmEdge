pub mod signatures;

pub fn encode<S: AsRef<str>>(s: S) -> Vec<u8> {
    let length = s.as_ref().len() / 2;
    let mut res = Vec::with_capacity(length);
    for i in 0..length {
        let code = &s.as_ref()[i * 2..i * 2 + 2];
        res.push(u8::from_str_radix(code, 16).expect("Unable to cast hex"));
    }
    res
}

pub fn decode(s: Vec<u8>) -> String {
    s.iter()
        .map(|n| format!("{:X}", n))
        .collect::<Vec<String>>()
        .join("")
}
