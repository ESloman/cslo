
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "object.h"
#include "value.h"

/**
 * Returns the next largest integer for the given double.
 * @param int argCount - the number of args provided
 * @param Value args - array of args - should be ONE with the double to ceil
 * @return Value - the computed value as Value
 */
Value math_ceil(int argCount, Value* args) {
    if (argCount == 0) {
        // error here
        printf("Not enough arguments provided to 'ceil'.\n");
        exit(-1);
    }

    Value arg = args[0];
    return NUMBER_VAL((double)ceil(AS_NUMBER(arg)));
}


/**
 * Returns the next smallest integer for the given double.
 * @param int argCount - the number of args provided
 * @param Value args - array of args - should be ONE with the double to floor
 * @return Value - the computed value as Value
 */
Value math_floor(int argCount, Value* args) {
    if (argCount == 0) {
        // error here
        printf("Not enough arguments provided to 'ceil'.\n");
        exit(-1);
    }

    Value arg = args[0];
    return NUMBER_VAL((double)floor(AS_NUMBER(arg)));
}
