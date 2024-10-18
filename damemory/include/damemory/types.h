#ifndef DAMEMORY__TYPES_H
#define DAMEMORY__TYPES_H

enum damemory_accesspattern
{
    DAMEMORY_ACCESSPATTERN_SEQUENTIAL,
    DAMEMORY_ACCESSPATTERN_RANDOM,
};

enum damemory_bandwidth
{
    DAMEMORY_BANDWIDTH_VERYLOW,
    DAMEMORY_BANDWIDTH_LOW,
    DAMEMORY_BANDWIDTH_MEDIUM,
    DAMEMORY_BANDWIDTH_HIGH,
};

enum damemory_granularity
{
    DAMEMORY_GRANULARITY_SMALL,
    DAMEMORY_GRANULARITY_BIG,
};

enum damemory_avglatency
{
    DAMEMORY_AVGLATENCY_VERYLOW,
    DAMEMORY_AVGLATENCY_LOW,
    DAMEMORY_AVGLATENCY_MEDIUM,
    DAMEMORY_AVGLATENCY_HIGH,
};

enum damemory_taillatency
{
    DAMEMORY_TAILLATENCY_VERYLOW,
    DAMEMORY_TAILLATENCY_LOW,
    DAMEMORY_TAILLATENCY_MEDIUM,
    DAMEMORY_TAILLATENCY_HIGH,
};

enum damemory_persitency
{
    DAMEMORY_PERSISTENCY_NONPERSISTENT,
    DAMEMORY_PERSISTENCY_PERSISTENT,
};

struct damemory_properties
{
    damemory_accesspattern access_pattern;
    damemory_bandwidth bandwidth;
    damemory_granularity granularity;
    damemory_avglatency avg_latency;
    damemory_taillatency tail_latency;
    damemory_persitency persistency;
};

typedef unsigned long long damemory_id;

#endif
