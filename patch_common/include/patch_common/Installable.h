#pragma once

class Installable
{
public:
    virtual ~Installable() {}
    virtual void install() = 0;
};
