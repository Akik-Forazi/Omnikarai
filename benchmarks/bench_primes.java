// bench_primes.java — Prime Counting (trial division)
// Compile: javac bench_primes.java
// Run    : java bench_primes
public class bench_primes {
    static boolean is_prime(int n) {
        if (n < 2) return false;
        for (int d = 2; (long)d * d <= n; d++)
            if (n % d == 0) return false;
        return true;
    }
    public static void main(String[] args) {
        int count = 0;
        for (int n = 2; n <= 100000; n++)
            if (is_prime(n)) count++;
        System.out.println(count);
    }
}
