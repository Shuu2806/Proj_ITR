#include <stdio.h>
#include <stdlib.h>

#include "assembly.h"
#include "assembly.c"

int main(){
    assembly_line_t line;
    init_assembly_line(&line);
    setup_arm(line, PART_FRAME, LEFT, 1);
    setup_arm(line, PART_ENGINE, LEFT, 2);
    setup_arm(line, PART_WHEELS, RIGHT, 2);
    setup_arm(line, PART_BODY, LEFT, 3);
    setup_arm(line, PART_DOORS, RIGHT, 4);
    setup_arm(line, PART_LIGHTS, RIGHT, 4);
    setup_arm(line, PART_WINDOWS, RIGHT, 5); 
    run_assembly(line);
    print_assembly_stats(line);
    shutdown_assembly(line);
    free_assembly_line(&line);
    return 0;
}