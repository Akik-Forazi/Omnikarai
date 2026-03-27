// bench_matmul.go — AI: Matrix Multiply
// Compile: go build -o bench_matmul_go.exe bench_matmul.go
package main

import ("fmt"; "os")

func matmulSum(n int) int64 {
	var total int64
	for i := 0; i < n; i++ {
		for j := 0; j < n; j++ {
			var acc int64
			for k := 0; k < n; k++ {
				a := int64((i+k)%17)
				b := int64((k*j+1)%13)
				acc += a * b
			}
			total += acc
		}
	}
	return total
}

func main() {
	var result int64
	for rep := 0; rep < 5000; rep++ {
		result = matmulSum(32)
	}
	fmt.Println(result)
	os.Exit(0)
}
