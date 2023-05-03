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

int get_cpu_count() {
    FILE *file = fopen("/proc/cpuinfo", "r");
    if (!file) {
        perror("Error opening /proc/cpuinfo");
        return -1;
    }

    int count = 0;
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "processor", 9) == 0) {
            count++;
        }
    }

    fclose(file);
    return count;
}

int get_cpu_stats(CPUStats *stats, int cpu_count) {
    FILE *file = fopen("/proc/stat", "r");
    if (!file) {
        perror("Error opening /proc/stat");
        return 1;
    }

    char line[256];
    int found_count = 0;
    while (fgets(line, sizeof(line), file) && found_count <= cpu_count) {
        int cpu_idx;
        if (sscanf(line, "cpu%d", &cpu_idx) == 1) {
            if (cpu_idx < cpu_count) {
                sscanf(line + 5, "%llu %llu %llu %llu %llu %llu %llu", &stats[cpu_idx].user, &stats[cpu_idx].nice, &stats[cpu_idx].system, &stats[cpu_idx].idle, &stats[cpu_idx].iowait, &stats[cpu_idx].irq, &stats[cpu_idx].softirq);
                found_count++;
            }
        } else if (strncmp(line, "cpu ", 4) == 0) {
            sscanf(line + 5, "%llu %llu %llu %llu %llu %llu %llu", &stats[cpu_count].user, &stats[cpu_count].nice, &stats[cpu_count].system, &stats[cpu_count].idle, &stats[cpu_count].iowait, &stats[cpu_count].irq, &stats[cpu_count].softirq);
            found_count++;
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
    int cpu_count = get_cpu_count();
    if (cpu_count < 0) {
        return 1;
    }

    CPUStats *prev_stats = calloc(cpu_count + 1, sizeof(CPUStats));
    CPUStats *curr_stats = calloc(cpu_count + 1, sizeof(CPUStats));

    if (get_cpu_stats(prev_stats, cpu_count)) {
        free(prev_stats);
        free(curr_stats);
        return 1;
    }

    sleep(1);

    while (1) {
        if (get_cpu_stats(curr_stats, cpu_count)) {
            free(prev_stats);
            free(curr_stats);
            return 1;
        }

        double total_cpu_load_percentage = calculate_cpu_load_percentage(&prev_stats[cpu_count], &curr_stats[cpu_count]);
        printf("Total CPU Load: %.2f%%\n", total_cpu_load_percentage);

        for (int i = 0; i < cpu_count; ++i) {
            double cpu_load_percentage = calculate_cpu_load_percentage(&prev_stats[i], &curr_stats[i]);
            printf("CPU%d Load: %.2f%%\n", i, cpu_load_percentage);
        }
        printf("\n");

        memcpy(prev_stats, curr_stats, (cpu_count + 1) * sizeof(CPUStats));
        sleep(1);
    }

    free(prev_stats);
    free(curr_stats);
    return 0;
}


