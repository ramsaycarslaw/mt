use std::time::Instant;

pub fn fib(n: i32) -> u64 {
	if n < 0 {
		panic!("{} is negative!", n);
	} else if n == 0 {
		panic!("zero is not a right argument to fibonacci()!");
	} else if n == 1 {
		return 1;
	}

	let mut sum = 0;
	let mut last = 0;
	let mut curr = 1;
	for _i in 1..n {
		sum = last + curr;
		last = curr;
		curr = sum;
	}
	sum
}

fn main() {
    let start = Instant::now();
    let answer = fib(50);
    let elapsed = start.elapsed().as_secs();

    println!("Found answer {}", answer);
    println!("Elapsed: {}", elapsed);
}
