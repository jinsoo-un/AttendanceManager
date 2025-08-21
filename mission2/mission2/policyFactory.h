#pragma once

#include "attendance.h"

struct PolicyBundle {
    IScoringPolicy* scoring;
    IGradePolicy* grading;
    IEliminationRule* elimination;
};

class IPolicyFactory {
public:
    virtual ~IPolicyFactory() {}
    virtual PolicyBundle create() = 0;
};

class DefaultPolicyFactory : public IPolicyFactory {
public:
    virtual PolicyBundle create();
};
