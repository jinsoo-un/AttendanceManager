#include "PolicyFactory.h"

PolicyBundle DefaultPolicyFactory::create() {
    PolicyBundle b;
    b.scoring = new DefaultScoringPolicy();
    b.grading = new ThresholdGradePolicy();
    b.elimination = new NormalNoWedWeekendElimination();
    return b;
}
