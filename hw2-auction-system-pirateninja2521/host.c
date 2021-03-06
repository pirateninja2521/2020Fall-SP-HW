#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>

void setname(char buf[128], int id){
    sprintf(buf, "fifo_%d.tmp", id);
    return;
}
void int2str(int i, char *s) {
    sprintf(s,"%d",i);
    return;
}
void close_and_redirect_fd(int read_pipe_fd[2], int write_pipe_fd[2]){  
    // read_pipe
    close(read_pipe_fd[1]);
    if (read_pipe_fd[0] != STDIN_FILENO){
        dup2(read_pipe_fd[0], STDIN_FILENO);
        close(read_pipe_fd[0]);
    }
    // write_pipe
    close(write_pipe_fd[0]);
    if (write_pipe_fd[1] != STDOUT_FILENO){
        dup2(write_pipe_fd[1], STDOUT_FILENO);
        close(write_pipe_fd[1]);
    }
    return;
}

int win2arr(int plr, int arr[8]){
    for (int i=0; i<8; i++){
        if (arr[i]==plr) return i;
    }
    return -1;
}

void handle_winner(int plr[2], int bid[2], int depth, int rec[8], int arr[8]){
    int win = (bid[0] > bid[1])? 0:1;
    if (depth==0){
        rec[win2arr(plr[win], arr)]++;
    }
    else{
        printf("%d %d\n", plr[win], bid[win]);
        fflush(stdout);
    }
    return;
}
 
void score2ranking(int arr[8], int rec[8], int key){
    int rank[8], picked[8]={0};
    int pre_plr;
    for(int i=1; i<=8; i++){
        int max = -1, max_plr = -1;
        for (int j=0; j<8; j++){
            if (picked[j]) continue;
            if (rec[j] > max){
                max_plr = j;
                max = rec[j];
            }
        }
        picked[max_plr]=1;
        if (i>1 && rec[max_plr]==rec[pre_plr]) rank[max_plr] = rank[pre_plr];
        else rank[max_plr] = i;
        pre_plr = max_plr;
    }
    char out[128], buf[128];
    sprintf(out, "%d\n", key);
    for (int i=0; i<8; i++){
        sprintf(buf,"%d %d\n", arr[i], rank[i]);
        strcat(out, buf);
    }
    printf("%s", out);
    fflush(stdout);
}

