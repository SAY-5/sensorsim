/* sensorsim CLI: emit n samples + (v3) generate / verify a golden
 * trace.
 *
 * Usage:
 *   sensorsim sample --n 100 --dt 0.01
 *   sensorsim golden generate --n 100 --dt 0.01 --out trace.jsonl
 *   sensorsim golden verify  --in  trace.jsonl   (TBD: load + verify)
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ss/fault.h"
#include "ss/golden.h"
#include "ss/sensor.h"

static int cmd_sample(int argc, char** argv) {
    int n = 100;
    double dt = 0.01;
    static struct option opts[] = {
        {"n", required_argument, 0, 'n'},
        {"dt", required_argument, 0, 'd'},
        {0, 0, 0, 0},
    };
    int c;
    while ((c = getopt_long(argc, argv, "n:d:", opts, NULL)) != -1) {
        switch (c) {
            case 'n': n = atoi(optarg); break;
            case 'd': dt = strtod(optarg, NULL); break;
            default: return 2;
        }
    }
    sensor_t s;
    ss_sensor_init(&s, 1, 0.001, 0.05, 0.01, 42);
    fault_t f;
    ss_fault_init(&f, SS_FAULT_NONE);
    for (int i = 0; i < n; ++i) {
        double t = (double)i * dt;
        double truth = 25.0;
        double sample = ss_sensor_sample(&s, truth, t);
        sample = ss_fault_apply(&f, sample);
        printf("{\"t\":%.6f,\"sample\":%.6f}\n", t, sample);
    }
    return 0;
}

static int cmd_golden_generate(int argc, char** argv) {
    int n = 100;
    double dt = 0.01;
    const char* out_path = NULL;
    static struct option opts[] = {
        {"n", required_argument, 0, 'n'},
        {"dt", required_argument, 0, 'd'},
        {"out", required_argument, 0, 'o'},
        {0, 0, 0, 0},
    };
    int c;
    while ((c = getopt_long(argc, argv, "n:d:o:", opts, NULL)) != -1) {
        switch (c) {
            case 'n': n = atoi(optarg); break;
            case 'd': dt = strtod(optarg, NULL); break;
            case 'o': out_path = optarg; break;
            default: return 2;
        }
    }
    sensor_t s;
    ss_sensor_init(&s, 1, 0.001, 0.05, 0.01, 42);
    fault_t f;
    ss_fault_init(&f, SS_FAULT_NONE);
    golden_trace_t g;
    ss_golden_init(&g);
    ss_golden_generate(&g, &s, &f, dt, n);
    FILE* out = out_path ? fopen(out_path, "w") : stdout;
    ss_golden_write_jsonl(out, &g);
    if (out_path) fclose(out);
    ss_golden_free(&g);
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr,
                "usage: sensorsim {sample|golden} ...\n"
                "  sample --n N --dt SEC\n"
                "  golden generate --n N --dt SEC --out PATH\n");
        return 2;
    }
    if (strcmp(argv[1], "sample") == 0) return cmd_sample(argc - 1, argv + 1);
    if (strcmp(argv[1], "golden") == 0 && argc >= 3 && strcmp(argv[2], "generate") == 0) {
        return cmd_golden_generate(argc - 2, argv + 2);
    }
    fprintf(stderr, "unknown command: %s\n", argv[1]);
    return 2;
}
