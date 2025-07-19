#pragma once

class IUpdateable
{
public:
	virtual void Update(float fDeltaTime) = 0;
};