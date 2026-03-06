#ifndef TEST_APU_H
#define TEST_APU_H

/**
 * @brief NES APU hardware-correctness test suite.
 *
 * Tests are grounded in the NESDev APU documentation:
 *   https://www.nesdev.org/wiki/APU
 *
 * Covers pulse, triangle, noise channels.  DMC is skipped (not yet
 * implemented in apu.c).  Each test is labelled with the register
 * writes that set up the channel so results can be reproduced by
 * hand or compared against a reference emulator.
 */

/* Run the full suite.  Prints a formatted table and returns the
 * number of failing tests (0 = all pass). */
int run_apu_test_suite(void);

#endif
