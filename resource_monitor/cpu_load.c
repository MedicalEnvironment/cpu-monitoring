#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    unsigned long long user;
    unsigned long long nice;
    unsigned long long system;
    unsigned long long idle;
    unsigned long long iowait;
    unsigned long long irq;
    unsigned long long softirq;
} CPUStats;

int get_cpu_stats(CPUStats *stats) {
    FILE *file = fopen("/proc/stat", "r");
    if (!file) {
        perror("Error opening /proc/stat");
        return 1;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "cpu ", 4) == 0) {
            sscanf(line + 5, "%llu %llu %llu %llu %llu %llu %llu", &stats->user, &stats->nice, &stats->system, &stats->idle, &stats->iowait, &stats->irq, &stats->softirq);
            break;
        }
    }

    fclose(file);
    return 0;
}

double calculate_cpu_load_percentage(const CPUStats *prev, const CPUStats *curr) {
    unsigned long long prev_idle = prev->idle + prev->iowait;
    unsigned long long curr_idle = curr->idle + curr->iowait;

    unsigned long long prev_total = prev_idle;
    unsigned long long curr_total = curr_idle;

    for (const unsigned long long *prev_val = &prev->user, *curr_val = &curr->user; prev_val <= &prev->softirq; ++prev_val, ++curr_val) {
        prev_total += *prev_val;
        curr_total += *curr_val;
    }

    unsigned long long total_diff = curr_total - prev_total;
    unsigned long long idle_diff = curr_idle - prev_idle;

    return 100.0 * (1.0 - (double)idle_diff / (double)total_diff);
}

int main() {
    CPUStats prev_stats, curr_stats;

    if (get_cpu_stats(&prev_stats)) {
        return 1;
    }

    sleep(1);

    while (1) {
        if (get_cpu_stats(&curr_stats)) {
            return 1;
        }

        double cpu_load_percentage = calculate_cpu_load_percentage(&prev_stats, &curr_stats);
        printf("CPU Load: %.2f%%\n", cpu_load_percentage);

        prev_stats = curr_stats;
        sleep(1);
    }

    return 0;
}
