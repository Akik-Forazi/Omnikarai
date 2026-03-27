// bench_loop.java — Iterative Loop Counter
// Compile: javac bench_loop.java
// Run    : java bench_loop
public class bench_loop {
    public static void main(String[] args) {
        long total = 0;
        for (long i = 1; i <= 10000000L; i++)
            total += i;
        System.out.println(total);
    }
}
