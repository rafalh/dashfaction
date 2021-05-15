#pragma once

class Installable
{
public:
    virtual ~Installable() = default;
    virtual void install() = 0;
};
