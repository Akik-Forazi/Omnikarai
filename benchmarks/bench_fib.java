// bench_fib.java — Recursive Fibonacci
// Compile: javac bench_fib.java
// Run    : java bench_fib
public class bench_fib {
    static long fib(int n) {
        if (n <= 1) return n;
        return fib(n - 1) + fib(n - 2);
    }
    public static void main(String[] args) {
        System.out.println(fib(35));
    }
}
