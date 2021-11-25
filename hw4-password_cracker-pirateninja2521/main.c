#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <openssl/md5.h>
#include <pthread.h>

#define KGRN "\033[0;32;32m"
#define RESET "\033[0m"


void md52str(unsigned char *in, char out[64])
{
    char buf[4];
    int i;
    memset(out, 0, 64);
    for(i = 0 ; i < MD5_DIGEST_LENGTH ; i++) {
        if ( in[i] < 0x10)
            sprintf(buf, "0%1x", in[i]);
        else
            sprintf(buf, "%2x", in[i]);
        strcat(out, buf);
    }
}

void set_suffix(int num, char suffix[64]){
    memset(suffix, 0, 64);
    int i=0;
    while (num!=0){
        suffix[i] = (char)(num%94 + 33);
        i++;
        num/=94;
    }
}

char prefix[11][7][129], ans[11][1024], *goal, *outfile;
int N, M;

void *cracker(void *m){
    int i = (int) m, num;
    char suffix[64], try[256], out[64];
    unsigned char hash[MD5_DIGEST_LENGTH];
    for (int n=1; n<=N; n++){
        num = 1;
        while(num++){
            set_suffix(num, suffix);
            sprintf(try, "%s%s", prefix[i][n], suffix);
            MD5(try, strlen(try), hash);
            md52str(hash, out);
            if (strncmp(goal, out, n)==0){
                strcat(ans[i], try);
                // printf("%s %s\n", try[i], out[i]);
                strcat(ans[i], "\n");
                strcpy(prefix[i][n+1], try);
                break;
            }
        }
    }
}
int main(int argc, char** argv)
{
    if (argc != 6) {
        fprintf(stderr, "usage: ./cracker [Prefix] [Goal] [N] [M] [outfile]\n");
        exit(1);
    }
    N = atoi(argv[3]);
    M = atoi(argv[4]);
    goal = argv[2];
    FILE *fp = fopen(argv[5], "w");

    pthread_t tid[11];

    for (int m=1; m<=M; m++){
        strcpy(prefix[m][1], argv[1]);
        int len = strlen(prefix[m][1]);
        prefix[m][1][len] = (char)m+'0';
        pthread_create(&tid[m], NULL, cracker, (void *) m);
    }
    for(int i=1; i<=M; i++) {
        pthread_join(tid[i], NULL);
    }
    for(int i=1; i<=M; i++){
        fprintf(fp, "%s===\n", ans[i]);
        // fprintf(stderr, "%s===\n", ans[i]);
    }
    return 0;

}