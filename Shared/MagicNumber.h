#pragma once

#define RR_SECRET64 (0ull)
#define RR_SECRET32 (RR_SECRET64 & 4294967295u)
#define RR_SECRET8 (RR_SECRET32 & 255)
