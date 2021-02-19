package main

import (
	"fmt"
	"time"
)

func fib(n int) int {
	a := 0
	b := 1

	for i := 0; i < n; i++ {
		a, b = b, a+b
	}
	return a
}

func main() {
	start := time.Now()

	answer := fib(50)

	elapsed := time.Since(start)

	fmt.Printf("Found answer %d\nElapsed: ", answer)
	fmt.Println(elapsed.Seconds())
}
