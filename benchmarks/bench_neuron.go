// bench_neuron.go — AI: ReLU Neuron Activation Layer
// Compile: go build -o bench_neuron_go.exe bench_neuron.go
package main

import ("fmt"; "os")

func relu(x int64) int64 {
	if x > 0 { return x }
	return 0
}

func layer(inputs, neurons int) int64 {
	var total int64
	for n := 0; n < neurons; n++ {
		var acc int64
		for i := 0; i < inputs; i++ {
			w := (int64(n)*int64(inputs) + int64(i) + 1) % 11
			x := int64(i-inputs/2) * w
			acc += relu(x)
		}
		total += acc
	}
	return total
}

func main() {
	var result int64
	for rep := 0; rep < 2000; rep++ {
		result = layer(128, 256)
	}
	fmt.Println(result)
	os.Exit(0)
}
