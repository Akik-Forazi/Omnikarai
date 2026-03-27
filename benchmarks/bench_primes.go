package main
import ("fmt"; "os")
func is_prime(n int) bool {
	if n < 2 { return false }
	for d := 2; d*d <= n; d++ { if n%d == 0 { return false } }
	return true
}
func main() {
	result := 0
	for r := 0; r < 30; r++ {
		count := 0
		for n := 2; n <= 100000; n++ { if is_prime(n) { count++ } }
		result = count
	}
	fmt.Println(result)
	os.Exit(0)
}
