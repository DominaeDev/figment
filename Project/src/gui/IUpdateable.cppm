export module GUI.Layout.IUpdateable;

export class IUpdateable
{
public:
	virtual ~IUpdateable() = default;
	virtual void Update(float fDeltaTime) = 0;
};