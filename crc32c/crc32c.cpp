// crc32c.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "crc32c.h"


// This is an example of an exported variable
CRC32C_API int ncrc32c=0;

// This is an example of an exported function.
CRC32C_API int fncrc32c(void)
{
	return 42;
}

// This is the constructor of a class that has been exported.
// see crc32c.h for the class definition
Ccrc32c::Ccrc32c()
{
	return;
}
