// bench_dotprod.go — AI: Dot Product
// Compile: go build -o bench_dotprod_go.exe bench_dotprod.go
package main

import ("fmt"; "os")

func dotprod(n int) int64 {
	var s int64
	for i := 0; i < n; i++ {
		s += int64(i) * int64(n-i)
	}
	return s
}

func main() {
	var result int64
	for rep := 0; rep < 10000; rep++ {
		result = dotprod(1024)
	}
	fmt.Println(result)
	os.Exit(0)
}
