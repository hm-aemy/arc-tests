#include <stdbool.h>

int main() {
    bool clk = 0;
    for (int i = 0; i < 32; i++){
        clk = ~clk;
    }
    return(0);

}