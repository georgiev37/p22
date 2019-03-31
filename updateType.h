#ifndef UPDATETYPE_H
#define UPDATETYPE_H

#include "cstdint"

enum class UpdateType : int64_t {
	New = 0,
	Change = 1,
	Cancel = 2
};
#endif