int main(int argc, char** argv) {
    if (argc != 4) {
        fprintf(stderr, "usage: %s [host_id] [key] [depth]\n", argv[0]);
        exit(1);
    }
    // printf("%s %s %s %s\n", argv[0], argv[1], argv[2], argv[3]);
    int host_id = atoi(argv[1]), key = atoi(argv[2]), depth = atoi(argv[3]);
    if (depth == 0){
        // Prepare to read and write FIFO file.
        char name[128];
        setname(name, host_id);
        int in_fifo_fd=open(name, O_RDONLY);
        dup2(in_fifo_fd, STDIN_FILENO);
        close(in_fifo_fd);
        int out_fifo_fd=open("fifo_0.tmp", O_WRONLY);
        dup2(out_fifo_fd, STDOUT_FILENO);
        close(out_fifo_fd);
    }
    char new[128];
    FILE *fp0r , *fp0w, *fp1r, *fp1w;
    int child_pid[2];
    int pipe_fd[2][2][2];
    if (depth!= 2){
        // Fork child hosts.
        for (int i=0; i<2; i++){ // child 1 or 2
            for (int j=0; j<2; j++){ // read or write (for child)
                pipe(pipe_fd[i][j]);
            }
        }
        int2str(depth+1, new);
        if ((child_pid[0]=fork())==0){ // child0
            // printf("child0\n");
            close_and_redirect_fd(pipe_fd[0][0], pipe_fd[0][1]);
            if (execlp("./host", "./host", argv[1], argv[2], new, NULL) < 0){
                fprintf(stderr, "execlp error\n");
                exit(1);
            }
        }
        else if ((child_pid[1]=fork())==0){ // child1
            // printf("child1\n");
            close_and_redirect_fd(pipe_fd[1][0], pipe_fd[1][1]);
            if (execlp("./host", "./host", argv[1], argv[2], new, NULL) < 0){
                fprintf(stderr, "execlp error\n");
                exit(1);
            }
        }
        else{ // parent
            // printf("parent\n");
            close(pipe_fd[0][1][1]);
            close(pipe_fd[0][0][0]);
            close(pipe_fd[1][1][1]);
            close(pipe_fd[1][0][0]);
            fp0r = fdopen(pipe_fd[0][1][0], "r"); // read from child 0
            fp0w = fdopen(pipe_fd[0][0][1], "w"); // write to child 0
            fp1r = fdopen(pipe_fd[1][1][0], "r"); // read from child 1
            fp1w = fdopen(pipe_fd[1][0][1], "w"); // write to child 1

        }
    }
    int cnt=0;
    while(1){
        int arr[8]={0};     
        for (int i=0; i < (1<<(3-depth)); i++){
            scanf("%d", &arr[i]);
        }
        /* if (depth==0){
            // fprintf( stderr, "Host id: %d, key: %d\n", host_id, key);
            // fprintf( stderr, "cnt: %d\n", ++cnt);
            for (int i=0; i < 8; i++){
                fprintf( stderr, "%d ", arr[i]);
            }
            fprintf(stderr, "\n");
        } */
        if (depth != 2){ // Send half the players to each child host if not leaf host.
            for (int i=0; i<(1<<2-depth); i++){
                // printf("%d %d\n", arr[i], arr[i+(1<<2-depth)]);
                fprintf(fp0w, "%d ", arr[i]);
                fprintf(fp1w, "%d ", arr[i+(1<<2-depth)]);
            }
            fprintf(fp0w, "\n");
            fflush(fp0w);
            fprintf(fp1w, "\n");
            fflush(fp1w);
            if (arr[0]==-1){ // to end 
                waitpid(-1, NULL, 0);
                waitpid(-1, NULL, 0);
                return 0;
            }
        }
        else{ // Fork players for leaf host.
            if (arr[0]==-1){ // to end 
                return 0;
            }
            for (int i=0; i<2; i++){ // child 1 or 2
                for (int j=0; j<2; j++){ // read or write (for child)
                    pipe(pipe_fd[i][j]);
                }
            }
            if ((child_pid[0]=fork())==0){ // child0
                close_and_redirect_fd(pipe_fd[0][0], pipe_fd[0][1]);
                int2str(arr[0], new);
                if (execlp("./player", "./player", new, NULL) < 0){
                    fprintf(stderr, "execlp error\n");
                    exit(1);
                }
            }
            else if ((child_pid[1]=fork())==0){ // child1
                close_and_redirect_fd(pipe_fd[1][0], pipe_fd[1][1]);
                int2str(arr[1], new);
                if (execlp("./player", "./player", new, NULL) < 0){
                    fprintf(stderr, "execlp error\n");
                    exit(1);
                }
            }
            else{ // parent
                close(pipe_fd[0][1][1]);
                close(pipe_fd[0][0][0]);
                close(pipe_fd[1][1][1]);
                close(pipe_fd[1][0][0]);
                waitpid(-1, NULL, 0);
                waitpid(-1, NULL, 0);
                fp0r = fdopen(pipe_fd[0][1][0], "r"); // read from child 0
                fp0w = fdopen(pipe_fd[0][0][1], "w"); // write to child 0
                fp1r = fdopen(pipe_fd[1][1][0], "r"); // read from child 1
                fp1w = fdopen(pipe_fd[1][0][1], "w"); // write to child 1
            }

        }
        // exit(1);
        int rec[8]={0};
        for (int i=0; i<10; i++){ // Iterate 10 rounds of bid comparison
            // Read the 2 players and their bids from child hosts or players via pipe, then compare their bids.
            int plr[2], bid[2];
            fscanf(fp0r, "%d%d", &plr[0], &bid[0]);
            fscanf(fp1r, "%d%d", &plr[1], &bid[1]);
            handle_winner(plr, bid, depth, rec, arr);
        }
        if (depth==0){
            score2ranking(arr, rec, key);
        }
        // sleep(1);
    }

    return 0;
}
