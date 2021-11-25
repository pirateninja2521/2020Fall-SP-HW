#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s [player_id]\n", argv[0]);
        exit(1);
    }
    int player_id = atoi(argv[1]);
    int bid_list[21] = {20, 18, 5, 21, 8, 7, 2, 19, 14, 13, 9, 1, 6, 10, 16, 11, 4, 12, 15, 17, 3};

    for (int round = 1; round<=10; round++){
        int bid = bid_list[player_id + round - 2] * 100;
        printf("%d %d\n", player_id, bid);
    }

    return 0;
}