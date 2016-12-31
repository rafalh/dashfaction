#pragma once

class Revertable
{
public:
	virtual ~Revertable()
	{
		Revert();
	}

	virtual void Revert() {}
};
