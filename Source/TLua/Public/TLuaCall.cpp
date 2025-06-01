
#include "TLuaCall.hpp"

#include "CoreMinimal.h"

namespace TLua
{

	static void RegisterActor()
	{

	}

	static void RegisterComponent()
	{
	}

	void RegisterUnreal()
	{
		RegisterComponent();
		RegisterActor();
	}
}