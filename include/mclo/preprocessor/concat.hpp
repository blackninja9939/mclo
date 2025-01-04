#pragma once

#define MCLO_CONCAT_IMPL( A, ... ) A##__VA_ARGS__
#define MCLO_CONCAT( A, ... ) MCLO_CONCAT_IMPL( A, __VA_ARGS__ )
