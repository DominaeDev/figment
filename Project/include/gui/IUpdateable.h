#ifndef IUPDATEABLE_H__
#define IUPDATEABLE_H__

class IUpdateable
{
public:
	virtual ~IUpdateable() = default;
	virtual void Update(float fDeltaTime) = 0;
};
#endif