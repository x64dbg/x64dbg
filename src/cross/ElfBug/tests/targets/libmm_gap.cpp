// Linked with a 64KB max page size, so the loader leaves ---p holes between its segments.
extern "C" int mm_gap_code(const int value)
{
    return value + 1;
}
