#ifndef TEST_APU_H
#define TEST_APU_H

/**
 * @brief NES APU hardware-correctness test suite.
 *
 * Tests are grounded in the NESDev APU documentation:
 *   https://www.nesdev.org/wiki/APU
 *
 * Covers pulse, triangle, noise, and DMC channels.
 *
 * DMC tests exercise the output unit, register interface, IRQ, loop,
 * and DMA signalling in isolation — no CPU, bus, or ROM is required.
 * Bytes are injected directly via dmc_load_sample_byte() to simulate
 * what the bus would deliver after a real DMA fetch.  The only path
 * not covered here is the actual cartridge read in bus_service_dmc_dma().
 *
 * Bug fix: previous versions of the measurement helpers (max_sample,
 * mean_sample, measure_frequency, nonzero_ratio) incorrectly used the
 * return value of apu_tick(), which returns void.  All helpers have
 * been corrected to call apu_get_output() explicitly after each tick.
 */

/* Run the full suite.  Prints a formatted table and returns the
 * number of failing tests (0 = all pass). */
int run_apu_test_suite(void);

#endif
