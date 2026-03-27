#include <stdio.h>
long long matmul_sum(int n){
    long long total=0,i,j,k,a,b,acc;
    for(i=0;i<n;i++) for(j=0;j<n;j++){
        acc=0;
        for(k=0;k<n;k++){a=(i+k)%17;b=(k*j+1)%13;acc+=a*b;}
        total+=acc;}
    return total;}
int main(){printf("%lld\n%lld\n",matmul_sum(4),matmul_sum(32));return 0;}
