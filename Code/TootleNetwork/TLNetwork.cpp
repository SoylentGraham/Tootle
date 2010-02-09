#include "TLNetwork.h"

namespace TLNetwork
{
	namespace Platform
	{
		// Wrapper pass-through routine to create a connection object of the correct platform specific type.
		// Eventually this should probably be added to a network manager which keeps track of the connection
		// as and when it is needed
		// Creates the appropriate connection class based on the platform being built
		TPtr<TLNetwork::TConnection> CreateConnection();
	}
}


TPtr<TLNetwork::TConnection> TLNetwork::CreateConnection()
{
	return Platform::CreateConnection();
}
