package main
import ("fmt"; "os")
func main() {
	var s int64
	for i := int64(1); i <= 100000000; i++ { s += i }
	fmt.Println(s)
	os.Exit(0)
}
