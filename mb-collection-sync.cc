#include <iostream>

#include "CollectionSync.h"

void Usage(const std::string& ProgName)
{
	std::cerr << "Usage: " << ProgName << " user password collection path" << std::endl;
}

int main(int argc, const char *argv[])
{
	if (argc==5)
	{
		CCollectionSync Sync(argv[1],argv[2],argv[3],argv[4]);
	}
	else
	{
		Usage(argv[0]);
	}

	return 0;
}
