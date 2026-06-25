#include <stdint.h>

#define KIB (1ULL << 10)
#define MIB (1ULL << 20)
#define GIB (1ULL << 30)
#define TIB (1ULL << 40)

#define ALIGN_UP(x, a) (((x) + ((a) - 1)) & ~((a) - 1))
#define ALIGN_DOWN(x, a) ((x) & ~((a) - 1))