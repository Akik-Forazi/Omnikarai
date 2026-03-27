package main
import (
	"fmt"
	"os"
)
func fib(n int) int64 {
	if n <= 1 { return int64(n) }
	return fib(n-1) + fib(n-2)
}
func main() {
	var result int64
	for r := 0; r < 5; r++ { result = fib(40) }
	fmt.Println(result)
	os.Exit(0)
}
